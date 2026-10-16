/* Parse RPU header fields and update decoder sequence state. */
#include "rpu_header.h"
#include "bitstream.h"

typedef unsigned char byte;
typedef unsigned int u32;
typedef char require_uint32[(sizeof(u32) == 4) ? 1 : -1];
typedef char require_int32[(sizeof(int) == 4) ? 1 : -1];
typedef char require_pointer64[(sizeof(void *) == 8 &&
                               sizeof(rpu_header_log_fn) == 8 &&
                               sizeof(rpu_header_event_fn) == 8) ? 1 : -1];

static void copy_bytes(byte *dest, const byte *src, u32 count)
{
    while (count-- != 0)
        *dest++ = *src++;
}

static u32 read32(const byte *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) |
           ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

static void write16(byte *p, u32 value)
{
    p[0] = (byte)value;
    p[1] = (byte)(value >> 8);
}

static void write32(byte *p, u32 value)
{
    p[0] = (byte)value;
    p[1] = (byte)(value >> 8);
    p[2] = (byte)(value >> 16);
    p[3] = (byte)(value >> 24);
}

static byte *read_pointer(const byte *p)
{
    byte *value;
    copy_bytes((byte *)&value, p, 8);
    return value;
}

static rpu_header_log_fn read_log(const byte *p)
{
    rpu_header_log_fn value;
    copy_bytes((byte *)&value, p, 8);
    return value;
}

static rpu_header_event_fn read_event(const byte *p)
{
    rpu_header_event_fn value;
    copy_bytes((byte *)&value, p, 8);
    return value;
}

static int signed32(u32 value)
{
    return (value & 0x80000000u) ? -1 - (int)~value : (int)value;
}

/* The diagnostic formats one unsigned-byte argument. */
static void unavailable_id_message(char *message, byte id)
{
    static const char prefix[] = "RPU ID ";
    static const char suffix[] = " not available";
    volatile char *zero = message;
    u32 i, pos = 0;
    for (i = 0; i < 256u; ++i)
        zero[i] = 0;
    for (i = 0; prefix[i] != 0; ++i)
        message[pos++] = prefix[i];
    if (id >= 100u)
        message[pos++] = (char)('0' + id / 100u);
    if (id >= 10u)
        message[pos++] = (char)('0' + (id / 10u) % 10u);
    message[pos++] = (char)('0' + id % 10u);
    for (i = 0; suffix[i] != 0; ++i)
        message[pos++] = suffix[i];
}

static void default_sequence(byte *header)
{
    header[0x14] = 0;
    header[0x15] = 0;
    header[0x07] = 0;
    header[0x08] = 0;
    header[0x09] = 0;
    header[0x0a] = 15;
    header[0x0d] = 1;
    header[0x0e] = 0;
    header[0x0f] = 10;
    header[0x10] = 10;
    header[0x11] = 14;
    header[0x12] = 0;
    header[0x16] = 1;
}

static void default_mapping(byte *header)
{
    u32 bl_depth = header[0x0f], el_depth;
    u32 limit;
    write16(header + 0x1c, 0);
    write16(header + 0x28, 0);
    header[0x25] = 2;
    header[0x26] = 2;
    header[0x27] = 2;
    if (bl_depth < 8u) {
        bl_depth = 8;
        header[0x0f] = 8;
    }
    write16(header + 0x48, 0);
    limit = ~(0xffffffffu << (bl_depth & 31u));
    el_depth = header[0x10];
    write16(header + 0x68, 0);
    header[0xf0] = 0;
    header[0xf1] = 2;
    write16(header + 0x2a, limit);
    write16(header + 0x4a, limit);
    write16(header + 0x6a, limit);
    if (el_depth < 8u) {
        el_depth = 8;
        header[0x10] = 8;
    }
    /* Exactly +0xf2; the next entry at +0xf4 is deliberately untouched. */
    write16(header + 0xf2, ~(0xffffffffu << (el_depth & 31u)));
    header[0x133] = 1;
    header[0x134] = 1;
}

static void read_pivots(byte *header, u32 count_offset, u32 array_offset,
                        u32 depth_offset, struct bitstream *bits)
{
    u32 i;
    for (i = 0; i < header[count_offset]; ++i) {
        u32 value = read_u_v(header[depth_offset], bits);
        write16(header + array_offset + i * 2u, value);
    }
}

int rpu_header_read(void *ctx)
{
    byte *decoder = (byte *)ctx;
    byte *header = read_pointer(read_pointer(decoder));
    struct bitstream *bits = (struct bitstream *)(decoder + 0x40);
    u32 initial_bits = bits->bits_read;
    rpu_header_log_fn log = read_log(decoder + 0x315c8);
    u32 log_level = read32(decoder + 0x315d0);
    rpu_header_event_fn event;
    u32 value, component;

    header[0] = (byte)read_u_v(6, bits);
    if (header[0] < 2u) {
        if (log != 0 && log_level > 2u)
            log("RPU Error: RPU type: %d not supported\n", (int)header[0]);
        return -3;
    }
    write16(header + 2, read_u_v(11, bits));
    if (header[0] != 2u)
        return signed32(bits->bits_read - initial_bits);

    header[4] = (byte)read_u_v(4, bits);
    header[5] = (byte)read_u_v(4, bits);
    header[6] = (byte)read_u_v(1, bits);
    if (header[6] != 0) {
        header[8] = (byte)read_u_v(1, bits);
        header[9] = (byte)read_u_v(2, bits);
        if (header[9] != 0) {
            if (log != 0 && log_level > 2u)
                log("RPU Error: Only fixed coefficient data type supported.\n");
            return -3;
        }
        header[0x0a] = (byte)read_ue_v(bits);
        if (header[8] != 0)
            header[0x0c] = (byte)(read_ue_v(bits) + 6u);
        header[0x0d] = (byte)read_u_v(2, bits);
        header[0x0e] = (byte)read_u_v(1, bits);
        if ((header[3] & 7u) == 0) {
            header[0x0f] = (byte)(read_ue_v(bits) + 8u);
            header[0x10] = (byte)(read_ue_v(bits) + 8u);
            header[0x11] = (byte)(read_ue_v(bits) + 8u);
            header[0x12] = (byte)read_u_v(1, bits);
            if (header[0x12] != 0) {
                header[0x13] = (byte)read_u_v(1, bits);
                if (header[9] == 0 && header[0x13] != 0)
                    header[0x0b] = (byte)(read_ue_v(bits) + 6u);
            }
            header[0x14] = (byte)read_u_v(3, bits);
            header[0x15] = (byte)read_u_v(1, bits);
            header[0x16] = (byte)read_u_v(1, bits);
            if (header[0x16] == 0 && header[9] == 0 && header[8] != 0)
                header[0x17] = (byte)(read_ue_v(bits) + 10u);
        }
        copy_bytes(decoder + 0x64, header + 7, 17);
        write32(decoder + 0x78, 1);
    } else if (read32(decoder + 0x78) == 0) {
        event = read_event(decoder + 0x316d8);
        if (event != 0)
            event(4, 0x103,
                  "No previous sequence header found, using default values.");
        default_sequence(header);
    } else {
        copy_bytes(header + 7, decoder + 0x64, 17);
    }

    header[0x18] = (byte)read_u_v(1, bits);
    header[0x19] = (byte)read_u_v(1, bits);
    if (header[0x19] != 0) {
        header[0x1a] = (byte)read_ue_v(bits);
        /* The mask uses the retained +0x1b, not the newly read +0x1a. */
        if (((read32(decoder + 0x7c) >> (header[0x1b] & 31u)) & 1u) == 0) {
            event = read_event(decoder + 0x316d8);
            if (event != 0) {
                char message[256];
                unavailable_id_message(message, header[0x1b]);
                event = read_event(decoder + 0x316d8);
                event(4, 0x104, message);
            }
            default_mapping(header);
        }
        return signed32(bits->bits_read - initial_bits);
    }

    header[0x1a] = 0;
    value = read_ue_v(bits);
    header[0x1b] = (byte)value;
    write32(decoder + 0x7c, read32(decoder + 0x7c) | (1u << (value & 31u)));
    header[0x1c] = (byte)read_ue_v(bits);
    header[0x1d] = (byte)read_ue_v(bits);
    if ((header[3] & 7u) == 0 && header[0x1d] != 0) {
        header[0x1e] = (byte)read_u_v(4, bits);
        if (header[0x1e] == 14u && header[0x1d] != 0) {
            if (header[0x1d] == 2u)
                header[0x1f] = (byte)(read_u_v(2, bits) * 2u + 4u);
            header[0x20] = (byte)(read_u_v(2, bits) * 2u + 4u);
        }
    }

    for (component = 0; component < 3u; ++component) {
        header[0x25 + component] = (byte)(read_ue_v(bits) + 2u);
        read_pivots(header, 0x25u + component, 0x28u + component * 0x20u,
                    0x0f, bits);
    }
    if ((header[3] & 7u) == 0 && header[0x12] != 0) {
        header[0xe8] = (byte)read_u_v(3, bits);
        if (header[0xe8] != 0) {
            header[0xe9] = (byte)read_u_v(3, bits);
            header[0xea] = (byte)read_u_v(3, bits);
            header[0xeb] = (byte)read_u_v(3, bits);
        }
        header[0xec] = (byte)read_u_v(3, bits);
        if (header[0xec] != 0) {
            header[0xed] = (byte)read_u_v(3, bits);
            header[0xee] = (byte)read_u_v(3, bits);
            header[0xef] = (byte)read_u_v(3, bits);
        }
    }
    if ((header[3] & 7u) == 0 && header[0x16] == 0) {
        header[0xf0] = (byte)read_u_v(3, bits);
        header[0xf1] = header[0x1d] == 2u ?
                      (byte)(read_ue_v(bits) + 2u) : 2;
        read_pivots(header, 0xf1, 0xf2, 0x10, bits);
        if (header[0x1d] != 0)
            header[0x132] = (byte)read_u_v(3, bits);
    }
    header[0x133] = (byte)(read_ue_v(bits) + 1u);
    header[0x134] = (byte)(read_ue_v(bits) + 1u);
    if ((int)header[0x133] + (int)header[0x134] - 1 > 1 && header[0] == 2u)
        header[0x135] = (byte)read_ue_v(bits);
    return signed32(bits->bits_read - initial_bits);
}
