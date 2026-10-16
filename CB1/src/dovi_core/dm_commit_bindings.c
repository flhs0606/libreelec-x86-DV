/* Bind DM commit operations to color, tone, map and trim helpers. */
#include "dm_commit_bindings.h"
#include "dm_commit.h"
#include "color_matrix.h"
#include "trim_luminance.h"

static short lms_to_ipt(void *opaque,void *output)
{(void)opaque;return get_lms2_ipt_m33_fxp(output);}
static short ipt_to_lms(void *opaque,void *output)
{(void)opaque;return get_ipt2_lms_m33_fxp(output);}
static short invert_matrix(void *opaque,const void *input,void *output,short input_scale,short output_scale)
{(void)opaque;return inv_m33(input,output,input_scale,output_scale);}
static void frame_format(void *opaque,const void *signal,void *format)
{(void)opaque;sig_env_to_ks_frame_format(signal,format);}
static void ipt_input(void *opaque,unsigned int bits,unsigned int range,void *output)
{(void)opaque;commit_ipt_ks_input_map(bits,range,output);}
static void assign_matrix(void *opaque,const void *input,void *output)
{(void)opaque;assign_m33_i2_i(input,output);}
static void assign_vector(void *opaque,const void *input,void *output)
{(void)opaque;assign_v3_i2_i(input,output);}
static void rgb_offset(void *opaque,const void *matrix,int y,int uv,void *output)
{(void)opaque;yuv2_rgb_offset_rgb_fxp(matrix,(unsigned int)y,(unsigned int)uv,output);}
static short rgb_to_lms_crosstalk(void *opaque,const void *matrix,short matrix_scale,int cross_talk,const void *white_point,short white_point_scale,void *output)
{(void)opaque;return get_rgb2_lms_ct_wp_m33_fxp(matrix,matrix_scale,cross_talk,white_point,white_point_scale,output);}
static short yuv_to_rgb_narrow(void *opaque,unsigned int definition,int bits,void *output)
{(void)opaque;return get_yuv2_rgb_m33_narrow_fxp(definition,bits,output);}
static short yuv_to_rgb(void *opaque,unsigned int definition,void *output)
{(void)opaque;return get_yuv2_rgb_m33_fxp(definition,output);}
static void yuv_offset(void *opaque,unsigned int range,int bits,void *output)
{(void)opaque;get_yuv_rgb_off_fxp(range,bits,output);}
static void range(void *opaque,int bits,unsigned int input_range,unsigned short *minimum,unsigned short *extent)
{(void)opaque;get_range(bits,input_range,minimum,extent);}
static void ta_tb(void *opaque,int gamma,unsigned int maximum,unsigned int minimum,int *ta,int *tb)
{(void)opaque;calc_ta_tb(gamma,maximum,minimum,ta,tb);}
static int power2(void *opaque,int value,int scale_in,int scale_out)
{(void)opaque;return fx_pow2_taylor(value,scale_in,scale_out);}
static unsigned int gamma(void *opaque,unsigned short a,unsigned short b,unsigned short exponent,unsigned int gain,unsigned short y)
{(void)opaque;return de_gamma(a,b,exponent,gain,y);}
static short rgb_to_lms_default(void *opaque,unsigned int definition,void *output)
{(void)opaque;return get_rgb2_lms_by_def_m33(definition,output);}
static short rgb_to_lms_primaries(void *opaque,int rx,int ry,int gx,int gy,int bx,int by,int wx,int wy,void *output)
{(void)opaque;return get_rgb2_lms_by_prims_m33_fxp(rx,ry,gx,gy,bx,by,wx,wy,output);}
static void ipt_norm(void *opaque,unsigned int bits,unsigned int input_range,void *offset,int *scale)
{(void)opaque;get_ipt_norm_param((int)bits,input_range,offset,scale);}
static void target_curve(void *opaque,const void *signal,void *output)
{(void)opaque;target_signal_to_tc_ctrl(signal,output);}
static void hlg_signal(void *opaque,int minimum,int maximum,void *output)
{(void)opaque;update_tc_signal_hlg_a(minimum,maximum,output);}
static void tone_control(void *opaque,const void *mapping,const void *control,void *output,int graphics)
{(void)opaque;tm_ctrl_to_tc_ctrl(mapping,control,output,graphics);}
static void tone_level(void *opaque,const void *mapping,void *output)
{(void)opaque;update_tc_ctrl_level(mapping,output);}
static int tone_curve(void *opaque,const void *control,void *map,void *used)
{
    const struct dm_commit_bindings *bindings=opaque;
    return create_tone_curve(control,map,used,&bindings->tone_curve);
}
static unsigned int output_map(void *opaque,void *configuration,int update_target,void *dm)
{(void)opaque;return (unsigned int)commit_ks_output_map(configuration,update_target,dm);}
static void upsample(void *opaque,unsigned int chroma,const void *bits,void *output)
{(void)opaque;commit_us(chroma,bits,output);}
static void trim(void *opaque,unsigned short maximum,void *metadata,void *dm,void *control)
{
    const struct dm_commit_bindings *bindings=opaque;
    (void)commit_trim_with_ops(maximum,metadata,dm,control,&bindings->trim);
}
static struct dm_commit_ops operations(const struct dm_commit_bindings *bindings)
{
    struct dm_commit_ops result={
        (void *)bindings,lms_to_ipt,ipt_to_lms,invert_matrix,frame_format,ipt_input,
        assign_matrix,assign_vector,rgb_offset,rgb_to_lms_crosstalk,yuv_to_rgb_narrow,
        yuv_to_rgb,yuv_offset,range,ta_tb,power2,gamma,rgb_to_lms_default,
        rgb_to_lms_primaries,ipt_norm,target_curve,hlg_signal,tone_control,tone_level,
        tone_curve,output_map,upsample,trim
    };
    return result;
}
int commit_dm_metadata(void *metadata,void *dm,const struct dm_commit_bindings *bindings)
{
    struct dm_commit_ops ops=operations(bindings);
    return commit_dm_metadata_with_ops(metadata,dm,&ops);
}
void commit_dm_color(void *metadata,void *configuration,void *color,void *input_map)
{
    struct dm_commit_ops ops=operations((const struct dm_commit_bindings *)0);
    commit_dm_color_with_ops(metadata,configuration,color,input_map,&ops);
}
