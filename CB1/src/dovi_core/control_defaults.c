/* Initialize DM defaults through byte accesses to little-endian fields. */
#include "control_defaults.h"

typedef char require_pointer_64[(sizeof(void *) == 8) ? 1 : -1];
typedef char require_u32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char require_u64[(sizeof(unsigned long long) == 8) ? 1 : -1];

static void put16(void *object, unsigned int offset, unsigned int value)
{
    unsigned char *bytes = (unsigned char *)object + offset;
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
}

static void put32(void *object, unsigned int offset, unsigned int value)
{
    unsigned char *bytes = (unsigned char *)object + offset;
    bytes[0] = (unsigned char)value;
    bytes[1] = (unsigned char)(value >> 8);
    bytes[2] = (unsigned char)(value >> 16);
    bytes[3] = (unsigned char)(value >> 24);
}

static void put64(void *object, unsigned int offset, unsigned long long value)
{
    unsigned int index;
    unsigned char *bytes = (unsigned char *)object + offset;
    for (index = 0u; index < 8u; ++index)
        bytes[index] = (unsigned char)(value >> (index * 8u));
}

static unsigned int get32(const void *object, unsigned int offset)
{
    const unsigned char *bytes = (const unsigned char *)object + offset;
    return (unsigned int)bytes[0] | ((unsigned int)bytes[1] << 8) |
           ((unsigned int)bytes[2] << 16) | ((unsigned int)bytes[3] << 24);
}

void control_init_grc_signal(void *signal)
{
    put32(signal, DM_GRAPHICS_SIGNAL_CLR_OFFSET, 2u);
    put32(signal, DM_GRAPHICS_SIGNAL_CHRM_OFFSET, 2u);
    put32(signal, DM_GRAPHICS_SIGNAL_RGBDEF_OFFSET, 2u);
    put32(signal, DM_GRAPHICS_SIGNAL_YUVXFERSPEC_OFFSET, 2u);
    put16(signal, DM_GRAPHICS_SIGNAL_GAMMA_OFFSET, 0x999au);
    put32(signal, DM_GRAPHICS_SIGNAL_MIN_OFFSET, 0x51eu);
    put32(signal, DM_GRAPHICS_SIGNAL_MAX_OFFSET, 0x1900000u);
    put16(signal, DM_GRAPHICS_SIGNAL_A_OFFSET, 0x181u);
    put32(signal, DM_GRAPHICS_SIGNAL_ROWNUM_OFFSET, 1080u);
    put16(signal, DM_GRAPHICS_SIGNAL_B_OFFSET, 0x433u);
    put32(signal, DM_GRAPHICS_SIGNAL_COLNUM_OFFSET, 1920u);
    put32(signal, DM_GRAPHICS_SIGNAL_DTP_OFFSET, 1u);
    put32(signal, DM_GRAPHICS_SIGNAL_RNG_OFFSET, 1u);
    put32(signal, DM_GRAPHICS_SIGNAL_BDP_OFFSET, 8u);
    put32(signal, DM_GRAPHICS_SIGNAL_V3WP_OFFSET + 8u, 0x8000u);
    put32(signal, DM_GRAPHICS_SIGNAL_V3WP_OFFSET + 4u, 0x8000u);
    put32(signal, DM_GRAPHICS_SIGNAL_V3WP_OFFSET, 0x8000u);
    put32(signal, DM_GRAPHICS_SIGNAL_G_OFFSET, 0x151cb4u);
    put16(signal, DM_GRAPHICS_SIGNAL_WPSCALE_OFFSET, 15u);
    put16(signal, DM_GRAPHICS_SIGNAL_DIAGSIZE_OFFSET, 42u);
    put32(signal, DM_GRAPHICS_SIGNAL_WEAV_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_CROSSTALK_OFFSET, 0x28fu);
    put32(signal, DM_GRAPHICS_SIGNAL_LOC_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_EOTF_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_RGB2LMSRGBWEXT_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_RGB2LMSM33EXT_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_WPEXT_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_YUV2RGBEXT_OFFSET, 0u);
    put32(signal, DM_GRAPHICS_SIGNAL_YUV2RGBOFFEXT_OFFSET, 0u);
    put16(signal, DM_GRAPHICS_SIGNAL_MINPQ_OFFSET, 0u);
    put16(signal, DM_GRAPHICS_SIGNAL_MAXPQ_OFFSET, 0u);
}

void control_init_src_signal(void *signal)
{
    put32(signal, DM_SOURCE_SIGNAL_V3WP_OFFSET + 8u, 0x8000u);
    put32(signal, DM_SOURCE_SIGNAL_V3WP_OFFSET + 4u, 0x8000u);
    put32(signal, DM_SOURCE_SIGNAL_V3WP_OFFSET, 0x8000u);
    put16(signal, DM_SOURCE_SIGNAL_WPSCALE_OFFSET, 15u);
    put32(signal, DM_SOURCE_SIGNAL_YUVXFERSPEC_OFFSET, 2u);
    put16(signal, DM_SOURCE_SIGNAL_GAMMA_OFFSET, 0x999au);
    put32(signal, DM_SOURCE_SIGNAL_MIN_OFFSET, 0x51eu);
    put32(signal, DM_SOURCE_SIGNAL_MAX_OFFSET, 0x3e800000u);
    put16(signal, DM_SOURCE_SIGNAL_A_OFFSET, 0x3dfbu);
    put32(signal, DM_SOURCE_SIGNAL_ROWNUM_OFFSET, 1080u);
    put16(signal, DM_SOURCE_SIGNAL_B_OFFSET, 0xe4u);
    put32(signal, DM_SOURCE_SIGNAL_COLNUM_OFFSET, 1920u);
    put16(signal, DM_SOURCE_SIGNAL_DIAGSIZE_OFFSET, 42u);
    put32(signal, DM_SOURCE_SIGNAL_DTP_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_WEAV_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_LOC_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_CLR_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_BDP_OFFSET, 14u);
    put32(signal, DM_SOURCE_SIGNAL_EOTF_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_RNG_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_RGBDEF_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_RGB2LMSRGBWEXT_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_RGB2LMSM33EXT_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_WPEXT_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_YUV2RGBEXT_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_YUV2RGBOFFEXT_OFFSET, 0u);
    put16(signal, DM_SOURCE_SIGNAL_MINPQ_OFFSET, 0u);
    put16(signal, DM_SOURCE_SIGNAL_MAXPQ_OFFSET, 0u);
    put32(signal, DM_SOURCE_SIGNAL_G_OFFSET, 0x151cb4u);
    put32(signal, DM_SOURCE_SIGNAL_CROSSTALK_OFFSET, 0x28fu);
    /* Leave Chrm and all row-pitch/matrix fields. */
}

void control_init_tgt_signal(void *signal)
{
    put32(signal, DM_TARGET_SIGNAL_WEAV_OFFSET, 2u);
    put32(signal, DM_TARGET_SIGNAL_RGBDEF_OFFSET, 2u);
    put32(signal, DM_TARGET_SIGNAL_YUVXFERSPEC_OFFSET, 2u);
    put32(signal, DM_TARGET_SIGNAL_ROWNUM_OFFSET, 1080u);
    put32(signal, DM_TARGET_SIGNAL_MIN_OFFSET, 0x51eu);
    put32(signal, DM_TARGET_SIGNAL_COLNUM_OFFSET, 1920u);
    put32(signal, DM_TARGET_SIGNAL_V3WP_OFFSET + 8u, 0x8000u);
    put32(signal, DM_TARGET_SIGNAL_V3WP_OFFSET + 4u, 0x8000u);
    put32(signal, DM_TARGET_SIGNAL_V3WP_OFFSET, 0x8000u);
    put32(signal, DM_TARGET_SIGNAL_MAX_OFFSET, 0x1900000u);
    put32(signal, DM_TARGET_SIGNAL_CHRM_OFFSET, 1u);
    put16(signal, DM_TARGET_SIGNAL_WPSCALE_OFFSET, 15u);
    put16(signal, DM_TARGET_SIGNAL_DIAGSIZE_OFFSET, 42u);
    put32(signal, DM_TARGET_SIGNAL_DTP_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_LOC_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_BDP_OFFSET, 12u);
    put32(signal, DM_TARGET_SIGNAL_CLR_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_EOTF_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_RNG_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_LMS2RGBRGBWEXT_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_LMS2RGBM33EXT_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_WPEXT_OFFSET, 0u);
    put16(signal, DM_TARGET_SIGNAL_GAMMA_OFFSET, 0x999au);
    put32(signal, DM_TARGET_SIGNAL_RGB2YUVEXT_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_RGB2YUVOFFEXT_OFFSET, 0u);
    put16(signal, DM_TARGET_SIGNAL_MINPQ_OFFSET, 0u);
    put16(signal, DM_TARGET_SIGNAL_MAXPQ_OFFSET, 0u);
    put32(signal, DM_TARGET_SIGNAL_CROSSTALK_OFFSET, 0x28fu);
}

void control_init_tone(void *tone)
{
    unsigned int index;
    put32(tone, DM_TONE_CONTROL_ROLLOFF_OFFSET, 0x2aaaaaaau);
    put32(tone, DM_TONE_CONTROL_KEYWEIGHT_OFFSET, 0x3000u);
    put32(tone, DM_TONE_CONTROL_INTENSITYVECTORWEIGHT_OFFSET, 0x6000u);
    put32(tone, DM_TONE_CONTROL_CHRMVECTORWEIGHT_OFFSET, 0x6000u);
    put32(tone, DM_TONE_CONTROL_CHRMVECTORWEIGHTBWD_OFFSET, 0x6000u);
    put32(tone, DM_TONE_CONTROL_BWDINVERTIBLE_OFFSET, 0u);
    put32(tone, DM_TONE_CONTROL_INTENSITYVECTORWEIGHTBWD_OFFSET, 0x14000u);
    put32(tone, DM_TONE_CONTROL_BPWEIGHT_OFFSET, 0x1000u);
    put16(tone, DM_TONE_CONTROL_TMIDBIAS_OFFSET, 0u);
    put16(tone, DM_TONE_CONTROL_TMAXBIAS_OFFSET, 0u);
    put16(tone, DM_TONE_CONTROL_TMINBIAS_OFFSET, 0u);
    put16(tone, DM_TONE_CONTROL_DBRIGHTNESS_OFFSET, 0u);
    put16(tone, DM_TONE_CONTROL_DCONTRAST_OFFSET, 0u);
    for (index = 1u; index < 5u; ++index)
        put32(tone, DM_TONE_CONTROL_DEFAULT2_OFFSET + index * 4u, 0u);
    put32(tone, DM_TONE_CONTROL_DEFAULT2_OFFSET + 20u, 0x1000u);
    put32(tone, DM_TONE_CONTROL_DEFAULT2_OFFSET + 24u, 0x800u);
    for (index = 0u; index < 7u; ++index)
        put32(tone, DM_TONE_CONTROL_VALUEADJ2_OFFSET + index * 4u, 0u);
    put32(tone, DM_TONE_CONTROL_VALUEADJ2SATGAINBWD_OFFSET, 0u);
    for (index = 1u; index < 5u; ++index)
        put32(tone, DM_TONE_CONTROL_CODEBIAS2_OFFSET + index * 4u, 0xfffff800u);
    put32(tone, DM_TONE_CONTROL_CODEBIAS2_OFFSET + 24u, 0u);
    put32(tone, DM_TONE_CONTROL_CODEBIAS2_OFFSET + 20u, 0x800u);
    for (index = 0u; index < 4u; ++index) {
        put32(tone, DM_TONE_CONTROL_DEFAULT3_OFFSET + index * 4u, 0u);
        put32(tone, DM_TONE_CONTROL_CODEBIAS3_OFFSET + index * 4u, 0u);
    }
    /* Default2[0], CodeBias2[0], l2off and padding remain untouched. */
}

void *control_init_config(int platform, void *h_mmg, void *config)
{
    unsigned char *bytes = (unsigned char *)config;
    void *alpha = bytes + DM_CONFIG_ALPSIGENV_OFFSET;
    (void)platform;
    put64(config, DM_CONFIG_HMMG_OFFSET, (unsigned long long)h_mmg);
    /* DmCtrlFxp_t scalar offsets and widths. */
    put32(config, 32u, 1u);    /* CFxp */
    put32(config, 12u, 0x51eu);
    put32(config, 8u, 0u);     /* CStbModeGpri */
    put32(config, 16u, 0x1900000u);
    put16(config, 20u, 0u);
    put16(config, 22u, 0u);
    put32(config, 24u, 0u);    /* CPlatformCpu */
    put32(config, 28u, 0u);
    put32(config, 96u, 0u);
    put32(config, 100u, 0u);
    control_init_grc_signal(bytes + DM_CONFIG_GRCSIGENV_OFFSET);
    /* Alpha scalar layout is the common signal prefix with Bdp at +36. */
    put32(alpha, 28u, 1u);    /* CDtpU8 */
    put32(alpha, 4u, 1920u);
    put32(alpha, 16u, 4u);    /* CClrGrey */
    put32(alpha, 0u, 1080u);
    put32(alpha, 20u, 2u);    /* CChrm444 */
    put32(alpha, 24u, 0u);    /* CWeavPlnr */
    put32(alpha, 36u, 8u);
    put32(alpha, 32u, 0u);    /* CLocHost */
    control_init_src_signal(bytes + DM_CONFIG_SRCSIGENV_OFFSET);
    control_init_tgt_signal(bytes + DM_CONFIG_TGTSIGENV_OFFSET);
    control_init_tone(bytes + DM_CONFIG_TMCTRL_OFFSET);
    return config;
}

void control_init_extensions(const void *config, void *extensions)
{
    const unsigned char *tone = (const unsigned char *)config + DM_CONFIG_TMCTRL_OFFSET;
    unsigned char *ext = (unsigned char *)extensions;
    unsigned int index;
    /* Nested TrimSet_t starts at +80: u16 count; TrimSets[2] at +8,
     * each Trim_t has four u16 scalar words and a u64 pointer; storage +40.
     * Each descriptor occupies 16 bytes with eight-byte pointer alignment. */
    put16(ext, DM_EXTENSIONS_MID_PQ_OFFSET, 0x7ffu);
    put16(ext, DM_EXTENSIONS_MIN_PQ_OFFSET, 0u);
    put16(ext, 88u, 2u);
    put16(ext, DM_EXTENSIONS_MAX_PQ_OFFSET, 0xfffu);
    put16(ext, DM_EXTENSIONS_TRIMSETS_OFFSET, 0u);
    put16(ext, 90u, 7u);
    put16(ext, 92u, 16u);
    put64(ext, 96u, (unsigned long long)(ext + 120u));
    put16(ext, 94u, 0u);
    put16(ext, 120u, 0xfffu);
    for (index = 1u; index < 7u; ++index)
        put16(ext, 120u + index * 2u,
              get32(tone, DM_TONE_CONTROL_DEFAULT2_OFFSET + index * 4u) -
              get32(tone, DM_TONE_CONTROL_CODEBIAS2_OFFSET + index * 4u));
    put16(ext, 104u, 3u);
    put64(ext, 112u, (unsigned long long)(ext + 358u));
    put16(ext, 110u, 0u);
    put16(ext, 106u, 4u);
    put16(ext, 108u, 16u);
    put16(ext, 358u, 0xfffu);
    for (index = 1u; index < 4u; ++index)
        put16(ext, 358u + index * 2u,
              get32(tone, DM_TONE_CONTROL_DEFAULT3_OFFSET + index * 4u) -
              get32(tone, DM_TONE_CONTROL_CODEBIAS3_OFFSET + index * 4u));
}
