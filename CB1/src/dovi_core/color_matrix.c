/* Fixed-point color transforms and matrix arithmetic. */
#include "color_matrix.h"
#include "color_matrix_tables.inc"

static int s32(unsigned int x) { return x <= 0x7fffffffu ? (int)x : -1 - (int)~x; }
static short s16(unsigned int x) { x &= 65535u; return (short)(x <= 32767u ? (int)x : (int)x - 65536); }
static long long s64(unsigned long long x)
{ return x <= 0x7fffffffffffffffull ? (long long)x : -1 - (long long)~x; }
static long long add64(long long a, long long b)
{ return s64((unsigned long long)a + (unsigned long long)b); }
static long long sub64(long long a, long long b)
{ return s64((unsigned long long)a - (unsigned long long)b); }
static long long mul64(long long a, long long b)
{ return s64((unsigned long long)a * (unsigned long long)b); }
static long long lsl64(long long a, unsigned int n)
{ return s64((unsigned long long)a << (n & 63u)); }
static long long div64(long long a, long long b)
{ if (!b) return 0; if (a == (-0x7fffffffffffffffll-1) && b == -1) return a; return a / b; }
static int div32(int a, int b)
{ if (!b) return 0; if (a == (-2147483647-1) && b == -1) return a; return a / b; }
static int sub32(int a, int b) { return s32((unsigned int)a - (unsigned int)b); }

/* Byte accesses retain legal unaligned storage and alias reloads across widths. */
static unsigned long long load(const void *p, unsigned int index, unsigned int width)
{
    const unsigned char *bytes = (const unsigned char *)p + index * width;
    unsigned int i;
    unsigned long long result = 0;
    for (i = 0; i < width; ++i) result |= (unsigned long long)bytes[i] << (i * 8u);
    return result;
}
static void store(void *p, unsigned int index, unsigned int width, unsigned long long value)
{
    unsigned char *bytes = (unsigned char *)p + index * width;
    unsigned int i;
    for (i = 0; i < width; ++i) bytes[i] = (unsigned char)(value >> (i * 8u));
}
static int h(const short *p, unsigned int i) { return s16((unsigned int)load(p,i,2)); }
static int w(const int *p, unsigned int i) { return s32((unsigned int)load(p,i,4)); }
static long long x(const long long *p, unsigned int i) { return s64(load(p,i,8)); }
static void ph(short *p, unsigned int i, long long v) { store(p,i,2,(unsigned long long)v); }
static void pw(int *p, unsigned int i, long long v) { store(p,i,4,(unsigned long long)v); }
static void px(long long *p, unsigned int i, long long v) { store(p,i,8,(unsigned long long)v); }

void assign_v3_i2_i(const int *input, int *output)
{ unsigned int i; for (i = 0; i < 3; ++i) pw(output,i,w(input,i)); }
void assign_m33_i2_i(const short *input, short *output)
{ unsigned int i; for (i = 0; i < 9; ++i) ph(output,i,h(input,i)); }

short find_int64_m33_precision(const long long *input)
{
    unsigned int i;
    long long maximum = 0;
    for (i = 0; i < 9; ++i) {
        long long value = x(input,i);
        if (value == (-0x7fffffffffffffffll-1)) value = 0x7fffffffffffffffll;
        else if (value < 0) value = -value;
        if (value > maximum) maximum = value;
    }
    for (i = 0; i < 64; ++i)
        if (s64(1ull << i) >= maximum) return (short)i;
    return 64;
}
short adjust64_m33_scale_to16_m33(const long long *input, short scale, short *output)
{
    short precision = find_int64_m33_precision(input);
    unsigned int i, shift = precision < 16 ? 0u : (unsigned int)(precision-15);
    for (i = 0; i < 9; ++i) ph(output,i,x(input,i) >> (shift & 63u));
    return precision < 16 ? scale : s16((unsigned int)scale + 15u - (unsigned int)precision);
}
short m33_times_m33_as(const int *input, short *output)
{
    long long temporary[9];
    unsigned int row,col;
    for (row = 0; row < 3; ++row) {
        int a = xyz2_lms[row*3], b = xyz2_lms[row*3+1], c = xyz2_lms[row*3+2];
        for (col = 0; col < 3; ++col)
            temporary[row*3+col] = add64(add64((long long)w(input,col+3)*b,
                (long long)w(input,col)*a),(long long)w(input,col+6)*c) >> 23;
    }
    return adjust64_m33_scale_to16_m33(temporary,31,output);
}
short inv_m33(const short *input, short *output, short input_scale, short output_scale)
{
    long long determinant = 0;
    int temporary[9], positive = 0, negative = -1;
    unsigned int i,j,shift = 0;
    for (i = 0; i < 3; ++i) {
        unsigned int a = (i+1)%3, b = (i+2)%3;
        long long minor = (long long)h(input,3+a)*h(input,6+b) - (long long)h(input,3+b)*h(input,6+a);
        determinant += (long long)h(input,i)*minor;
    }
    if (!determinant) return -1;
    for (i = 0; i < 3; ++i) for (j = 0; j < 3; ++j) {
        unsigned int a = (i+1)%3, b = (i+2)%3, c = (j+1)%3, d = (j+2)%3;
        long long minor = (long long)h(input,a*3+c)*h(input,b*3+d) - (long long)h(input,a*3+d)*h(input,b*3+c);
        long long quotient = div64(lsl64(minor,(unsigned int)((int)input_scale + output_scale + 3)),determinant);
        int value = s32((unsigned int)(add64(quotient >> 2,1) >> 1));
        temporary[j*3+i] = value;
        if (value > positive) positive = value;
        if (value < negative) negative = value;
    }
    while (negative < -32768 || positive > 32767) { negative >>= 1; positive >>= 1; ++shift; }
    for (i = 0; i < 9; ++i) {
        int value = temporary[i];
        if (shift) value = s32((unsigned int)(value >> ((shift-1u)&31u)) + 1u) >> 1;
        ph(output,i,value);
    }
    return (short)shift;
}

void yuv2_rgb_offset_rgb_fxp(const short *matrix, unsigned int y_offset, unsigned int uv_offset, int *output)
{
    unsigned int i;
    for (i = 0; i < 3; ++i) {
        long long value = (long long)(h(matrix,i*3+1)+h(matrix,i*3+2))*uv_offset + (long long)h(matrix,i*3)*y_offset;
        pw(output,i,value >> 16);
    }
}
void get_yuv_rgb_off_fxp(unsigned int range, int bits, int *output)
{
    unsigned int uv = 1u << (((unsigned int)bits-1u)&31u);
    pw(output,0,range ? 0 : 1u << (((unsigned int)bits-4u)&31u));
    pw(output,2,uv); pw(output,1,uv);
}
short rgb2_yuv_m33_full2_narrow(const short *input, short scale, int bits, short *output)
{
    int y = 0xdbdb, uv = 0xe0e0;
    unsigned int i;
    long long temporary[9];
    if (bits >= 8) {
        int denominator = s32((1u << ((unsigned int)bits&31u))-1u);
        y = div32(s32(0xdb0000u << (((unsigned int)bits-8u)&31u)),denominator);
        uv = div32(s32(0xe00000u << (((unsigned int)bits-8u)&31u)),denominator);
    }
    for (i = 0; i < 9; ++i) temporary[i] = (long long)h(input,i)*(i < 3 ? y : uv);
    return adjust64_m33_scale_to16_m33(temporary,s16((unsigned int)scale+16u),output);
}
short yuv2_rgb_m33_narrow2_full(const short *input, short scale, short bits, short *output)
{
    unsigned int i,shift = ((unsigned int)(int)bits-8u)&31u;
    int numerator = s32(((1u << ((unsigned int)(int)bits&31u))-1u)*0x10000u);
    int y = div32(numerator,s32(0xdbu << shift));
    int uv = div32(numerator,s32(0xe0u << shift));
    long long temporary[9];
    for (i = 0; i < 9; ++i) temporary[i] = (long long)h(input,i)*(i%3 ? uv : y);
    return adjust64_m33_scale_to16_m33(temporary,s16((unsigned int)scale+16u),output);
}
short get_lms2_ipt_m33_fxp(short *output) { assign_m33_i2_i(lms2_ipt,output); return 12; }
short get_lms2_ipt_m33_narrow_fxp(short bits, short *output)
{ return rgb2_yuv_m33_full2_narrow(lms2_ipt,12,bits,output); }
short get_ipt2_lms_m33_fxp(short *output) { assign_m33_i2_i(ipt2_lms,output); return 15; }
short get_ipt2_lms_m33_narrow_fxp(short bits, short *output)
{ return yuv2_rgb_m33_narrow2_full(ipt2_lms,15,bits,output); }

void get_rgb2_xyz_m33(unsigned int definition, int *output)
{
    unsigned int i;
    for (i = 0; i < 9; ++i) pw(output,i,definition < 7 ? rgb2_xyz[definition][i] : 0);
}
void get_yuv2_rgb_from_kr_kg_kb_m33(int kr, int kg, int kb, long long *output)
{
    int rr = sub32(0x4000000,kr), bb = sub32(0x4000000,kb);
    px(output,0,0x4000000); px(output,3,0x4000000); px(output,1,0);
    px(output,2,s32((unsigned int)rr*2u));
    if (kg) {
        px(output,5,sub64(0,div64(mul64((long long)kr*2,rr),kg)));
        px(output,4,sub64(0,div64(mul64((long long)kb*2,bb),kg)));
    }
    px(output,8,0); px(output,6,0x4000000); px(output,7,s32((unsigned int)bb*2u));
}
short get_rgb2_yuv_m33_fxp(unsigned int definition, short *output)
{
    long long temporary[9];
    if (definition < 6) {
        int matrix[9],rr,bb;
        get_rgb2_xyz_m33(definition,matrix);
        temporary[0] = matrix[3]; temporary[1] = matrix[4]; temporary[2] = matrix[5];
        bb = sub32(0x4000000,matrix[5]); rr = sub32(0x4000000,matrix[3]);
        temporary[3] = -div64(lsl64(matrix[3] >> 1,26),bb);
        temporary[4] = -div64(lsl64(matrix[4] >> 1,26),bb);
        temporary[8] = -div64(lsl64(matrix[5] >> 1,26),rr);
        temporary[7] = -div64(lsl64(matrix[4] >> 1,26),rr);
    } else {
        temporary[0] = 0x1322d0e; temporary[1] = 0x2591687; temporary[2] = 0x74bc6a;
        temporary[3] = -0xacc919; temporary[4] = -0x15336e5;
        temporary[7] = -0x1acbc6f; temporary[8] = -0x53438f;
    }
    temporary[5] = 0x2000000; temporary[6] = 0x2000000;
    return adjust64_m33_scale_to16_m33(temporary,26,output);
}
short get_rgb2_yuv_m33_narrow_fxp(unsigned int definition, int bits, short *output)
{ short full[9], scale = get_rgb2_yuv_m33_fxp(definition,full); return rgb2_yuv_m33_full2_narrow(full,scale,bits,output); }
short get_yuv2_rgb_m33_fxp(unsigned int definition, short *output)
{
    long long temporary[9];
    if (definition > 5) {
        temporary[0] = 0x4000000; temporary[1] = 0; temporary[2] = 0x59ba5e4;
        temporary[3] = 0x4000000; temporary[4] = -0x1606542; temporary[5] = -0x2db468a;
        temporary[6] = 0x4000000; temporary[7] = 0x716872c; temporary[8] = 0;
    } else {
        int matrix[9]; get_rgb2_xyz_m33(definition,matrix);
        get_yuv2_rgb_from_kr_kg_kb_m33(matrix[3],matrix[4],matrix[5],temporary);
    }
    return adjust64_m33_scale_to16_m33(temporary,26,output);
}
short get_yuv2_rgb_m33_narrow_fxp(unsigned int definition, int bits, short *output)
{ short full[9], scale = get_yuv2_rgb_m33_fxp(definition,full); return yuv2_rgb_m33_narrow2_full(full,scale,s16((unsigned int)bits),output); }
short get_rgb2_lms_by_def_m33(unsigned int definition, short *output)
{ int matrix[9]; get_rgb2_xyz_m33(definition,matrix); return m33_times_m33_as(matrix,output); }
short get_lms2_rgb_by_def_m33_fxp(unsigned int definition, short *output)
{ short scale = get_rgb2_lms_by_def_m33(definition,output); return s16((unsigned int)scale-(unsigned int)inv_m33(output,output,scale,scale)); }
short get_rgb2_lms_by_prims_m33_fxp(int rx, int ry, int gx, int gy, int bx, int by, int wx, int wy, short *output)
{
    int matrix[9], u,v,z, factors[3],coords[6];
    int rb = sub32(rx,bx), gb = sub32(gx,bx), rby = sub32(ry,by), gby = sub32(gy,by);
    int d = s32((unsigned int)(sub64((long long)rb*gby,(long long)rby*gb) >> 26));
    unsigned int i;
    if (!d) { u = 0; v = 0; z = 0x4000000; }
    else {
        u = s32((unsigned int)div64(sub64((long long)gby*sub32(wx,bx),(long long)gb*sub32(wy,by)),d));
        v = s32((unsigned int)div64(sub64((long long)rb*sub32(wy,by),(long long)rby*sub32(wx,bx)),d));
        z = sub32(sub32(0x4000000,u),v);
    }
    factors[0] = s32((unsigned int)div64(lsl64(u,26),wy));
    factors[1] = s32((unsigned int)div64(lsl64(v,26),wy));
    factors[2] = s32((unsigned int)div64(lsl64(z,26),wy));
    coords[0]=rx;coords[1]=ry;coords[2]=gx;coords[3]=gy;coords[4]=bx;coords[5]=by;
    for (i=0;i<3;++i) {
        matrix[i] = s32((unsigned int)((unsigned long long)((long long)factors[i]*coords[i*2]) >> 26));
        matrix[3+i] = s32((unsigned int)((unsigned long long)((long long)factors[i]*coords[i*2+1]) >> 26));
        matrix[6+i] = s32((unsigned int)((unsigned long long)((long long)factors[i]*sub32(sub32(0x4000000,coords[i*2]),coords[i*2+1])) >> 26));
    }
    return m33_times_m33_as(matrix,output);
}
short get_lms2_rgb_by_prims_m33_fxp(int rx, int ry, int gx, int gy, int bx, int by, int wx, int wy, short *output)
{
    short scale = get_rgb2_lms_by_prims_m33_fxp(rx,ry,gx,gy,bx,by,wx,wy,output);
    return s16((unsigned int)scale-(unsigned int)inv_m33(output,output,scale,scale));
}
short get_rgb2_lms_ct_wp_m33_fxp(const short *matrix, short matrix_scale, int cross_talk, const int *white_point, short white_point_scale, short *output)
{
    unsigned int i,j;
    int factor = s32(0x8000u - (unsigned int)cross_talk*3u);
    long long columns[3],temporary[9];
    for (j=0;j<3;++j) columns[j] = (long long)(h(matrix,j)+h(matrix,j+3)+h(matrix,j+6))*cross_talk;
    for (i=0;i<3;++i) {
        int white = w(white_point,i);
        for(j=0;j<3;++j) temporary[i*3+j] = div64(lsl64(add64(columns[j],(long long)factor*h(matrix,i*3+j)),16),white);
    }
    return adjust64_m33_scale_to16_m33(temporary,s16((unsigned int)matrix_scale+31u-(unsigned int)white_point_scale),output);
}
short get_lms2_rgb_ct_wp_m33_fxp(const short *matrix, short matrix_scale, int cross_talk, const int *white_point, short white_point_scale, short *output)
{
    unsigned int i,j;
    int factor = s32(0x8000u - (unsigned int)cross_talk*3u);
    long long rows[3],temporary[9];
    for (i=0;i<3;++i) rows[i] = (long long)(h(matrix,i*3)+h(matrix,i*3+1)+h(matrix,i*3+2))*cross_talk;
    for (i=0;i<3;++i) for(j=0;j<3;++j) {
        if (!factor) temporary[i*3+j] = 0;
        else {
            long long value = div64(lsl64(sub64((long long)h(matrix,i*3+j)*0x8000,rows[i]),16),factor);
            temporary[i*3+j] = mul64(w(white_point,j),value);
        }
    }
    return adjust64_m33_scale_to16_m33(temporary,s16((unsigned int)matrix_scale+16u+(unsigned int)white_point_scale),output);
}
