/* Read spatial resampling filters and their prediction parameters. */
#include "resampling_filters.h"
#include "bitstream.h"

typedef unsigned char byte;
typedef unsigned int u32;
typedef long long offset64;
typedef void (*log_fn)(const char *format, ...);
typedef char require_uint32[(sizeof(u32) == 4) ? 1 : -1];
typedef char require_int32[(sizeof(int) == 4) ? 1 : -1];
typedef char require_offset64[(sizeof(offset64) == 8) ? 1 : -1];
typedef char require_pointer64[(sizeof(void *) == 8 && sizeof(log_fn) == 8) ? 1 : -1];

static void copy_bytes(byte *to, const byte *from, u32 size)
{
    while (size-- != 0)
        *to++ = *from++;
}

static byte *read_pointer(const byte *p)
{
    byte *value;
    copy_bytes((byte *)&value, p, 8);
    return value;
}

static log_fn read_log(const byte *p)
{
    log_fn value;
    copy_bytes((byte *)&value, p, 8);
    return value;
}

static u32 read32(const byte *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) |
           ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void write32(byte *p, u32 value)
{
    p[0] = (byte)value;
    p[1] = (byte)(value >> 8);
    p[2] = (byte)(value >> 16);
    p[3] = (byte)(value >> 24);
}

static int signed32(u32 value)
{
    return (value & 0x80000000u) ? -1 - (int)~value : (int)value;
}

int rpu_data_chroma_resampling_filter_1D_exp_read(
    void *decoder, int partition, int component, int direction)
{
    byte *rdc = (byte *)decoder;
    byte *root = read_pointer(rdc);
    byte *context = read_pointer(rdc + 8);
    byte *payload = read_pointer(root + 8);
    struct bitstream *bs = (struct bitstream *)(rdc + 0x40);
    offset64 combination = (offset64)component * 2 + direction;
    offset64 coefficient = (offset64)component * 0x50 + (offset64)direction * 0x28;
    byte *counter = context + 0x2d8 + combination * 4;
    byte *part = payload + (offset64)partition * 0x7038;
    byte *flag = part + 0x649c + coefficient;
    log_fn log = read_log(rdc + 0x315c8);
    u32 severity = read32(rdc + 0x315d0);
    byte *header;
    u32 count, i;

    if (signed32(read32(counter)) <= 0) {
        *flag = 0;
    } else {
        *flag = (byte)read_u_v(1, bs);
        if (*flag != 0) {
            int predicted = 0;
            if (signed32(read32(counter)) > 1) {
                byte delta = (byte)(read_ue_v(bs) + 1u);
                part[0x64c0 + coefficient] = delta;
                predicted = signed32((u32)partition - delta);
            }
            if (*flag == 1 && (predicted >= partition || predicted < 0)) {
                if (log != (log_fn)0 && severity > 2u)
                    log("ERROR: Partition %d, cmp %d, dir %d:  pred_part_idx_resampling1D shall be in the range [0 .. %d] but is %d\n",
                        partition, component, direction,
                        signed32((u32)partition - 1u), predicted);
                return -2;
            }
            write32(part + 0x6f54 + combination * 4,
                    read32(payload + (offset64)predicted * 0x7038 +
                           0x6f54 + combination * 4));
            return 0;
        }
    }

    /* Reload rpu_data/header after reading the prediction. */
    root = read_pointer(rdc);
    header = read_pointer(root);
    payload = read_pointer(root + 8);
    count = header[direction == 0 ? 0x20 : 0x1f];
    for (i = 0; i < count; ++i) {
        u32 denom = header[0x0c];
        u32 value = read_u_v(denom + 1u, bs);
        u32 sign = value & (1u << (denom & 31u));
        write32(payload + (offset64)partition * 0x7038 + 0x64a0 +
                coefficient + (offset64)i * 4, (0u - sign) | value);
    }
    write32(part + 0x6f54 + combination * 4, (u32)(coefficient + 0x64a0));
    write32(counter, read32(counter) + 1u);
    return 0;
}

int rpu_data_spatial_resampling_filter_exp_read(
    void *decoder, int pivot, int direction, int phases)
{
    byte *rdc = (byte *)decoder;
    byte *root = read_pointer(rdc);
    byte *header = read_pointer(root);
    byte *payload = read_pointer(root + 8);
    struct bitstream *bs = (struct bitstream *)(rdc + 0x40);
    u32 taps = direction == 0 ? 8u : 6u;
    int phase;
    u32 i;
    offset64 start = 0x6544 + ((offset64)direction * 6 + pivot) * 0x20;

    for (phase = 0; phase < phases; ++phase) {
        byte *coeff = payload + start + (offset64)phase * 0x60;
        for (i = 0; i < taps; ++i) {
            write32(coeff + i * 4u, (u32)read_se_v(bs));
            write32(coeff + 0x180 + i * 4u, read_u_v(header[0x0b], bs));
        }
    }
    return 0;
}
