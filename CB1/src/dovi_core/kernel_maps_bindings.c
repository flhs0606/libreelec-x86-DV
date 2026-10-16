#include "kernel_maps.h"
#include "color_matrix.h"

static const KernelMapOps color_ops={
    assign_m33_i2_i,assign_v3_i2_i,get_lms2_rgb_by_def_m33_fxp,get_lms2_rgb_by_prims_m33_fxp,
    get_lms2_rgb_ct_wp_m33_fxp,get_lms2_ipt_m33_fxp,get_lms2_ipt_m33_narrow_fxp,
    get_rgb2_yuv_m33_fxp,get_rgb2_yuv_m33_narrow_fxp,get_yuv_rgb_off_fxp,
    get_ipt2_lms_m33_fxp,get_ipt2_lms_m33_narrow_fxp,yuv2_rgb_offset_rgb_fxp
};
const KernelMapOps *kernel_map_color_ops(void){return &color_ops;}
void get_ipt_norm_param(int bits,unsigned int range,int *offset,int *scale)
{get_ipt_norm_param_with_ops(bits,range,offset,scale,&color_ops);}
void commit_ipt_ks_input_map(unsigned int bits,unsigned int range,void *map)
{commit_ipt_ks_input_map_with_ops(bits,range,map,&color_ops);}
int commit_ks_output_map(const void *config,int update_target_minmax,void *dm)
{return commit_ks_output_map_with_ops(config,update_target_minmax,dm,&color_ops);}
