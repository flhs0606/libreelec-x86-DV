/* Generate tone-curve samples with explicit integer wrapping and shifts. */
#include "tone_curve_lut.h"
#include "tone_math.h"
#include "trim_luminance.h"
#include "tone_curve_lut_table.inc"

static int s32(unsigned int x)
{ return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static long long s64(unsigned long long x)
{ return x <= 0x7fffffffffffffffull ? (long long)x : -1 - (long long)~x; }
static long long add64(long long a, long long b)
{ return s64((unsigned long long)a + (unsigned long long)b); }
static long long sub64(long long a, long long b)
{ return s64((unsigned long long)a - (unsigned long long)b); }
static long long mul64(long long a, long long b)
{ return s64((unsigned long long)a * (unsigned long long)b); }
static long long lsl64(long long a, unsigned int shift)
{ return s64((unsigned long long)a << (shift & 63u)); }
static long long div64(long long a, long long b)
{
    if (!b) return 0;
    if (a == (-0x7fffffffffffffffll - 1) && b == -1) return a;
    return a / b;
}
static int sat32(long long x)
{ return x > 2147483647 ? 2147483647 : x < (-2147483647 - 1) ? (-2147483647 - 1) : (int)x; }
static short narrow16(unsigned int x)
{ x &= 65535u; return x <= 32767u ? (short)x : (short)(-1 - (int)(~x & 65535u)); }

long long fx_pow2_taylor_int(int value, int scale_in, int scale_out)
{
    unsigned int count;
    int fraction, factor;
    if (value <= 0x4000000) return fx_pow2_taylor(value, scale_in, scale_out);
    count = ((unsigned int)value - 0x4000001u) >> 26;
    fraction = s32((unsigned int)value - 0x4000000u - (count << 26));
    factor = s32(1u << ((count + 1u) & 31u));
    return (long long)fx_pow2_taylor(fraction, scale_in, scale_out) * factor;
}

static int clamp(int value, int minimum, int maximum)
{ if (value < minimum) value = minimum; if (value > maximum) value = maximum; return value; }

static int interpolate(const short *p, int a, int b, int c, int d, int maximum)
{
    unsigned int sum = (unsigned int)(int)p[0] * (unsigned int)a;
    int result;
    sum += (unsigned int)(int)p[1] * (unsigned int)b;
    sum += (unsigned int)(int)p[2] * (unsigned int)c;
    sum += (unsigned int)(int)p[3] * (unsigned int)d;
    result = s32(sum + 0x8000u) >> 16;
    return clamp(result, p[1], maximum);
}

void generate_tone_curve_lut(int x_min, int x_max,
    const struct tone_curve_pair *param1, const struct tone_curve_pair *param2,
    const struct tone_curve_pair *param3, int slope_over_rolloff,
    const struct tone_curve_display *display, int trim_slope, int trim_power,
    int trim_offset, short *lut515, short *lut4k, short lut_size, short clamp_output)
{
    int step = lut_size ? 4096 / lut_size : 0;
    unsigned int common, common3, previous = 0;
    unsigned int trim_slope_one = (unsigned int)trim_slope + 0x10000000u;
    unsigned int trim_power_one = (unsigned int)trim_power + 0x10000000u;
    short *out = lut515 + (lut4k != 0);
    int index = 0, value, tmp, slope_inv, power_inv, sor_inv;
    long long a, numerator, denominator, normalized, powered, powered_scaled;
    unsigned int norm, output;
    (void)x_min; (void)x_max;
    if (display->mode == 0) {
        common = param1->norm < param2->norm ? param1->norm : param2->norm;
        common3 = param3->norm < 31u ? param3->norm : 31u;
        do {
            unsigned int diff1 = param1->norm - common;
            value = fx_pow2_taylor(smpy_ss(slope_over_rolloff, tone_curve_xlog_table[index]), 23, 31);
            if (s32(diff1) > 31) diff1 = 31;
            numerator = add64((long long)param2->value * value >> ((param2->norm - common) & 63u),
                (long long)param1->value * (0x7fffffff >> (diff1 & 31u)));
            denominator = add64(add64((long long)value * param3->value >> ((param3->norm - common3) & 63u),
                lsl64(1, common3 + 31u)) >> (common3 & 63u), 1);
            a = div64(numerator, denominator);
            norm = unorm_sl(a);
            normalized = lsl64(a, norm) >> 32;
            if (normalized < 0) normalized = 0;
            value = 0;
            if ((int)normalized != 0) {
                tmp = fx_log2_taylor((int)normalized, 26);
                tmp = s32((unsigned int)tmp - ((norm + common - 32u) << 26));
                value = fx_pow2_taylor(s32((unsigned int)((long long)tmp * display->rolloff >> 33)), 24, 29);
            }
            value = smpy_ss(value, s32(trim_slope_one << 2));
            value = sadd_sss(value, trim_offset);
            if (value < 0) value = 0;
            if (value != 0) {
                tmp = s32((unsigned int)fx_log2_taylor(value, 26) - 0x70000000u);
                value = fx_pow2_taylor(sat32((long long)tmp * s32(trim_power_one) >> 30), 24, 31);
                if (value < 0) value = 0;
            }
            value = sadd_sss(value, 0x40000) >> 19;
            if (clamp_output) value = clamp(value & 65535, display->min_pq >> 19, display->max_pq >> 19);
            output = (unsigned int)value & 65535u;
            if (output < previous) output = previous;
            *out++ = narrow16(output);
            previous = output;
            index = s32((unsigned int)index + (unsigned int)step);
        } while (index < 4096);
    } else if (display->mode == 1) {
        common = param2->norm < param3->norm ? param2->norm : param3->norm;
        sor_inv = sdiv_sls(0x800000000000000ll, slope_over_rolloff);
        power_inv = sdiv_sls(0x100000000000000ll, s32(trim_power_one));
        slope_inv = sdiv_sls(0x100000000000000ll, s32(trim_slope_one));
        do {
            int large_index = 0;
            value = fx_pow2_taylor(smpy_ss(power_inv, tone_curve_xlog_table[index]), 23, 28);
            value = ssub_sss(value, trim_offset);
            value = s32((unsigned int)((long long)value * slope_inv >> 43) + 1u);
            tmp = value >> 1;
            if (tmp >= 4095) { tmp = value >> 2; large_index = 1; }
            if (tmp > 4095) tmp = 4095;
            powered = 0;
            powered_scaled = 0;
            if (tmp >= 3) {
                value = tone_curve_xlog_table[tmp];
                if (large_index) value = s32((unsigned int)value + 0x4000000u);
                powered = fx_pow2_taylor_int(s32((unsigned int)value * 3u), 26, 29);
                powered_scaled = lsl64(powered, 29);
            }
            denominator = sub64(lsl64(param2->value, 29) >> ((param2->norm - common) & 63u),
                mul64(param3->value, powered) >> ((param3->norm - common) & 63u));
            numerator = sub64(powered_scaled, lsl64(param1->value, 58u - param1->norm));
            norm = unorm_sl(denominator);
            tmp = s32((unsigned int)(lsl64(denominator, norm) >> 32) + 1u);
            value = sdiv_sls(numerator, tmp);
            value = sshr_sss(value, s32(30u - norm - common));
            if (value < 0) value = 0;
            output = 0;
            if (value != 0) {
                tmp = s32((unsigned int)fx_log2_taylor(value, 26) - 0x7c000000u);
                value = fx_pow2_taylor(s32((unsigned int)((long long)tmp * sor_inv >> 33)), 24, 31);
                if (value < 0) value = 0;
                output = (unsigned int)(sadd_sss(value, 0x40000) >> 19) & 65535u;
            }
            if (clamp_output) output = (unsigned int)clamp((int)output, display->min_pq >> 19, display->max_pq >> 19) & 65535u;
            if (output < previous) output = previous;
            *out++ = narrow16(output);
            previous = output;
            index = s32((unsigned int)index + (unsigned int)step);
        } while (index < 4096);
    } else {
        do {
            value = index;
            if (clamp_output) value = clamp(value, display->min_pq >> 19, display->max_pq >> 19);
            *out++ = narrow16((unsigned int)value);
            index = s32((unsigned int)index + (unsigned int)step);
        } while (index < 4096);
    }
    if (lut4k) {
        int group;
        lut515[0] = lut515[1];
        lut515[513] = lut515[512];
        lut515[514] = lut515[512];
        for (group = 0; group < 512; ++group) {
            const short *p = lut515 + group;
            short *q = lut4k + group * 8;
            q[0] = p[1];
            q[1] = narrow16((unsigned int)interpolate(p, -0x8e0, 0xec6b, 0x21cb, -0x556, (int)previous));
            q[2] = narrow16((unsigned int)interpolate(p, -0xe36, 0xd243, 0x461d, -0xa2a, (int)previous));
            q[3] = narrow16((unsigned int)interpolate(p, -0x1081, 0xb309, 0x6b71, -0xdf9, (int)previous));
            q[4] = narrow16((unsigned int)interpolate(p, -0x1041, 0x9041, 0x9041, -0x1041, (int)previous));
            q[5] = narrow16((unsigned int)interpolate(p, -0xdf9, 0x6b71, 0xb309, -0x1081, (int)previous));
            q[6] = narrow16((unsigned int)interpolate(p, -0xa2a, 0x461d, 0xd243, -0xe36, (int)previous));
            q[7] = narrow16((unsigned int)interpolate(p, -0x556, 0x21cb, 0xec6b, -0x8e0, (int)previous));
        }
    }
}
