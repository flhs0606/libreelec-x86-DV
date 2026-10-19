/* Fixed-point Taylor chain used by the control trim chroma distance.
 * Wrap and shift operations use explicit ARM64 integer widths. */
#include "trim_luminance.h"

static int s32(unsigned int x) { return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static long long s64(unsigned long long x)
{ return x <= 0x7fffffffffffffffull ? (long long)x : -1 - (long long)~x; }
static long long mul64(long long a, long long b)
{ return s64((unsigned long long)a * (unsigned long long)b); }
static long long add64(long long a, long long b)
{ return s64((unsigned long long)a + (unsigned long long)b); }
static long long lsl64(long long a, unsigned int n)
{ return s64((unsigned long long)a << (n & 63u)); }
static long long div64(long long a, long long b)
{ if (!b) return 0; if (a == (-0x7fffffffffffffffll - 1) && b == -1) return a; return a / b; }

void fx_assert(int test, const char *message) { (void)test; (void)message; }
unsigned int unorm_s(int value)
{
    unsigned int x, count = 0;
    if (!value) return 0;
    x = value < 0 ? (value == (-2147483647 - 1) ? 0x7fffffffu : 0u - (unsigned int)value) : (unsigned int)value;
    while (!(x & 0x80000000u)) { x <<= 1; ++count; }
    return count ? count - 1u : 0;
}
int smpy_ss(int first, int second)
{ return s32((unsigned int)((unsigned long long)((long long)first * second) >> 31)); }
int sshr_sss(int value, int shift)
{
    unsigned int amount;
    long long result;
    if (shift >= 0) return value >> (shift > 31 ? 31 : shift);
    amount = 0u - (unsigned int)shift;
    if (s32(amount) > 31) amount = 31;
    result = lsl64(value, amount);
    if (s32((unsigned int)((unsigned long long)result >> 32)) != (s32((unsigned int)result) >> 31))
        return value < 0 ? (-2147483647 - 1) : 2147483647;
    return s32((unsigned int)result);
}
int fx_log2_taylor(int value, int scale_out)
{
    unsigned int norm, shift = 30u - (unsigned int)scale_out;
    int a, result;
    if (!value) return s32(1u << ((unsigned int)scale_out & 31u));
    norm = unorm_s(value);
    a = s32((((unsigned int)value << (norm & 31u)) & 0x3fffffffu) << 1);
    result = smpy_ss(a, -0x19b9ec1);
    result = smpy_ss(a, s32((unsigned int)result + 0x7a2972du));
    result = smpy_ss(a, s32((unsigned int)result - 0x1193711au));
    result = smpy_ss(a, s32((unsigned int)result + 0x1d275dc5u));
    result = smpy_ss(a, s32((unsigned int)result - 0x2dec78c6u));
    result = smpy_ss(a, s32((unsigned int)result + 0x5c518466u));
    if (s32(shift) > 31) shift = 31;
    result = s32((unsigned int)result + 0x8e8u) >> (shift & 31u);
    return s32((unsigned int)result + ((30u - norm) << ((unsigned int)scale_out & 31u)));
}
int fx_pow2_taylor(int value, int scale_in, int scale_out)
{
    unsigned int a = ((unsigned int)value << ((32u - (unsigned int)scale_in) & 31u)) >> 1;
    int result, shift;
    if (scale_in > 31) scale_in = 31;
    result = smpy_ss((int)a, 0x29080);
    result = smpy_ss((int)a, s32((unsigned int)result + 0x164080u));
    result = smpy_ss((int)a, s32((unsigned int)result + 0x9d8f80u));
    result = smpy_ss((int)a, s32((unsigned int)result + 0x38d3180u));
    result = smpy_ss((int)a, s32((unsigned int)result + 0xf5fe000u));
    result = smpy_ss((int)a, s32((unsigned int)result + 0x2c5c8b80u));
    shift = s32(30u - (unsigned int)(value >> ((unsigned int)scale_in & 31u)) - (unsigned int)scale_out);
    return sshr_sss(s32((unsigned int)result + 0x40000000u), shift);
}
int pow_taylor(unsigned int value, unsigned int exponent)
{
    int l = fx_log2_taylor(s32(value), 26);
    long long product = mul64(s32((unsigned int)l - 0x7c000000u), (long long)exponent);
    return fx_pow2_taylor(s32((unsigned int)(product >> 31)), 26, 31);
}
unsigned long long pow_in_s_taylor(long long value, long long exponent)
{
    int l = fx_log2_taylor(s32((unsigned int)(value >> ((unsigned long long)exponent & 63u))), 26);
    int p = fx_pow2_taylor(s32((unsigned int)(mul64(s32((unsigned int)l - 0x7c000000u), 0x14640000) >> 31)), 26, 30);
    int scale = fx_pow2_taylor(0x14640000, 31, 27), i;
    /* Use ASR for the first product, then logical LSR in its loop. */
    unsigned long long result = (unsigned long long)(((long long)p * scale) >> 27);
    for (i = 2; i < s32((unsigned int)exponent); ++i)
        result = (result * (unsigned long long)(long long)scale) >> 27;
    return ((unsigned long long)(long long)scale * result) >> 26;
}
int l2_pq_x(long long linear)
{
    unsigned long long magnitude = linear < 0 ? 0ull - (unsigned long long)linear : (unsigned long long)linear;
    /* LSL followed by signed magic division implements signed /10000. */
    long long input = div64(s64(magnitude << 7), 10000), powered, denominator, numerator, result;
    int t, exponent;
    unsigned int i;
    if (input < 0x80000000ll) powered = pow_taylor((unsigned int)input, 0x14640000u);
    else if (input < 0x200000000ll) {
        t = fx_log2_taylor(s32((unsigned int)((unsigned long long)input >> 2)), 26);
        t = fx_pow2_taylor(s32((unsigned int)(mul64(s32((unsigned int)t - 0x7c000000u), 0x14640000) >> 31)), 26, 30);
        exponent = fx_pow2_taylor(0x14640000, 31, 29);
        powered = ((long long)exponent * s32((unsigned int)(((long long)t * exponent) >> 29))) >> 28;
    } else {
        if (input < 0x800000000ll) exponent = 4;
        else if (input < 0x2000000000ll) exponent = 6;
        else if (input < 0x8000000000ll) exponent = 8;
        else if (input < 0x20000000000ll) exponent = 10;
        else if (input < 0x80000000000ll) exponent = 12;
        else if (input < 0x200000000000ll) exponent = 14;
        else if (input < 0x800000000000ll) exponent = 16;
        else { t = s32(0x8864a180u); goto final_power; }
        powered = s64(pow_in_s_taylor(input, exponent));
    }
    denominator = add64(mul64(powered, 0x4ac00000) >> 26, 0x80000000ll) >> 8;
    numerator = add64(mul64(powered, 0x4b680000) >> 26, 0x6b000000ll) >> 8;
    t = s32((unsigned int)div64(lsl64(numerator, 23), denominator) << 7);
final_power:
    t = fx_log2_taylor(t, 26);
    t = fx_pow2_taylor(s32((unsigned int)(mul64(s32((unsigned int)t - 0x7c000000u), 0x4ed80000) >> 31)), 26, 30);
    exponent = fx_pow2_taylor(0x4ed80000, 31, 27);
    result = ((long long)t * exponent) >> 27;
    result = mul64(result, result) >> 29;
    for (i = 0; i < 6; ++i) result = mul64(result, result) >> 31;
    if (linear < 0) result = s64(0x620ull - (unsigned long long)result);
    return s32((unsigned int)(((unsigned long long)result + 0x1000ull) >> 13));
}
