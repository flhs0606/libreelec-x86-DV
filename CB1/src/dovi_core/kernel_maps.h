#ifndef DOVI_KERNEL_MAPS_H
#define DOVI_KERNEL_MAPS_H

/* Buffers use the layouts in dm_commit_abi.h. Every operation in a
 * supplied color callback table is required. */
typedef struct KernelMapOps {
    void (*assign_matrix)(const short *, short *);
    void (*assign_vector)(const int *, int *);
    short (*lms_to_rgb_definition)(unsigned int, short *);
    short (*lms_to_rgb_primaries)(int,int,int,int,int,int,int,int,short *);
    short (*lms_to_rgb_white_point)(const short *,short,int,const int *,short,short *);
    short (*lms_to_ipt)(short *);
    short (*lms_to_ipt_narrow)(short,short *);
    short (*rgb_to_yuv)(unsigned int,short *);
    short (*rgb_to_yuv_narrow)(unsigned int,int,short *);
    void (*yuv_rgb_offset)(unsigned int,int,int *);
    short (*ipt_to_lms)(short *);
    short (*ipt_to_lms_narrow)(short,short *);
    void (*yuv_to_rgb_offset)(const short *,unsigned int,unsigned int,int *);
} KernelMapOps;

typedef struct KernelTrimOps {
    void (*assertion)(const char *source,const char *function,int line,const char *condition);
    void (*dump_stack)(void);
} KernelTrimOps;

void sig_env_to_ks_frame_format(const void *signal,void *format);
void get_range(int bits,unsigned int range,unsigned short *minimum,unsigned short *extent);
void get_ipt_norm_param_with_ops(int bits,unsigned int range,int *offset,int *scale,const KernelMapOps *ops);
void commit_ipt_ks_input_map_with_ops(unsigned int bits,unsigned int range,void *map,const KernelMapOps *ops);
int commit_ks_output_map_with_ops(const void *config,int update_target_minmax,void *dm,const KernelMapOps *ops);
/* The bit-depth argument points to a readable 32-bit scalar. */
void commit_us(unsigned int chroma,const int *bits,void *output);
void clamp_level2(int *level2);
int commit_trim_with_ops(unsigned short target_max_pq,void *extensions,void *dm,void *tone_control,const KernelTrimOps *ops);

/* Concrete bindings use color_matrix.c. Trim diagnostics stay explicit. */
const KernelMapOps *kernel_map_color_ops(void);
void get_ipt_norm_param(int bits,unsigned int range,int *offset,int *scale);
void commit_ipt_ks_input_map(unsigned int bits,unsigned int range,void *map);
int commit_ks_output_map(const void *config,int update_target_minmax,void *dm);

#endif
