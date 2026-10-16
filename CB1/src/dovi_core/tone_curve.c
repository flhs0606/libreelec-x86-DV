/* Build a three-point tone curve and its auxiliary tone/saturation maps. */
#include "tone_curve.h"
#include "tone_parameters.h"
#include "tone_curve_lut.h"
#include "tone_maps.h"
#include "tone_math.h"
#include "trim_luminance.h"

static int trim_limit(int value)
{
    if(value < -0x8000000)return -0x8000000;
    return value > 0x8000000 ? 0x8000000 : value;
}
static int signed_word(unsigned int value)
{return value<=0x7fffffffu?(int)value:-1-(int)~value;}

void create_tone_curve_3pxx(struct tone_curve_source *source,
    struct tone_curve_display *display,struct tone_curve_pair *param1,
    struct tone_curve_pair *param2,struct tone_curve_pair *param3,
    int *slope_over_rolloff,short *tone_i,short *tone_s,
    short *saturation_i,short *saturation_s,int *ratio,
    short lut_size,short clamp_output)
{
    int trim_slope=trim_limit(source->trim_slope);
    int trim_power=trim_limit(source->trim_power);
    int trim_offset=trim_limit(source->trim_offset);
    int maximum=source->clip,minimum=source->crush,midpoint;
    int target_min,target_mid,target_max,source_mid,slope,source_target_ratio,screen_ratio;
    if(maximum<0x4108b6b6)maximum=0x4108b6b6;
    if(minimum>0x13316bf8)minimum=0x13316bf8;
    source->clip=maximum;
    midpoint=minimum+0x15e00000;
    if(midpoint<source->mid)midpoint=source->mid;
    source->crush=minimum;
    if(maximum-0x80000<midpoint)midpoint=maximum-0x80000;
    source->mid=midpoint;
    if(display->mode==1){
        calc_mapped_params(maximum,source,display,&target_min,&target_mid,&target_max,
            &source_mid,&slope,&source_target_ratio,&screen_ratio);
        calc_curve_params(source->crush,source_mid,source->clip,target_min,target_mid,
            target_max,slope,display,param1,param2,param3,slope_over_rolloff);
        generate_tone_curve_lut(source->crush,source->clip,param1,param2,param3,
            *slope_over_rolloff,display,trim_slope,trim_power,trim_offset,
            tone_i,(short *)0,lut_size,clamp_output);
    }else{
        unsigned long long magnitude=uabs_ls(((long long)display->mid_bias*0x10aab)>>12);
        int weight=magnitude<0x7fffffffull?(int)magnitude:0x7fffffff;
        int clip_weight=0x7fffffff-weight;
        int product=smpy_ss(clip_weight,source->clip);
        maximum=signed_word((unsigned int)weight+(unsigned int)product);
        calc_mapped_params(maximum,source,display,&target_min,&target_mid,&target_max,
            &source_mid,&slope,&source_target_ratio,&screen_ratio);
        calc_curve_params(source->crush,source_mid,maximum,target_min,target_mid,
            target_max,slope,display,param1,param2,param3,slope_over_rolloff);
        generate_tone_curve_lut(source->crush,maximum,param1,param2,param3,
            *slope_over_rolloff,display,trim_slope,trim_power,trim_offset,
            tone_i,(short *)0,lut_size,clamp_output);
    }
    if(ratio==(int *)0){
        (void)tone_map_s(source->intensity_weight,source_target_ratio,display->mode,tone_s,lut_size);
        (void)saturation_map_s(source->chroma_vector_weight,source_target_ratio,display->mode,saturation_s,lut_size);
        (void)saturation_map_i(display->min_pq,display->max_pq,source->min_pq,
            source->saturation_gain,tone_i,display->mode,saturation_i,lut_size);
    }else{
        *ratio=source_target_ratio;
    }
}
static void build(void *opaque,struct tone_curve_source *source,
    struct tone_curve_display *display,struct tone_curve_pair *param1,
    struct tone_curve_pair *param2,struct tone_curve_pair *param3,
    int *slope_over_rolloff,short *tone_i,short *tone_s,
    short *saturation_i,short *saturation_s,int *ratio,short size,short clamp)
{
    (void)opaque;
    create_tone_curve_3pxx(source,display,param1,param2,param3,slope_over_rolloff,
        tone_i,tone_s,saturation_i,saturation_s,ratio,size,clamp);
}
const struct tone_curve_ops *tone_curve_concrete_ops(void)
{
    static const struct tone_curve_ops operations={(void *)0,build};
    return &operations;
}
