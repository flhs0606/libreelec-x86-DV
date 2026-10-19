#ifndef DOVI_TONE_MATH_H
#define DOVI_TONE_MATH_H

unsigned int unorm_sl(long long value);
unsigned long long uabs_ls(long long value);
int sadd_sss(int first, int second);
int ssub_sss(int first, int second);
int sshl_ssu(int value, int shift);
int sdiv_sls(long long numerator, int denominator);
unsigned int unorm_u(unsigned int value);
unsigned int ssqrt_s(unsigned int value);

#endif
