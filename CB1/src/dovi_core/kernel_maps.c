/* Build display kernel maps using byte access to packed fields. */
#include "kernel_maps.h"
#include "control_abi_recovered.h"
#include "byte_utils.h"

static unsigned int r16(const void *object,unsigned int offset)
{
    const unsigned char *p=(const unsigned char *)object+offset;
    return (unsigned int)p[0]|(unsigned int)p[1]<<8;
}
static unsigned int r32(const void *object,unsigned int offset)
{
    const unsigned char *p=(const unsigned char *)object+offset;
    return (unsigned int)p[0]|(unsigned int)p[1]<<8|(unsigned int)p[2]<<16|(unsigned int)p[3]<<24;
}
static void *rp(const void *object,unsigned int offset)
{
    unsigned long long p=r32(object,offset)|(unsigned long long)r32(object,offset+4u)<<32;
    return (void *)p;
}
static void w16(void *object,unsigned int offset,unsigned int value)
{
    unsigned char *p=(unsigned char *)object+offset;
    p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);
}
static void w32(void *object,unsigned int offset,unsigned int value)
{
    unsigned char *p=(unsigned char *)object+offset;
    p[0]=(unsigned char)value;p[1]=(unsigned char)(value>>8);
    p[2]=(unsigned char)(value>>16);p[3]=(unsigned char)(value>>24);
}
static unsigned int shl(unsigned int value,unsigned int bits){return value<<(bits&31u);}
static int div_max(int denominator){return denominator ? (int)(4294967295LL/denominator) : 0;}
static void identity(void *matrix)
{
    /* Machine stores zeroes and ones; it does not scale the identity. */
    unsigned int n;for(n=0u;n<9u;++n)w16(matrix,n*2u,(n%4u)==0u?1u:0u);
}

void sig_env_to_ks_frame_format(const void *signal,void *format)
{
    unsigned int dtype,weave,channels,size;
    w32(format,0u,r32(signal,0u));w32(format,4u,r32(signal,4u));
    dtype=r32(signal,28u);((unsigned char *)format)[8]=dtype==0u?0:dtype==1u?1:2;
    weave=r32(signal,24u);((unsigned char *)format)[9]=weave==0u?0:weave==1u?1:2;
    ((unsigned char *)format)[10]=(unsigned char)(r32(signal,32u)!=0u);
    ((unsigned char *)format)[11]=(unsigned char)r32(signal,36u);
    w32(format,16u,r32(signal,8u));w32(format,24u,r32(signal,12u));
    weave=r32(signal,24u);
    channels=weave==0u?1u:r32(signal,16u)==2u?4u:r32(signal,16u)==4u?1u:r32(signal,20u)==1u?2u:3u;
    dtype=r32(signal,28u);size=dtype==1u?1u:dtype==2u?4u:2u;
    w32(format,20u,channels*size);
}

void get_range(int bits,unsigned int range,unsigned short *minimum,unsigned short *extent)
{
    unsigned int unit;
    if(range==0u){unit=shl(1u,(unsigned int)bits-8u)&65535u;*minimum=(unsigned short)(unit*16u);*extent=(unsigned short)(unit*235u-(*minimum));}
    else if(range==2u){unit=shl(1u,(unsigned int)bits-8u)&65535u;*minimum=(unsigned short)unit;*extent=(unsigned short)(shl(1u,(unsigned int)bits-10u)*1019u-unit);}
    else {*minimum=0u;*extent=(unsigned short)(shl(1u,(unsigned int)bits)-1u);}
}

void get_ipt_norm_param_with_ops(int bits,unsigned int range,int *offset,int *scale,const KernelMapOps *ops)
{
    ops->yuv_rgb_offset(range,bits,offset);
    if(range!=0u)*scale=bits==0?0x7fffffff:div_max((int)(shl(1u,(unsigned int)bits)-1u));
    else *scale=bits<8?0x12b404a:div_max((int)shl(219u,(unsigned int)bits-8u));
}

void commit_ipt_ks_input_map_with_ops(unsigned int bits,unsigned int range,void *map,const KernelMapOps *ops)
{
    unsigned char *out=(unsigned char *)map;short *matrix=(short *)(out+4u);
    int offsets[3],scale;short matrix_scale;
    out[0]=0u;
    matrix_scale=range==0u?ops->ipt_to_lms_narrow((short)bits,matrix):ops->ipt_to_lms(matrix);
    w16(map,2u,(unsigned short)matrix_scale);
    get_ipt_norm_param_with_ops((int)bits,range,offsets,&scale,ops);
    ops->yuv_to_rgb_offset(matrix,(unsigned int)offsets[0],(unsigned int)offsets[1],(int *)(out+24u));
    w16(map,36u,0u);w16(map,1074u+16u,1u);out[46]=1u;
    w16(map,1074u+8u,1u);w16(map,1074u,1u);
    w32(map,40u,(unsigned int)div_max((int)shl((unsigned int)scale,16u-bits)));
    w16(map,1074u+14u,0u);w16(map,1074u+12u,0u);w16(map,1074u+10u,0u);
    w16(map,1074u+6u,0u);w16(map,1074u+4u,0u);w16(map,1074u+2u,0u);w16(map,1072u,0u);
}

int commit_ks_output_map_with_ops(const void *config,int update_target_minmax,void *dm,const KernelMapOps *ops)
{
    const unsigned char *target=(const unsigned char *)config+DM_CONFIG_TGTSIGENV_OFFSET;
    unsigned char *kernel=(unsigned char *)rp(dm,DM_CONTEXT_HDMKS_OFFSET);
    unsigned char *out=kernel+DM_KERNEL_KSOMAP_OFFSET;
    unsigned char *csc=(unsigned char *)dm+DM_CONTEXT_CSCCTRL_OFFSET;
    unsigned int color,bits,range,extent;short scale;int offsets[3];
    bits=r32(target,36u);
    w32(csc,112u,32767u);w32(csc,108u,32767u);w32(csc,104u,32767u);w32(csc,116u,15u);
    color=r32(target,16u);out[60]=color==1u?1u:color==2u?2u:0u;
    ops->assign_matrix((const short *)(csc+144u),(short *)(out+2u));
    w16(out,0u,r32(csc,164u));
    if(color==3u&&r32(target,156u)==0u){identity(out+22u);w16(out,20u,0u);}
    else {
        if(r32(target,156u)!=0u){mem_cpy_byte(csc+80u,target+160u,18);w32(csc,100u,r32(target,180u));}
        else if(r32(target,120u)==0u){scale=ops->lms_to_rgb_definition(r32(target,48u),(short *)(csc+80u));w32(csc,100u,(unsigned int)(int)scale);}
        else {scale=ops->lms_to_rgb_primaries((int)r32(target,124u),(int)r32(target,128u),(int)r32(target,132u),(int)r32(target,136u),(int)r32(target,140u),(int)r32(target,144u),(int)r32(target,148u),(int)r32(target,152u),(short *)(csc+80u));w32(csc,100u,(unsigned int)(int)scale);}
        if(r32(target,52u)!=0u){mem_cpy_byte(csc+104u,target+56u,12);w32(csc,116u,(unsigned int)(int)(short)r16(target,68u));}
        scale=ops->lms_to_rgb_white_point((const short *)(csc+80u),(short)r32(csc,100u),(int)r32(target,200u),(const int *)(csc+104u),(short)r32(csc,116u),(short *)(out+22u));w16(out,20u,(unsigned short)scale);
    }
    if(color==3u){w32(out,40u,0u);w32(out,44u,0x9c400000u);}
    if(update_target_minmax!=0){
        unsigned char *current=(unsigned char *)rp(dm,DM_CONTEXT_HDMKS_OFFSET)+DM_KERNEL_KSOMAP_OFFSET;
        unsigned int minimum=r32(target,184u),maximum=r32(target,188u);
        if(r32(target,40u)==1u||r32(target,16u)==3u){minimum=0u;maximum=0x9c400000u;}
        w32(current,40u,minimum);w32(current,44u,maximum);
    }
    if(color==3u){
        w16(out,48u,0u);w16(out,56u,bits);out[58]=1u;out[60]=0u;
        extent=r32(target,36u);w32(out,52u,shl(shl(1u,extent)-1u,16u-extent));
        if(r32(target,76u)==0u){scale=r32(target,44u)==1u?ops->lms_to_ipt((short *)(out+64u)):ops->lms_to_ipt_narrow((short)r32(target,36u),(short *)(out+64u));w16(out,62u,(unsigned short)scale);}
        else {ops->assign_matrix((const short *)(target+80u),(short *)(out+64u));w16(out,62u,r32(target,100u));}
    } else {
        unsigned short minimum,length;
        range=(r32(target,16u)-1u)<2u?r32(target,44u):1u;
        get_range((int)bits,range,&minimum,&length);w16(out,48u,minimum);w16(out,50u,length);
        extent=shl(1u,r32(target,36u))-1u;w32(out,52u,extent?(unsigned int)length*65536u/extent:(unsigned int)length*65536u);
        w16(out,56u,bits);out[58]=(unsigned char)r32(target,40u);
        if(r32(target,76u)==0u){scale=r32(target,44u)==0u?ops->rgb_to_yuv_narrow(r32(target,72u),(int)r32(target,36u),(short *)(out+64u)):ops->rgb_to_yuv(r32(target,72u),(short *)(out+64u));w16(out,62u,(unsigned short)scale);}
        else {ops->assign_matrix((const short *)(target+80u),(short *)(out+64u));w16(out,62u,r32(target,100u));}
    }
    if(r32(target,104u)!=0u){ops->assign_vector((const int *)(target+108u),offsets);w32(out,88u,(unsigned int)offsets[1]>>((15u-bits)&31u));w32(out,84u,(unsigned int)offsets[0]>>((15u-bits)&31u));w32(out,92u,(unsigned int)offsets[2]>>((15u-bits)&31u));}
    else if(color==3u)ops->yuv_rgb_offset(r32(target,44u),(int)r32(target,36u),(int *)(out+84u));
    else {ops->yuv_rgb_offset(r32(target,44u),(int)bits,offsets);w32(out,84u,(unsigned int)offsets[0]);w32(out,88u,(unsigned int)offsets[1]);w32(out,92u,(unsigned int)offsets[2]);}
    range=r32(target,44u);extent=r32(target,36u);ops->yuv_rgb_offset(range,(int)extent,(int *)(out+100u));
    w32(out,96u,range==0u?((int)extent>7?shl(219u,extent-8u):219u):shl(1u,extent)-1u);
    return 0;
}

void commit_us(unsigned int chroma,const int *bits,void *output)
{
    unsigned char *out=(unsigned char *)output;unsigned int depth;
    out[0]=chroma==0u?0:chroma==1u?1:2;w16(out,2u,0u);
    depth=r32(bits,0u);w16(out,6u,1u);w16(out,4u,shl(1u,depth)-1u);
    w16(out,8u,8u);w16(out,10u,64u);w16(out,42u,4u);w16(out,26u,192u);w16(out,12u,192u);w16(out,28u,64u);
    w16(out,44u,12u);w16(out,46u,22u);w16(out,48u,94u);w16(out,50u,0xfdf4u);
    w16(out,52u,0x998u);w16(out,54u,0x998u);w16(out,56u,0xfdf4u);w16(out,58u,94u);w16(out,60u,22u);
}

void clamp_level2(int *level2)
{
    int a=level2[1],b=level2[2],c=level2[3],d=level2[4],e=level2[5],f;
    if(a>2047)a=2047;if(a< -2048)a= -2048;level2[1]=a;
    if(b>2047)b=2047;if(b< -2048)b= -2048;level2[2]=b;
    if(c>2047)c=2047;if(c< -2048)c= -2048;level2[3]=c;
    if(d>65535)d=65535;if(d<0)d=0;level2[4]=d;
    if(e<2048)e=2048;else if(e>6143)e=6143;level2[5]=e;
    f=level2[6];if(f>8191)f=8191;if(f<0)f=0;level2[6]=f;
}

int commit_trim_with_ops(unsigned short target_max_pq,void *extensions,void *dm,void *tone_control,const KernelTrimOps *ops)
{
    unsigned char *trim=(unsigned char *)extensions+88u;
    unsigned short *values=(unsigned short *)rp(trim,8u),indices[17],keys[17];
    int selected[7],dim=(short)r16(trim,2u),count=(short)r16(trim,6u),i,j,k,n,lower=0,upper=0;
    unsigned int bubble=(unsigned short)count;int swapped;
    unsigned char *tm=(unsigned char *)dm+DM_CONTEXT_DMCFG_OFFSET+DM_CONFIG_TMCTRL_OFFSET;
    int *level=(int *)((unsigned char *)tone_control+116u);
    values[0]=(unsigned short)r16(extensions,74u);
    for(i=0;i<=count;++i){indices[i]=(unsigned short)i;keys[i]=values[i*dim];}
    if(bubble!=0u)do {
        swapped=0;for(i=0;i<(int)bubble;++i)if(keys[i+1]<=keys[i]){
            unsigned short tmp=keys[i];keys[i]=keys[i+1];keys[i+1]=tmp;
            tmp=indices[i];indices[i]=indices[i+1];indices[i+1]=tmp;swapped=1;
        }
    } while(swapped&&--bubble!=0u);
    if(keys[0]<target_max_pq&&target_max_pq<keys[count]){
        upper=1;while(keys[upper]<=target_max_pq)++upper;lower=upper-1;
        if(upper>count){
            ops->assertion("/mnt/fileroot/yao.liu/androidq/hardware/amlogic/dolby_vision/dv_source_code/libs/dm/dlb_dm_process/src/CdmCmmtFxp.c","InterpolateTrim",0x80a,"idx2 <= pTrim->TrimNum");
            ops->dump_stack();
        }
        if(keys[upper]!=keys[lower]){
            j=(int)indices[lower]*dim;k=(int)indices[upper]*dim;
            for(i=1;i<dim;++i){
                int difference=(int)values[k+i]-(int)values[j+i];
                int numerator=(int)((unsigned int)difference*32768u),denominator=(int)values[k]-(int)values[j];
                int slope=denominator?numerator/denominator:numerator;
                selected[i]=(int)values[j+i]+((int)((unsigned int)slope*((unsigned int)target_max_pq-values[j]))>>15);
            }
        }else {n=indices[lower]*dim;for(i=1;i<dim;++i)selected[i]=values[n+i];}
    } else {n=indices[keys[0]>=target_max_pq?0:count]*dim;for(i=1;i<dim;++i)selected[i]=values[n+i];}
    for(i=1;i<dim-1;++i){
        /* The function starts at ValueAdj2[6] and Default2[6]. */
        level[i]=(int)((unsigned int)selected[i]+r32(tm,96u+(unsigned int)i*4u)+r32(tm,68u+(unsigned int)i*4u));
    }
    if(r32(tone_control,56u)==1u&&r32(tone_control,60u)==0u){
        level[5]=(int)((unsigned int)level[5]-r32(tm,88u));
        level[5]=(int)((unsigned int)level[5]+r32(tm,124u));
    }
    level[4]=(int)(((unsigned int)selected[4]+r32(tm,112u)+r32(tm,84u))*16u);
    level[6]=(int)((unsigned int)selected[6]*2u+r32(tm,120u)+r32(tm,92u));
    clamp_level2(level);level[6]-=4096;
    return 0;
}
