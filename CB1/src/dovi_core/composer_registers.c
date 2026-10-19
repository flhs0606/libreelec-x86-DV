/* Pack the composer configuration and frame dimensions into registers. */
#include "composer_registers.h"

typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned char byte;
typedef char require_u32[(sizeof(u32) == 4) ? 1 : -1];
typedef char require_u64[(sizeof(u64) == 8) ? 1 : -1];

static u32 read32(const byte *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void write32(byte *p, u32 word)
{
    p[0] = (byte)word; p[1] = (byte)(word >> 8);
    p[2] = (byte)(word >> 16); p[3] = (byte)(word >> 24);
}

static void copy32(byte *out, const byte *in)
{
    write32(out, read32(in));
}

static void polynomial(const byte *in, byte *out, u32 channel, u32 offset, u32 rows)
{
    u32 i;
    for (i = 0; i < rows * 3u; ++i) {
        u32 at = channel * 96u + i * 4u;
        u32 shift = read32(in + COMPOSER_CONFIG_COEFFICIENT_LOG2_DENOM_OFFSET) & 31u;
        u32 word = (read32(in + COMPOSER_CONFIG_POLY_COEF_INT_OFFSET + at) << shift) |
                    read32(in + COMPOSER_CONFIG_POLY_COEF_OFFSET + at);
        write32(out + offset + i * 4u, word & 0x3fffffffu);
    }
}

static void non_linear(const byte *in, byte *out, u32 channel, u32 offset)
{
    u32 i;
    copy32(out + offset, in + COMPOSER_CONFIG_NLQ_OFFSET_OFFSET + channel * 4u);
    for (i = 0; i < 3u; ++i) {
        u32 at = channel * 12u + i * 4u;
        u32 shift = read32(in + COMPOSER_CONFIG_COEFFICIENT_LOG2_DENOM_OFFSET) & 31u;
        u32 word = (read32(in + COMPOSER_CONFIG_NLQ_COEFF_INT_OFFSET + at) << shift) |
                    read32(in + COMPOSER_CONFIG_NLQ_COEFF_OFFSET + at);
        write32(out + offset + 4u + i * 4u, word & 0xffffffu);
    }
}

static void mmr(const byte *in, byte *out, u32 channel, u32 offset)
{
    u32 i;
    for (i = 0; i < 22u; ++i) {
        u32 at = (channel - 1u) * 88u + i * 4u;
        u32 integer = read32(in + COMPOSER_CONFIG_MMR_COEF_INT_OFFSET + at);
        u64 wide = (u64)integer;
        u32 shift = read32(in + COMPOSER_CONFIG_COEFFICIENT_LOG2_DENOM_OFFSET) & 63u;
        if (integer & 0x80000000u) wide |= 0xffffffff00000000ull;
        wide = (wide << shift) | read32(in + COMPOSER_CONFIG_MMR_COEF_OFFSET + at);
        write32(out + offset + i * 8u, (u32)wide);
        write32(out + offset + i * 8u + 4u, (u32)(wide >> 32) & 255u);
    }
}

static void chroma(const byte *in, byte *out, u32 channel,
                   u32 mapping, u32 pivots, u32 order, u32 poly, u32 mmr_at,
                   u32 mmr_order, u32 nlq)
{
    u32 i, word = 0;
    copy32(out + mapping, in + COMPOSER_CONFIG_MAPPING_IDC_OFFSET + channel * 4u);
    copy32(out + pivots, in + COMPOSER_CONFIG_NUM_PIVOTS_OFFSET + channel * 4u);
    for (i = 0; i < 2u; ++i) {
        const byte *p = in + COMPOSER_CONFIG_PIVOT_VALUE_OFFSET + channel * 36u + i * 8u;
        write32(out + pivots + 4u + i * 4u, read32(p) | (read32(p + 4) << 10));
    }
    copy32(out + pivots + 12u, in + COMPOSER_CONFIG_PIVOT_VALUE_OFFSET + channel * 36u + 16u);
    for (i = 0; i < 4u; ++i)
        word |= read32(in + COMPOSER_CONFIG_POLY_ORDER_OFFSET + channel * 32u + i * 4u) << (i * 2u);
    write32(out + order, word);
    polynomial(in, out, channel, poly, 4u);
    mmr(in, out, channel, mmr_at);
    copy32(out + mmr_order, in + COMPOSER_CONFIG_MMR_ORDER_OFFSET + (channel - 1u) * 4u);
    non_linear(in, out, channel, nlq);
}

unsigned short comp_cfg_2_comp_reg(const void *composer, void *registers,
                                 int width, int height)
{
    const byte *in = (const byte *)composer;
    byte *out = (byte *)registers;
    u32 i, word = 0;
    write32(out + COMPOSER_REGISTER_COMPOSER_MODE_OFFSET,
            in[COMPOSER_CONFIG_DISABLE_RESIDUAL_FLAG_OFFSET] != 0);
    write32(out + COMPOSER_REGISTER_VDR_RESOLUTION_OFFSET, (u32)width | ((u32)height << 13));
    write32(out + COMPOSER_REGISTER_BIT_DEPTH_OFFSET,
            (read32(in + COMPOSER_CONFIG_RPU_EL_BIT_DEPTH_OFFSET) << 4) |
            (read32(in + COMPOSER_CONFIG_RPU_VDR_BIT_DEPTH_OFFSET) << 8) |
            read32(in + COMPOSER_CONFIG_RPU_BL_BIT_DEPTH_OFFSET));
    copy32(out + COMPOSER_REGISTER_COEFFICIENT_LOG2_DENOMINATOR_OFFSET,
           in + COMPOSER_CONFIG_COEFFICIENT_LOG2_DENOM_OFFSET);
    copy32(out + COMPOSER_REGISTER_BL_NUM_PIVOTS_Y_OFFSET,
           in + COMPOSER_CONFIG_NUM_PIVOTS_OFFSET);
    for (i = 0; i < 4u; ++i) {
        const byte *p = in + COMPOSER_CONFIG_PIVOT_VALUE_OFFSET + i * 8u;
        write32(out + COMPOSER_REGISTER_BL_PIVOT_OFFSET + i * 4u,
                read32(p) | (read32(p + 4) << 10));
    }
    copy32(out + COMPOSER_REGISTER_BL_PIVOT_OFFSET + 16u,
           in + COMPOSER_CONFIG_PIVOT_VALUE_OFFSET + 32u);
    for (i = 0; i < 8u; ++i)
        word |= read32(in + COMPOSER_CONFIG_POLY_ORDER_OFFSET + i * 4u) << (i * 2u);
    write32(out + COMPOSER_REGISTER_BL_ORDER_OFFSET, word);
    polynomial(in, out, 0, COMPOSER_REGISTER_BL_COEFFICIENT_Y_OFFSET, 8u);
    non_linear(in, out, 0, COMPOSER_REGISTER_EL_NLQ_OFFSET_Y_OFFSET);
    chroma(in, out, 1u, COMPOSER_REGISTER_MAPPING_IDC_U_OFFSET,
           COMPOSER_REGISTER_BL_NUM_PIVOTS_U_OFFSET, COMPOSER_REGISTER_BL_ORDER_U_OFFSET,
           COMPOSER_REGISTER_BL_COEFFICIENT_U_OFFSET, COMPOSER_REGISTER_MMR_COEFFICIENT_U_OFFSET,
           COMPOSER_REGISTER_MMR_ORDER_U_OFFSET, COMPOSER_REGISTER_EL_NLQ_OFFSET_U_OFFSET);
    chroma(in, out, 2u, COMPOSER_REGISTER_MAPPING_IDC_V_OFFSET,
           COMPOSER_REGISTER_BL_NUM_PIVOTS_V_OFFSET, COMPOSER_REGISTER_BL_ORDER_V_OFFSET,
           COMPOSER_REGISTER_BL_COEFFICIENT_V_OFFSET, COMPOSER_REGISTER_MMR_COEFFICIENT_V_OFFSET,
           COMPOSER_REGISTER_MMR_ORDER_V_OFFSET, COMPOSER_REGISTER_EL_NLQ_OFFSET_V_OFFSET);
    return 0;
}
