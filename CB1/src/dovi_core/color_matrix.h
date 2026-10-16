#ifndef DOVI_COLOR_MATRIX_H
#define DOVI_COLOR_MATRIX_H

/* Flat row-major arrays: 9 signed shorts (M33), 3 signed ints (V3).
 * Scales and returns are signed 16-bit values, except explicit int arguments.
 * Keep these scalar widths when adapting callers. */
void assign_v3_i2_i(const int *input, int *output);
void assign_m33_i2_i(const short *input, short *output);
short inv_m33(const short *input, short *output, short input_scale, short output_scale);
void yuv2_rgb_offset_rgb_fxp(const short *matrix, unsigned int y_offset, unsigned int uv_offset, int *output);
void get_yuv_rgb_off_fxp(unsigned int range, int bits, int *output);
short get_lms2_ipt_m33_fxp(short *output);
short get_lms2_ipt_m33_narrow_fxp(short bits, short *output);
short get_ipt2_lms_m33_fxp(short *output);
short get_ipt2_lms_m33_narrow_fxp(short bits, short *output);
short get_rgb2_yuv_m33_fxp(unsigned int definition, short *output);
short get_rgb2_yuv_m33_narrow_fxp(unsigned int definition, int bits, short *output);
short get_yuv2_rgb_m33_fxp(unsigned int definition, short *output);
short get_yuv2_rgb_m33_narrow_fxp(unsigned int definition, int bits, short *output);
short get_rgb2_lms_by_def_m33(unsigned int definition, short *output);
short get_rgb2_lms_by_prims_m33_fxp(int rx, int ry, int gx, int gy, int bx, int by, int wx, int wy, short *output);
short get_rgb2_lms_ct_wp_m33_fxp(const short *matrix, short matrix_scale, int cross_talk, const int *white_point, short white_point_scale, short *output);
short get_lms2_rgb_by_def_m33_fxp(unsigned int definition, short *output);
short get_lms2_rgb_by_prims_m33_fxp(int rx, int ry, int gx, int gy, int bx, int by, int wx, int wy, short *output);
short get_lms2_rgb_ct_wp_m33_fxp(const short *matrix, short matrix_scale, int cross_talk, const int *white_point, short white_point_scale, short *output);

/* m33_times_m33_as takes Q26 RGB->XYZ input and an output pointer.
 * Its XYZ->LMS operand is constant. */
short find_int64_m33_precision(const long long *input);
short adjust64_m33_scale_to16_m33(const long long *input, short scale, short *output);
short m33_times_m33_as(const int *input, short *output);
short rgb2_yuv_m33_full2_narrow(const short *input, short scale, int bits, short *output);
short yuv2_rgb_m33_narrow2_full(const short *input, short scale, short bits, short *output);
void get_rgb2_xyz_m33(unsigned int definition, int *output);
void get_yuv2_rgb_from_kr_kg_kb_m33(int kr, int kg, int kb, long long *output);

#endif
