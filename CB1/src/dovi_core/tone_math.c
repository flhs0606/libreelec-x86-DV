/* Fixed-point scalar arithmetic for tone-curve generation. */
#include "tone_math.h"
#include "trim_luminance.h"

static int s32(unsigned int x) { return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static long long s64(unsigned long long x)
{ return x <= 0x7fffffffffffffffull ? (long long)x : -1 - (long long)~x; }

unsigned long long uabs_ls(long long value)
{
    if (value >= 0) return (unsigned long long)value;
    if (value == (-0x7fffffffffffffffll - 1)) return 0x7fffffffffffffffull;
    return 0ull - (unsigned long long)value;
}
unsigned int unorm_sl(long long value)
{
    unsigned long long bits = uabs_ls(value);
    unsigned int count = 0;
    if (!bits) return 0;
    while (!(bits & 0x8000000000000000ull)) { bits <<= 1; ++count; }
    return count ? count - 1u : 0;
}
unsigned int unorm_u(unsigned int value)
{
    unsigned int count = 0;
    if (!value || (value & 0x80000000u)) return 0;
    while (!(value & 0x80000000u)) { value <<= 1; ++count; }
    return count ? count - 1u : 0;
}
int sadd_sss(int first, int second)
{
    unsigned int x = (unsigned int)first, y = (unsigned int)second;
    unsigned int result = x + y, limit = 0x7fffffffu + (x >> 31);
    return s32(((limit ^ y) | ~(result ^ y)) & 0x80000000u ? result : limit);
}
int ssub_sss(int first, int second)
{
    unsigned int x = (unsigned int)first, y = (unsigned int)second;
    unsigned int result = x - y, limit = 0x7fffffffu + (x >> 31);
    return s32(((limit ^ result) & (limit ^ y)) & 0x80000000u ? limit : result);
}
int sshl_ssu(int value, int shift)
{
    unsigned long long result;
    int low, high;
    if (shift > 31) shift = 31;
    result = (unsigned long long)(long long)value << ((unsigned int)shift & 63u);
    low = s32((unsigned int)result);
    high = s32((unsigned int)(result >> 32));
    return high == (low >> 31) ? low : (value < 0 ? (-2147483647 - 1) : 2147483647);
}
int sdiv_sls(long long numerator, int denominator)
{
    long long quotient;
    if (!denominator) return 0;
    if (numerator == (-0x7fffffffffffffffll - 1) && denominator == -1)
        return (-2147483647 - 1);
    quotient = numerator / denominator;
    if (quotient > 2147483647ll) return 2147483647;
    if (quotient < -2147483648ll) return (-2147483647 - 1);
    return (int)quotient;
}
unsigned int ssqrt_s(unsigned int value)
{
    unsigned int norm = unorm_u(value), a = value << (norm & 31u);
    int square, cube, fourth, output;
    unsigned int shift = (norm >> 1) + 30u;
    unsigned long long accumulator = (unsigned long long)a * 0x52aa9432u + 0x0762247a00000000ull;
    long long result;
    fx_assert((int)(~norm >> 31), "sshl_su() called with negative shift value");
    square = smpy_ss(s32(a), s32(a));
    accumulator += (unsigned long long)((long long)square * -0x390c4115);
    cube = smpy_ss(square, s32(a));
    accumulator += (unsigned long long)((long long)cube * 0x1f11c934);
    fourth = smpy_ss(cube, s32(a));
    accumulator += (unsigned long long)((long long)fourth * -0x7748f4c);
    fx_assert((int)(~shift >> 31), "sshr_slu() called with negative shift value");
    if (shift > 63u) shift = 63u;
    result = s64(accumulator) >> (shift & 63u);
    output = result > 2147483647ll ? 2147483647 :
             result < (-2147483647ll - 1) ? (-2147483647 - 1) : (int)result;
    if (norm & 1u) output = smpy_ss(output, 0x5a82799a);
    return (unsigned int)output;
}
