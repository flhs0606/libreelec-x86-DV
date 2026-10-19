/* P3i T4c tone-control preparation and fixed-point gamma helpers. */
#include "tone_controls.h"
#include "tone_curve.h"
#include "trim_luminance.h"
#include "control_luminance.h"
#include "byte_utils.h"
#include "tone_tables.inc"

typedef char tone_require_int[(sizeof(int) == 4) ? 1 : -1];
typedef char tone_require_short[(sizeof(short) == 2) ? 1 : -1];
typedef char tone_require_source[(sizeof(struct tone_curve_source) == 56) ? 1 : -1];
typedef char tone_require_display[(sizeof(struct tone_curve_display) == 48) ? 1 : -1];
typedef char tone_require_pair[(sizeof(struct tone_curve_pair) == 8) ? 1 : -1];
static int signed32(unsigned int x) { return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static unsigned int rd32(const void *p, unsigned int at)
{
    const unsigned char *b = (const unsigned char *)p + at;
    return (unsigned int)b[0] | ((unsigned int)b[1] << 8) |
           ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24);
}
static unsigned short rd16(const void *p, unsigned int at)
{
    const unsigned char *b = (const unsigned char *)p + at;
    return (unsigned short)((unsigned int)b[0] | ((unsigned int)b[1] << 8));
}
static int signed16(unsigned short x) { return x <= 0x7fffu ? (int)x : -1 - (int)(unsigned short)~x; }
static void wr32(void *p, unsigned int at, unsigned int x)
{
    unsigned char *b = (unsigned char *)p + at;
    b[0] = (unsigned char)x; b[1] = (unsigned char)(x >> 8);
    b[2] = (unsigned char)(x >> 16); b[3] = (unsigned char)(x >> 24);
}
static void wr16(void *p, unsigned int at, unsigned int x)
{
    unsigned char *b = (unsigned char *)p + at;
    b[0] = (unsigned char)x; b[1] = (unsigned char)(x >> 8);
}
void update_tc_ctrl_level(const void *tm, void *tc)
{
    unsigned int i;
    for (i = 1; i < 7; ++i)
        wr32(tc, 116 + i * 4, rd32(tm, 48 + i * 4) + rd32(tm, 76 + i * 4));
    if (rd32(tc, 56) == 1 && rd32(tc, 60) == 0)
        wr32(tc, 136, rd32(tm, 68) + rd32(tm, 132));
    clamp_level2((int *)((unsigned char *)tc + 116));
    wr32(tc, 140, (rd32(tc, 140) - 2048u) * 2u);
    for (i = 1; i < 4; ++i) wr32(tc, 144 + i * 4, rd32(tm, 136 + i * 4));
}
void tm_ctrl_to_tc_ctrl(const void *tm, const void *dm, void *tc, int graphics_source)
{
    unsigned int direction = rd32(tc, 56), invertible = 0, rolloff, i;
    if (rd32(tm, 0) != 0 && direction == 1 && graphics_source != 0 && rd32(dm, 0) == 1)
        invertible = 1;
    wr32(tc, 60, invertible);
    wr32(tc, 64, rd32(tm, 4));
    rolloff = rd32(tm, 4);
    wr32(tc, 68, rolloff ? 0x80000000u / rolloff : 0x80000000u);
    wr32(tc, 72, rd32(tm, 8)); wr32(tc, 76, rd32(tm, 12));
    wr32(tc, 80, rd32(tm, 20)); wr32(tc, 84, rd32(tm, 28));
    if (direction == 1 && invertible == 0) {
        wr32(tc, 76, rd32(tm, 16)); wr32(tc, 80, rd32(tm, 24));
    }
    for (i = 0; i < 5; ++i) wr16(tc, 104 + i * 2, rd16(tm, 32 + i * 2));
    if (direction == 1 && invertible == 0) wr16(tc, 108, 0u - rd16(tm, 36));
}
int log10_taylor(int value)
{
    int l = signed32((unsigned int)fx_log2_taylor(value, 26) - 0x7c000000u);
    return signed32((unsigned int)((unsigned long long)((long long)l * 0x268826a1) >> 31));
}
void update_tc_signal_hlg_a(int minimum, int maximum, void *signal)
{
    int input = signed32((unsigned int)maximum << 9);
    int l = log10_taylor(input / 1000);
    wr32(signal, 12, (unsigned int)minimum);
    wr32(signal, 16, (unsigned int)maximum - (unsigned int)minimum);
    wr32(signal, 8, (unsigned int)l * 0x1ae1u + 0x4cccu);
}
void target_signal_to_tc_ctrl(const void *target, void *tc)
{
    unsigned int pair = rd32(target, 192);
    unsigned char *signal = (unsigned char *)tc + 28;
    if (!pair) {
        unsigned int maximum = rd32(target, 188), diagonal = rd16(target, 196);
        wr16(signal, 0, control_l_to_pq12(rd32(target, 184)));
        unsigned short max_pq = control_l_to_pq12(maximum);
        unsigned int hlg;
        wr16(signal, 4, diagonal);
        wr16(signal, 2, max_pq);
        hlg = rd32(target, 40) == 3 ? 1u : 0u;
        wr16(signal, 6, hlg);
        if (hlg)
            update_tc_signal_hlg_a(signed32(rd32(target, 184)), signed32(rd32(target, 188)), signal);
    } else {
        wr16(signal, 0, rd16(target, 192)); wr16(signal, 2, rd16(target, 194));
        wr16(signal, 4, rd16(target, 196));
    }
}
unsigned int de_gamma(unsigned short a, unsigned short b, unsigned short gamma,
                      unsigned int gain, unsigned short y)
{
    static const unsigned char scale[10] = {19,22,25,29,32,32,32,32,32,32};
    unsigned int sum = (unsigned int)y + b, input = sum > 65535u ? sum >> 1 : sum;
    unsigned int index = input >> 4, exponent, power;
    unsigned long long result;
    exponent = (tone_log10_lut[0][index] - (((input & 15u) * tone_log10_rate_lut[0][index]) >> 8)) * gamma;
    index = exponent >> 26;
    if (index > 9u) return 0;
    power = tone_power10_lut[index][(exponent >> 16) & 1023u];
    if (sum > 65535u) power = (unsigned int)(((unsigned long long)power * gain) >> 18);
    result = (unsigned long long)power * a;
    return (unsigned int)(result >> ((scale[index] - 16u) & 63u));
}
void calc_ta_tb(int gamma, unsigned int maximum, unsigned int minimum, int *ta, int *tb)
{
    unsigned int norm = unorm_s(gamma), diff_norm;
    int normalized = signed32((unsigned int)gamma << (norm & 31u));
    int inverse = sdiv_sls(0x2000000000000000ll, normalized);
    int high = fx_log2_taylor((int)(maximum >> 4), 26);
    int low, difference, result;
    high = smpy_ss(inverse, signed32((unsigned int)high - 0x38000000u));
    high = fx_pow2_taylor(high, signed32(41u - norm), 17);
    low = fx_log2_taylor((int)(minimum >> 4), 26);
    low = smpy_ss(inverse, signed32((unsigned int)low - 0x38000000u));
    low = fx_pow2_taylor(low, signed32(41u - norm), 17);
    difference = signed32((unsigned int)high - (unsigned int)low);
    result = fx_log2_taylor(difference, 26);
    result = smpy_ss(normalized, signed32((unsigned int)result - 0x44000000u));
    result = fx_pow2_taylor(result, signed32(norm + 10u), 17);
    *ta = result;
    diff_norm = unorm_s(difference);
    inverse = sdiv_sls(0x2000000000000000ll, signed32((unsigned int)difference << (diff_norm & 31u)));
    result = smpy_ss(inverse, low);
    *tb = sshr_sss(result, signed32(5u - diff_norm));
}
int create_tone_curve_changed(const void *tc, void *kernel_tmap, void *used,
                              const struct tone_curve_ops *ops)
{
    struct tone_curve_source source;
    struct tone_curve_display display;
    struct tone_curve_pair p1, p2, p3;
    int slope, result;
    unsigned int direction = rd32(tc, 56), selected = direction == 1u ? 28u : 0u;
    unsigned char *tmap = (unsigned char *)kernel_tmap;
    display.min_pq = signed32((unsigned int)rd16(tc, 28) << 19);
    display.max_pq = signed32((unsigned int)rd16(tc, 30) << 19);
    display.min_bias = signed32((unsigned int)signed16(rd16(tc, 104)) << 19);
    display.mid_bias = signed32((unsigned int)signed16(rd16(tc, 108)) << 19);
    display.max_bias = signed32((unsigned int)signed16(rd16(tc, 106)) << 19);
    display.contrast = signed32((unsigned int)signed16(rd16(tc, 110)) << 19);
    display.brightness = signed32((unsigned int)signed16(rd16(tc, 112)) << 19);
    display.diagonal_inches = rd16(tc, 32); display.bright_preserve = signed32(rd32(tc, 84));
    display.rolloff = signed32(rd32(tc, 64)); display.rolloff_inverse = signed32(rd32(tc, 68));
    display.mode = (unsigned short)(direction == 1u || direction == 2u ? direction : 0u);
    source.crush = signed32((unsigned int)rd16(tc, selected + 20u) << 19);
    source.mid = signed32((unsigned int)rd16(tc, selected + 24u) << 19);
    source.clip = signed32((unsigned int)rd16(tc, selected + 22u) << 19);
    source.min_pq = signed32((unsigned int)rd16(tc, 0) << 19);
    source.max_pq = signed32((unsigned int)rd16(tc, 2) << 19);
    source.key_weight = signed32(rd32(tc, 72) << 4);
    source.trim_slope = signed32(rd32(tc, 120) << 16);
    source.trim_offset = signed32(rd32(tc, 124) << 16);
    source.trim_power = signed32(rd32(tc, 128) << 16);
    source.chroma_weight = 0; source.saturation_gain = signed32(rd32(tc, 136) << 16);
    source.diagonal_inches = rd16(tc, 4);
    source.intensity_weight = signed32(rd32(tc, 76) << 4);
    source.chroma_vector_weight = signed32(rd32(tc, 80) << 4);
    if (!ops || !ops->build) ops = tone_curve_concrete_ops();
    ops->build(ops->opaque, &source, &display, &p1, &p2, &p3, &slope,
               (short *)(tmap + 4), (short *)(tmap + 1028), (short *)(tmap + 2056),
               (short *)(tmap + 3080), (int *)0, 512, (short)(direction == 2u ? 0 : 1));
    mem_cpy_byte(used, tc, 160);
    result = signed16(rd16(tmap, 4)) == signed16(rd16(tmap, 4u + (unsigned int)(signed16(rd16(tmap, 0)) * 2))) ? 0x100000 : 1;
    return result;
}
int create_tone_curve(const void *tc, void *kernel_tmap, void *used, const struct tone_curve_ops *ops)
{
    if (mem_eq_byte(used, tc, 160)) return 0;
    return create_tone_curve_changed(tc, kernel_tmap, used, ops);
}
