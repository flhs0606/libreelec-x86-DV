/* Build HDR infoframes and fixed metadata from packed control objects. */
#include "control_infoframe.h"
#include "control_luminance.h"
#include "control_abi_recovered.h"

typedef unsigned char byte;
typedef char require_u16[(sizeof(unsigned short) == 2) ? 1 : -1];
typedef char require_u32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char require_ptr64[(sizeof(void *) == 8) ? 1 : -1];

static unsigned short read_u16(const byte *p)
{
    return (unsigned short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

static unsigned int read_u32(const byte *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static const byte *read_ptr(const byte *p)
{
    unsigned long long address = (unsigned long long)read_u32(p) |
                                 ((unsigned long long)read_u32(p + 4) << 32);
    return (const byte *)address;
}

static void write_le16(byte *p, unsigned int value)
{
    p[0] = (byte)value;
    p[1] = (byte)(value >> 8);
}

static void write_be16(byte *p, unsigned int value)
{
    p[0] = (byte)(value >> 8);
    p[1] = (byte)value;
}

int control_get_stb_tmm(unsigned short *min_pq, unsigned int *min,
                        unsigned short *max_pq, unsigned int *max,
                        const void *dm)
{
    const byte *h = (const byte *)dm, *cfg;
    if (h == 0 || read_u32(h + 4) == 0)
        return -1;
    if (min_pq != 0)
        *min_pq = read_u16(h + 0xa9c);
    if (min != 0) {
        cfg = read_ptr(h + 0x7b8);
        if (read_u32(cfg + 0x310) == 3u) {
            if (read_u32(cfg + 8) == 1u)
                *min = control_pq12_to_l(read_u16(h + 0xa9c));
            else
                *min = read_u32(cfg + 0x3b8);
        } else {
            *min = read_u32(read_ptr(h + 0x1128) + 0x2a4c);
        }
    }
    if (max_pq != 0)
        *max_pq = read_u16(h + 0xa9e);
    if (max != 0) {
        cfg = read_ptr(h + 0x7b8);
        if (read_u32(cfg + 0x310) == 3u) {
            if (read_u32(cfg + 8) == 1u)
                *max = control_pq12_to_l(read_u16(h + 0xa9e));
            else
                *max = read_u32(cfg + 0x3bc);
        } else {
            *max = read_u32(read_ptr(h + 0x1128) + 0x2a50);
        }
    }
    return 0;
}

void get_fixed_metadata(const void *ctx, void *dm)
{
    static const byte fixed[64] = {
        0x00,0x01,0x20,0x00,0x03,0x1f,0x06,0x91,
        0x20,0x00,0xfc,0x5b,0x04,0x43,0x20,0x00,
        0x01,0x0b,0xea,0x57,0x00,0x00,0x00,0x00,
        0x08,0x00,0x00,0x00,0x08,0x00,0x00,0x00,
        0x42,0xb9,0xfe,0xa3,0xfe,0xa3,0xfe,0xa3,
        0x42,0xb9,0xfe,0xa3,0xfe,0xa3,0xfe,0xa3,
        0x42,0xb9,0xff,0xff,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x0c,0x02,0x01,0x01
    };
    byte *p = (byte *)dm;
    unsigned int i;
    unsigned short min_pq, max_pq;
    for (i = 0; i < 64; ++i)
        p[i] = fixed[i];
    (void)control_get_stb_tmm(&min_pq, 0, &max_pq, 0,
                            read_ptr((const byte *)ctx + CONTROL_CONTEXT_H_DM_OFFSET));
    write_be16(p + 64, min_pq);
    write_be16(p + 66, max_pq);
    p[68] = 0;
    p[69] = 42;
    p[70] = 0;
}

int get_info_frame(const void *ctx, const void *target, int input_format,
                   const void *hdr10, const void *dm, void *infoframe)
{
    static const byte primaries[16] = {
        0x48,0x8a,0x08,0x39,0x34,0x21,0xaa,0x9b,
        0x96,0x19,0xfc,0x08,0x13,0x3d,0x42,0x40
    };
    byte *out = (byte *)infoframe;
    const byte *t = (const byte *)target, *h = (const byte *)hdr10;
    const byte *p = (const byte *)dm, *ext;
    unsigned int i, min, max;
    out[0] = 7;
    out[1] = 1;
    out[2] = 26;
    out[3] = 2;
    out[4] = 0;
    if (input_format == 1) {
        for (i = 0; i < 16; ++i)
            out[5 + i] = h[8 + i];
        write_le16(out + 21, read_u32(h + 4) / 10000u);
        write_le16(out + 23, read_u32(h));
        for (i = 0; i < 4; ++i)
            out[25 + i] = h[24 + i];
        return 0;
    }
    if (input_format != 0 && input_format != 2)
        return 0;
    for (i = 0; i < 16; ++i)
        out[5 + i] = primaries[i];
    if (input_format == 0 &&
        read_u32((const byte *)ctx + CONTROL_CONTEXT_DOVI2HDR10_NOMAPPING_OFFSET) != 0) {
        min = control_pq12_to_l((unsigned short)(((unsigned int)p[64] << 8) + p[65]));
        max = control_pq12_to_l((unsigned short)(((unsigned int)p[66] << 8) + p[67]));
        write_le16(out + 23, (unsigned int)(((unsigned long long)min * 10000u) >> 18));
        write_le16(out + 21, (unsigned int)(((unsigned long long)max * 10000u) >> 18));
        out[25] = out[26] = out[27] = out[28] = 0;
        for (i = 0; i < p[70]; ++i) {
            ext = p + CONTROL_METADATA_EXT_OFFSET + i * CONTROL_EXTENSION_BYTES;
            if (ext[4] == 6) {
                if (ext[5] != 0 || ext[6] != 0 || ext[7] != 0 || ext[8] != 0) {
                    out[21] = ext[6];
                    out[22] = ext[5];
                    out[23] = ext[8];
                    out[24] = ext[7];
                }
                out[25] = ext[10];
                out[26] = ext[9];
                out[27] = ext[12];
                out[28] = ext[11];
            }
        }
        return 0;
    }
    write_le16(out + 21, read_u32(t + CONTROL_TARGET_TMAX_OFFSET) / 10000u);
    write_le16(out + 23, read_u32(t + CONTROL_TARGET_TMIN_OFFSET));
    out[25] = out[26] = out[27] = out[28] = 0;
    return 0;
}
