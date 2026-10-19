#ifndef DOVI_TRIM_LUMINANCE_H
#define DOVI_TRIM_LUMINANCE_H
unsigned int unorm_s(int value);
int smpy_ss(int first, int second);
int sshr_sss(int value, int shift);
void fx_assert(int test, const char *message);
int fx_log2_taylor(int value, int scale_out);
int fx_pow2_taylor(int value, int scale_in, int scale_out);
int pow_taylor(unsigned int value, unsigned int exponent);
unsigned long long pow_in_s_taylor(long long value, long long exponent);
int l2_pq_x(long long linear);
#endif
