/* Commit control configuration and pack composer, DM and LUT output. */
#include "register_commit.h"
#include "control_abi_recovered.h"
#include "control_luminance.h"
#include "control_infoframe.h"
#include "byte_utils.h"
#include "composer_registers.h"

typedef char require_pointer64[(sizeof(void *) == 8) ? 1 : -1];
typedef char require_u32[(sizeof(unsigned int) == 4) ? 1 : -1];

static unsigned int r16(const void *o, unsigned int n)
{
    const unsigned char *p = (const unsigned char *)o + n;
    return (unsigned int)p[0] | (unsigned int)p[1] << 8;
}
static unsigned int r32(const void *o, unsigned int n)
{
    const unsigned char *p = (const unsigned char *)o + n;
    return (unsigned int)p[0] | (unsigned int)p[1] << 8 |
           (unsigned int)p[2] << 16 | (unsigned int)p[3] << 24;
}
static void *rp(const void *o, unsigned int n)
{
    unsigned long long v = r32(o,n);
    v |= (unsigned long long)r32(o,n+4u) << 32;
    return (void *)v;
}
static void w16(void *o, unsigned int n, unsigned int v)
{
    unsigned char *p = (unsigned char *)o+n;
    p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8);
}
static void w32(void *o, unsigned int n, unsigned int v)
{
    unsigned char *p=(unsigned char *)o+n;
    p[0]=(unsigned char)v; p[1]=(unsigned char)(v>>8);
    p[2]=(unsigned char)(v>>16); p[3]=(unsigned char)(v>>24);
}
static void wp(void *o, unsigned int n, const void *v)
{
    unsigned long long p=(unsigned long long)v;
    w32(o,n,(unsigned int)p); w32(o,n+4u,(unsigned int)(p>>32));
}
static unsigned int be16(const void *o, unsigned int n)
{
    const unsigned char *p=(const unsigned char *)o+n;
    return (unsigned int)p[0]<<8 | p[1];
}
static unsigned int be32(const void *o, unsigned int n)
{
    const unsigned char *p=(const unsigned char *)o+n;
    return (unsigned int)p[0]<<24 | (unsigned int)p[1]<<16 |
           (unsigned int)p[2]<<8 | p[3];
}
static void copy_bytes(void *dst,const void *src,unsigned int count)
{
    unsigned int i;
    for(i=0u;i<count;++i) ((unsigned char *)dst)[i]=((const unsigned char *)src)[i];
}

int control_commit_dm_config(const void *config, void *kernel, void *dm)
{
    unsigned char *dst=(unsigned char *)rp(dm,DM_CONTEXT_PDMCFGTOBE_OFFSET);
    unsigned long long frame;
    if(!dm) return -1;
    mem_cpy_byte(dst,config,984);
    if(r32(dst,8u)==1u) {
        w32(dst,952u,r32(dst,572u)); w32(dst,956u,r32(dst,576u));
        w16(dst,960u,r16(dst,580u)); w16(dst,962u,r16(dst,582u));
    }
    if(r32(dm,4u)!=0u && mem_eq_byte(dst,rp(dm,1976u),984)!=0) return 0;
    wp(dm,4392u,kernel); w32(dm,0u,1u); w32(dm,1992u,1u);
    frame=(unsigned long long)rp(kernel,8u);
    wp(kernel,16u,(void *)(frame+0x10e0000u));
    wp(kernel,24u,(void *)(frame+0x21c0000u));
    frame=(unsigned long long)rp(kernel,32u);
    wp(kernel,40u,(void *)(frame+0x10e0000u));
    wp(kernel,48u,(void *)(frame+0x21c0000u));
    return 1;
}

static void commit_config(void *context,void *config,const struct register_commit_ops *ops)
{
    void *kernel=rp(context,16u), *dm=rp(context,0u);
    if(ops->commit_config) (void)ops->commit_config(config,kernel,dm);
    else (void)control_commit_dm_config(config,kernel,dm);
}

static unsigned int min_linear(unsigned int v) { return (v<<18)/10000u; }
static unsigned int max_linear(unsigned int v)
{ return (unsigned int)(((unsigned long long)v<<18)/10000u); }

static void source_signal(void *config,const void *source,int input)
{
    unsigned char *s=(unsigned char *)config+372u;
    unsigned int min,max;
    w32(s,0u,r32(source,16u)); w32(s,4u,r32(source,12u)); w32(s,28u,0u);
    if(input==0) {w32(s,20u,0u);w32(s,24u,0u);return;}
    if(input!=1 && input!=2) return;
    w32(s,16u,0u); w32(s,20u,r32(source,4u)!=0u);
    w32(s,24u,r32(source,4u)==0u?0u:2u); w32(s,36u,12u);
    if(input==1) {
        w32(s,40u,1u); w32(s,48u,3u);w32(s,72u,3u);
        w32(s,44u,r32(source,8u)!=0u);
        min=r32(source,20u); min=min?min_linear(min):0x51eu;
        max=r32(source,24u); max=max?max_linear(max):0xfa00000u;
    } else {
        min=0x51eu;max=0x1900000u;
        w32(s,40u,0u);w16(s,196u,0x999au);w32(s,44u,0u);
        w32(s,48u,2u);w32(s,72u,2u);
    }
    w32(s,200u,min);w32(s,204u,max);
    w16(s,208u,control_l_to_pq12(min));w16(s,210u,control_l_to_pq12(r32(s,204u)));
    if(input==2) {w16(s,212u,0x181u);w32(s,216u,0x151cb4u);w16(s,214u,0x433u);}
}

static void graphics_signal(void *context,void *config,const void *graphics)
{
    unsigned char *g=(unsigned char *)config+104u;
    unsigned int value;
    w32(config,0u,r32(graphics,0u));w32(g,0u,r32(graphics,8u));w32(g,4u,r32(graphics,4u));
    w32(g,28u,(int)r32(graphics,32u)<9?1u:0u);w32(g,32u,0u);
    w32(g,16u,r32(graphics,28u)==0u?0u:2u);
    value=r32(graphics,44u);
    w32(g,24u,value==1u?2u:0u);w32(g,20u,value==0u?0u:value==1u?1u:2u);
    w32(g,36u,r32(graphics,32u));w32(g,40u,r32(graphics,20u)==0u?0u:1u);
    w32(g,44u,r32(graphics,24u)==0u?0u:1u);
    value=r32(graphics,40u);if(value<6u)w32(g,48u,value);
    w32(g,132u,0u);w32(g,64u,0x7fffu);w32(g,60u,0x7fffu);w32(g,56u,0x7fffu);
    w32(g,168u,0u);w32(g,52u,0u);w16(g,68u,15u);
    value=r32(graphics,36u);if(value<6u)w32(g,72u,value);
    w32(g,76u,0u);w32(g,104u,0u);w16(g,196u,r32(graphics,48u));
    w32(g,200u,min_linear(r32(context,4132u)+r32(graphics,12u)));
    w16(g,208u,0u);w16(g,210u,0u);
    w32(g,204u,max_linear(r32(context,4136u)+r32(graphics,16u)));
    w16(g,212u,r32(graphics,52u));w16(g,214u,r32(graphics,56u));w32(g,216u,r32(graphics,60u));
    w16(g,220u,42u);w32(g,224u,0x28fu);
}

static void set_target_limits(void *config,unsigned int min,unsigned int max,
                              unsigned int min_pq,unsigned int max_pq,unsigned int mode)
{
    w32(config,952u,min);w32(config,956u,max);w16(config,960u,min_pq);w16(config,962u,max_pq);
    w32(config,8u,mode);w32(config,12u,min);w32(config,16u,max);
}

static void metadata_extensions(void *context,void *metadata)
{
    unsigned char *e=(unsigned char *)rp(context,8u);
    unsigned char *m=(unsigned char *)metadata;
    unsigned int i,j,level,base;
    int number,dimension;
    unsigned char *trim;
    e[0]=m[0];e[1]=m[1];w16(e,2u,13u);
    for(i=0u;i<9u;++i)w16(e,4u+i*2u,be16(m,2u+i*2u));
    for(i=0u;i<3u;++i)w32(e,24u+i*4u,be32(m,20u+i*4u));
    w16(e,36u,14u);
    for(i=0u;i<9u;++i)w16(e,38u+i*2u,be16(m,32u+i*2u));
    for(i=0u;i<3u;++i)w16(e,56u+i*2u,be16(m,50u+i*2u));
    w32(e,64u,be32(m,56u));
    for(i=0u;i<4u;++i)e[68u+i]=m[60u+i];
    w16(e,72u,be16(m,64u));w16(e,74u,be16(m,66u));w16(e,76u,be16(m,68u));e[78u]=m[70u];
    w16(e,514u,r16(e,72u));w16(e,516u,r16(e,74u));w16(e,518u,(r16(e,72u)+r16(e,74u))>>1);
    w16(e,80u,0u);w16(e,94u,0u);w16(rp(e,96u),0u,r16(e,74u));
    w16(e,110u,0u);w16(rp(e,112u),0u,r16(e,74u));
    w16(e,496u,0u);w16(e,498u,0u);w16(e,500u,0u);w16(e,502u,0u);w16(e,504u,0u);
    for(i=0u;i<e[78u];++i) {
        base=71u+i*261u;level=m[base+4u];
        if(level==1u)for(j=0u;j<3u;++j)w16(e,514u+j*2u,be16(m,base+5u+j*2u));
        else if(level==2u && r32((unsigned char *)context+24u,644u)==0u) {
            number=(short)r16(e,94u);dimension=(short)r16(e,90u);
            if(number<(short)r16(e,92u)) {
                number=(short)(number+1);w16(e,94u,(unsigned int)number);trim=(unsigned char *)rp(e,96u);
                /* The first store uses captured descriptor values;
                 * later stores reload all three fields. Signed dimensions
                 * address bytes before Trima through SXTW/SMADDL in ARM64. */
                trim+=(long long)number*dimension*2;
                w16(trim,0u,be16(m,base+5u));
                for(j=1u;j<7u;++j) {
                    number=(short)r16(e,94u);dimension=(short)r16(e,90u);
                    trim=(unsigned char *)rp(e,96u)+(long long)number*dimension*2;
                    w16(trim,j*2u,be16(m,base+5u+j*2u));
                }
                number=(short)r16(e,94u);dimension=(short)r16(e,90u);
                trim=(unsigned char *)rp(e,96u)+(long long)number*dimension*2;
                if((r16(trim,12u)&0x1fffu)==0x1fffu)w16(trim,12u,0xfffu);
            }
            w16(e,80u,r16(e,80u)+1u);
        } else if(level==5u) {
            w16(e,496u,1u);for(j=0u;j<4u;++j)w16(e,498u+j*2u,be16(m,base+5u+j*2u));
        }
    }
}

static void synthetic_composition(void *composition,unsigned int depth)
{
    unsigned char *c=(unsigned char *)composition;
    unsigned int i;
    w32(c,0u,12u);w32(c,4u,depth);w32(c,8u,0u);w32(c,12u,23u);c[1181u]=1u;c[1182u]=0u;
    for(i=0u;i<3u;++i) {
        w32(c,16u+i*4u,2u);w32(c,28u+i*36u,0u);w32(c,32u+i*36u,(1u<<(depth&31u))-1u);
        w32(c,136u+i*4u,0u);w32(c,148u+i*32u,1u);
        w32(c,244u+i*96u,0u);w32(c,248u+i*96u,1u);
        w32(c,532u+i*96u,0u);w32(c,536u+i*96u,depth==8u?0x7878u:0x1806u);
    }
}

static void matrix_regs(void *regs,unsigned int out,const void *map,unsigned int in,unsigned int scale)
{
    unsigned int i;
    for(i=0u;i<4u;++i)w32(regs,out+i*4u,r32(map,in+i*4u));
    w32(regs,out+16u,r16(map,in+16u)|(r16(map,scale)<<16));
}
static void input_regs(void *regs,const void *map,int second)
{
    unsigned int offset=second?8u:20u,i;
    w32(regs,0u,r16(map,38u)|(r16(map,36u)<<16));w32(regs,4u,r32(map,40u));
    if(!second) {w32(regs,8u,r32(map,1112u));w32(regs,12u,r32(map,1116u)|(r32(map,1120u)<<16));w32(regs,16u,r32(map,1124u));}
    matrix_regs(regs,offset,map,4u,2u);
    for(i=0u;i<3u;++i)w32(regs,offset+20u+i*4u,r32(map,24u+i*4u));
    if(second)w32(regs,40u,0u);
    w32(regs,second?44u:52u,((const unsigned char *)map)[46u]);
    matrix_regs(regs,second?48u:56u,map,1074u,1072u);
    matrix_regs(regs,second?68u:76u,map,1094u,1092u);
    w32(regs,second?88u:96u,0u);
    if(!second){w32(regs,100u,0u);w32(regs,104u,0u);}
}
static void output_regs(void *regs,const void *map,const void *context)
{
    unsigned int i;
    w32(regs,4u,r32(context,4144u));matrix_regs(regs,24u,map,2u,0u);matrix_regs(regs,44u,map,22u,20u);
    w32(regs,64u,r32(map,40u));w32(regs,68u,r32(map,44u));w32(regs,72u,r32(map,96u));
    for(i=0u;i<3u;++i)w32(regs,76u+i*4u,r32(map,100u+i*4u));
    w32(regs,88u,r32(map,52u));w32(regs,92u,r16(map,48u));matrix_regs(regs,96u,map,64u,62u);
    for(i=0u;i<3u;++i)w32(regs,116u+i*4u,r32(map,84u+i*4u));
    w32(regs,992u,(r32(context,4156u)&1u)<<5|(r32(context,4152u)&15u)*2u|r32(context,4148u));
}
static void raw_metadata(void *regs,const void *metadata)
{
    const unsigned char *m=(const unsigned char *)metadata;
    unsigned char *out=(unsigned char *)regs+148u;
    unsigned int i,j,length,added,total=71u;
    for(i=0u;i<70u;++i)out[i]=m[i+1u];
    out+=70u;
    for(i=0u;i<m[70u];++i) {
        length=be32(m,71u+i*261u);added=length+5u;
        if((int)added>0) {for(j=0u;j<added;++j)out[j]=m[71u+i*261u+j];out+=added;}
        total+=added;
    }
    w32(regs,144u,(total<<8)|m[0]);
}
static void lut_regs(void *out,const void *tmap,const void *imap)
{
    static const unsigned int starts[4]={4u,1028u,2056u,3080u};
    unsigned int a,i,n;
    for(a=0u;a<4u;++a)for(i=0u;i<256u;++i) {
        n=starts[a]+i*4u;
        w32(out,a*1024u+i*4u,(r16(tmap,n+2u)&0x7fffu)<<15|(r16(tmap,n)&0x7fffu));
    }
    for(i=0u;i<256u;++i)w32(out,4096u+i*4u,r32(imap,48u+i*4u));
}

int commit_reg_with_ops(void *context,int input,int output,void *metadata,
                        const void *composition,const void *target,const void *graphics,
                        const void *source,int priority,void *reg1,void *reg2,void *reg3,
                        void *lut1,void *lut2,const struct register_commit_ops *ops)
{
    unsigned char *config=(unsigned char *)context+24u;
    unsigned char *t=config+768u,*m=(unsigned char *)metadata,*kernel,*temp;
    unsigned int minpq=0u,maxpq=0u,min,max,version,iface,i,count,write;
    int ret,mode=0,use_target=0,hdr_target=0;
    wp(context,4168u,target);
    if(input==0 && r32(context,4164u)==0u && ((const unsigned char *)composition)[1181u]==0u)return -1;
    if(input==0){minpq=be16(m,64u);maxpq=be16(m,66u);}
    source_signal(config,source,input);graphics_signal(context,config,graphics);
    if(input==0) {
        if(priority==2){if(m[1]==1u)w32(context,4124u,1u);else if(r32(context,4124u)==0u)priority=1;if(priority==2)priority=0;}
        if(priority==1)w32(context,4124u,0u);
    }
    w32(t,0u,r32(target,4u));w32(t,4u,r32(target,0u));w32(t,20u,1u);w32(t,36u,12u);w32(t,28u,0u);w32(t,24u,2u);w32(t,16u,0u);
    if(output==0) {
        version=r32(target,20u);
        if(version==0u){w32(context,4184u,0u);mode=5;}
        else if(version==1u && (r32(target,16u)==12u||r32(target,16u)==15u)) {
            if(r32(target,28u)==0u){w32(context,4184u,0u);mode=5;}
            else if(r32(target,28u)==1u){w32(context,4184u,0u);mode=r32(target,8u)?2:5;}
            else{w32(context,4184u,0u);hdr_target=1;}
        } else if(version==2u) {
            iface=r32(target,32u);
            if(iface==2u){w32(context,4184u,0u);mode=r32(target,8u)?2:5;}
            else if(iface==1u || (iface==3u&&r32(target,8u)!=0u)) {
                mode=2;
                if(r32(target,12u)!=0u && r32(target,0u)<1921u && r32(target,4u)<1081u &&
                   (r32(target,36u)==1u || r32(target,36u)==2u)) {
                    w32(context,4148u,1u);w32(context,4152u,1u);w32(t,16u,1u);w32(t,20u,2u);w32(t,24u,1u);
                    if(r32(target,36u)==1u)w32(t,36u,10u);
                }
            }else if(iface==0u)mode=2;
            else{w32(context,4184u,0u);mode=5;}
        }else{w32(context,4184u,0u);hdr_target=1;}
        if(!hdr_target) {
            w32(context,4144u,(unsigned int)mode);w32(t,40u,1u);
            if(mode==5){w32(t,16u,3u);w32(t,48u,0u);w32(t,72u,2u);w32(t,44u,1u);}
            else {
                w32(context,4156u,1u);w32(context,4184u,1u);w32(t,120u,1u);w32(t,156u,0u);w32(t,44u,r32(t,16u)==1u);
                for(i=0u;i<6u;++i)w32(t,124u+i*4u,r32(target,56u+i*4u));
                w32(t,152u,0x150e560u);w32(t,148u,0x140346eu);w32(t,72u,3u);
            }
            if(mode==2||input==2||(input==0&&priority!=0))use_target=1;
            else if(input==1)set_target_limits(config,r32(config,572u),r32(config,576u),r16(config,580u),r16(config,582u),1u);
            else if(input==0) {
                min=control_pq12_to_l((unsigned short)minpq);w32(t,184u,min);
                max=control_pq12_to_l((unsigned short)maxpq);w32(t,188u,max);w16(t,192u,minpq);w16(t,194u,maxpq);
                w32(config,12u,min_linear(r32(target,40u)));w32(config,8u,1u);w32(config,16u,max_linear(r32(target,44u)));
            }
            if(use_target)set_target_limits(config,min_linear(r32(target,40u)),max_linear(r32(target,44u)),r32(target,48u),r32(target,52u),0u);
        }
    }
    if(output==1||hdr_target) {
        w32(context,4144u,2u);w32(t,20u,2u);w32(t,24u,0u);w32(t,40u,1u);w32(t,48u,3u);w32(t,72u,3u);w32(t,44u,0u);
        if(input==2||(input==0&&r32(context,4140u)==0u)) {
            min=min_linear(r32(target,40u));w32(t,184u,min);max=max_linear(r32(target,44u));w32(t,188u,max);
            minpq=control_l_to_pq12(min);w16(t,192u,minpq);maxpq=control_l_to_pq12(r32(t,188u));
            set_target_limits(config,r32(t,184u),r32(t,188u),minpq,maxpq,0u);
        }else if(input==1)set_target_limits(config,r32(config,572u),r32(config,576u),r16(config,580u),r16(config,582u),1u);
        else if(input==0) {
            min=control_pq12_to_l((unsigned short)minpq);w32(t,184u,min);max=control_pq12_to_l((unsigned short)maxpq);
            set_target_limits(config,r32(t,184u),max,minpq,maxpq,1u);
        }
    }else if(output!=0 && (((unsigned int)output-2u)&0xfffffffdu)==0u) {
        w32(context,4144u,4u);w32(t,20u,2u);w32(t,24u,0u);w32(t,40u,0u);w32(t,48u,output==4?3u:2u);w32(t,72u,output==4?3u:2u);
        w16(t,70u,0x999au);w32(t,44u,0u);w32(t,188u,0x1900000u);w32(t,184u,0x51eu);
        minpq=control_l_to_pq12(0x51eu);w16(t,192u,minpq);maxpq=control_l_to_pq12(r32(t,188u));
        set_target_limits(config,r32(t,184u),r32(t,188u),minpq,maxpq,input==2);
    }
    w16(t,196u,42u);commit_config(context,config,ops);
    if(input==0) {
        if(output==1 && r32(context,4140u)!=0u) {
            count=m[70u];write=0u;
            for(i=0u;i<count;++i)if((unsigned char)(m[71u+i*261u+4u]-5u)<=1u){copy_bytes(m+71u+write*261u,m+71u+i*261u,261u);++write;}
            m[70u]=(unsigned char)write;
        }
        metadata_extensions(context,metadata);ret=ops->commit_metadata(rp(context,8u),rp(context,0u));
        (void)comp_cfg_2_comp_reg(composition,reg1,(int)r32(source,12u),(int)r32(source,16u));
    } else {
        ret=ops->commit_metadata(0,rp(context,0u));
        if((unsigned int)input-1u<=1u){temp=(unsigned char *)context+2324u;synthetic_composition(temp,r32(source,0u));composition=temp;}
        (void)comp_cfg_2_comp_reg(composition,reg1,(int)r32(source,12u),(int)r32(source,16u));
    }
    kernel=(unsigned char *)rp(context,16u);input_regs(reg1,kernel+200u,0);
    kernel=(unsigned char *)rp(context,16u);input_regs(reg2,kernel+5556u,1);w32(reg2,92u,r32(kernel,176u)|(r32(kernel,172u)<<13));
    if(output==0){if(input==0){if(ops->reduce_trims)update_md_for_hdmi_bound(context,metadata,ops->reduce_trims);else update_md_for_hdmi(context,metadata);}else get_fixed_metadata(context,metadata);}
    kernel=(unsigned char *)rp(context,16u);output_regs(reg3,kernel+10788u,context);raw_metadata(reg3,metadata);
    kernel=(unsigned char *)rp(context,16u);lut_regs(lut1,kernel+1328u,kernel+200u);lut_regs(lut2,kernel+6684u,kernel+5556u);
    return ret;
}
