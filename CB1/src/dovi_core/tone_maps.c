/* Auxiliary tone/saturation LUTs; exact integer widths and wrap follow ARM64. */
#include "tone_maps.h"
#include "trim_luminance.h"
#include "tone_maps_table.inc"

static int s32(unsigned int x) { return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static short s16(unsigned int x)
{ x &= 65535u; return x <= 32767u ? (short)x : (short)(-1 - (int)(~x & 65535u)); }
static long long s64(unsigned long long x)
{ return x <= 0x7fffffffffffffffull ? (long long)x : -1 - (long long)~x; }

static int vector_map(int weight, int ratio, unsigned short mode, short *output, short count)
{
    unsigned int norm, shift, accumulator = 0;
    int step, sign = mode == 0 ? 1 : -1, i;
    fx_assert(1, " QFTMS must be <= 15");
    norm = unorm_s(weight);
    step = sshr_sss(weight, s32(9u - norm));
    if (mode > 1) {
        for (i = 0; i < count; ++i) output[i] = 4095;
        return 0;
    }
    shift = norm + 33u;
    if (shift > 63u) shift = 63u;
    for (i = 0; i < count; ++i) {
        long long scaled = ((long long)s32(accumulator) * ratio) >> (shift & 63u);
        int half = -0x40000000, value;
        accumulator += (unsigned int)step;
        fx_assert((int)(~(norm + 33u) >> 31), "sshr_slu() called with negative shift value");
        if (scaled <= 2147483647ll && scaled >= (-2147483647ll - 1))
            half = s32((unsigned int)scaled + 1u) >> 1;
        value = s32(4095u - (unsigned int)sign * (unsigned int)half);
        if (value > 8191) value = 8191;
        if (value < 0) value = 0;
        output[i] = (short)value;
    }
    return 0;
}
int tone_map_s(int weight, int ratio, unsigned short mode, short *output, short count)
{ return vector_map(weight, ratio, mode, output, count); }
int saturation_map_s(int weight, int ratio, unsigned short mode, short *output, short count)
{ return vector_map(weight, ratio, mode, output, count); }

int saturation_map_i(int minimum, int maximum, int source_minimum, int gain,
                     short *tone, unsigned short mode, short *output, short count)
{
    int shift = count == 256 ? 4 : 3, i;
    if (!mode) {
        int low = minimum >> 19, high = maximum >> 19;
        int delta = low - (source_minimum >> 19);
        unsigned int reciprocal = low < high ? 0x20000000u / (unsigned int)(high - low) : 0x20000000u;
        for (i = 0; i < count; ++i) {
            int tone_value = tone[i], grid = s16((unsigned int)i << shift);
            long long value = ((long long)(tone_value - grid + 4096) * gain) >> 28;
            short result = 0;
            if (delta > 0) {
                long long distance = high - tone_value, correction;
                unsigned long long lookup, corrected;
                unsigned int index, linear;
                if (distance < 0) distance = 0;
                lookup = (unsigned long long)(((long long)s32(reciprocal) * distance + 8192) >> 14);
                index = (unsigned int)(lookup >> 6) & 1023u;
                linear = tone_maps_depq_lut[index];
                if (index != 1023u) {
                    unsigned int difference = tone_maps_depq_lut[index + 1u] - linear;
                    linear += (unsigned int)(s64((unsigned long long)difference * (lookup & 63u)) >> 6);
                }
                corrected = (unsigned long long)((long long)delta * 3) * linear;
                correction = s64(corrected * 0x2c6734cull + 0x2000000000000ull) >> 50;
                if (correction > 4095) { output[i] = 0; continue; }
                value = s64(((unsigned long long)value * (4096ull - (unsigned long long)correction) + 2048ull) >> 12);
            }
            result = s32((unsigned int)value) <= 8191 ? s16((unsigned int)value) : 8191;
            output[i] = result;
        }
    } else if (mode == 1) {
        for (i = 0; i < count; ++i) {
            int denominator = (int)s16((unsigned int)i << shift) - tone[i] + 4096;
            int reciprocal = denominator ? 0x1000000 / denominator : 0;
            int value = s32((unsigned int)(((long long)reciprocal * gain) >> 28));
            if (value < 0) value = 0;
            if (value > 8191) value = 8191;
            output[i] = (short)value;
        }
    } else {
        for (i = 0; i < count; ++i) output[i] = 4095;
    }
    return 0;
}
