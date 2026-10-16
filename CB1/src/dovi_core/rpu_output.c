/* Process RPU buffers and publish queued decoder output. */
#include "rpu_output.h"
#include "bitstream.h"
#include "parser_diagnostics.h"

typedef unsigned char byte;
typedef unsigned int u32;
typedef char require_int32[(sizeof(int) == 4 && sizeof(u32) == 4) ? 1 : -1];
typedef char require_pointer64[(sizeof(void *) == 8 &&
                               sizeof(output_log_fn) == 8) ? 1 : -1];

static void copy_bytes(byte *dst, const byte *src, u32 size)
{
    while (size-- != 0) *dst++ = *src++;
}

static byte *pointer(const byte *src)
{
    byte *value;
    copy_bytes((byte *)&value, src, 8);
    return value;
}

static void set_pointer(byte *dst, const byte *value)
{
    copy_bytes(dst, (const byte *)&value, 8);
}

static output_log_fn logger(const byte *decoder)
{
    output_log_fn value;
    copy_bytes((byte *)&value, decoder + 0x315c8, 8);
    return value;
}

static u32 read32(const byte *src)
{
    return (u32)src[0] | ((u32)src[1] << 8) |
           ((u32)src[2] << 16) | ((u32)src[3] << 24);
}

static void write32(byte *dst, u32 value)
{
    dst[0] = (byte)value;
    dst[1] = (byte)(value >> 8);
    dst[2] = (byte)(value >> 16);
    dst[3] = (byte)(value >> 24);
}

static int signed32(u32 value)
{
    return value > 0x7fffffffu ? -1 - (int)~value : (int)value;
}

static void publish(byte *decoder, void **header, void **payload,
                    int *payload_size, void **dm_payload, int *dm_payload_size)
{
    byte *data = pointer(decoder);
    *header = pointer(data);
    *payload = pointer(pointer(decoder) + 8);
    data = pointer(pointer(decoder));
    *payload_size = (int)((u32)data[0x133] * data[0x134] * 0x7038u);
    *dm_payload = pointer(pointer(decoder) + 16);
    *dm_payload_size = 0x10744;
}

int rpu_decoder_get_output_with_ops(void *opaque, void **header,
    void **payload, int *payload_size, void **dm_payload, int *dm_payload_size,
    const struct rpu_output_ops *ops)
{
    byte *decoder = (byte *)opaque;
    u32 index;
    int result;
    if (read32(decoder + 0x315c0) != 0 &&
        read32(decoder + 0x315bc) != 0 && read32(decoder + 0x315b8) != 0) {
        publish(decoder, header, payload, payload_size, dm_payload, dm_payload_size);
        write32(decoder + 0x315b8, 0);
        return 1;
    }
    index = read32(decoder + 0x24);
    if (index == read32(decoder + 0x20) && read32(decoder + 0x30) == 0 &&
        read32(decoder + 0x28) == 0)
        return -20;
    if (read32(pointer(decoder + 0x18) + index * 4u) == 0)
        return read32(decoder + 0x28) != 0 ? -21 : -20;
    if (read32(decoder + 0x315d0) > 6u) {
        ops->dump_buffer_info(decoder);
        index = read32(decoder + 0x24);
    }
    result = ops->process_buffer(decoder, pointer(decoder + 0x10) +
        (unsigned long long)index * 4096u,
        signed32(read32(pointer(decoder + 0x18) + index * 4u)),
        header, payload, payload_size, dm_payload, dm_payload_size);
    index = read32(decoder + 0x24);
    write32(pointer(decoder + 0x18) + index * 4u, 0);
    index = read32(decoder + 0x24);
    write32(decoder + 0x24, (read32(decoder + 0x38) - 1u) & (index + 1u));
    return result;
}

static int parse_error(output_log_fn log, u32 severity, int error, u32 line)
{
    if (log != 0 && severity > 2u)
        log("%s+%d: %s\n",
            "/mnt/fileroot/yao.liu/androidq/hardware/amlogic/dolby_vision/dv_source_code/libs/vdr_rpu/src/rpu_decoder.c",
            line, rpu_error_code_2_str(error));
    return error;
}

/* Identity fallback writes only the first partition. */
static void identity_payload(byte *header, byte *payload)
{
    u32 component, pivot;
    for (component = 0; component < 3; ++component) {
        for (pivot = 0; (int)pivot < (int)header[0x25 + component] - 1; ++pivot) {
            byte *poly = payload + 0x5c + component * 0x30cu + pivot * 0x34u;
            payload[component * 15u + pivot] = 0;
            payload[0x2d + component * 15u + pivot] = 0;
            poly[0] = 1;
            write32(poly + 0xc, 0);
            write32(poly + 0x20, 0);
            write32(poly + 0x10, 1);
            write32(poly + 0x24, 0);
        }
    }
    if (header[0x16] == 0) {
        for (component = 0; component < 3; ++component) {
            byte *nlq = payload + 0x6874u + component * 0x1cu;
            payload[0x6844 + component] = 0;
            nlq[0] = 0;
            nlq[1] = 0;
            write32(nlq + 4, 1);
            write32(nlq + 8, 0);
            write32(nlq + 0xc, 0);
            write32(nlq + 0x10, 0);
            write32(nlq + 0x14, 0);
            write32(nlq + 0x18, 0);
        }
    }
}

int rpu_decoder_process_buffer_with_ops(void *opaque, byte *buffer, int size,
    void **header, void **payload, int *payload_size, void **dm_payload,
    int *dm_payload_size, const struct rpu_process_ops *ops)
{
    byte *decoder = (byte *)opaque;
    output_log_fn log = logger(decoder);
    u32 severity = read32(decoder + 0x315d0), length;
    byte *data, *rh, *old;
    int result;
    set_pointer(pointer(decoder) + 8, pointer(decoder + 0x316d0));
    bitstream_init((struct bitstream *)(decoder + 0x40), buffer, (u32)size);
    result = ops->header_read(decoder);
    if (result < 0) return parse_error(log, severity, result, 0x225);
    length = (u32)result;
    result = ops->validate_level(pointer(decoder), logger(decoder), read32(decoder + 0x315d0));
    if (result < 0) return result;
    result = ops->validate_spatial_filter(pointer(decoder), logger(decoder), read32(decoder + 0x315d0));
    if (result < 0) return result;
    data = pointer(decoder);
    rh = pointer(data);
    if (signed32(read32(pointer(decoder + 8) + 0x66c)) < (int)((u32)rh[0x133] * rh[0x134])) {
        /* This diagnostic call omits the value for its %d conversion. */
        if (log != 0 && severity > 2u)
            log("ERROR: Payload buffer too small for %d partitions\n");
        return -2;
    }
    if (rh[0] == 2 && rh[0x19] == 0) {
        result = ops->payload_read(decoder);
        if (result < 0) return parse_error(log, severity, result, 0x236);
        length += (u32)result;
        data = pointer(decoder);
    }
    result = ops->validate_profile(data, logger(decoder), read32(decoder + 0x315d0));
    if (result < 0) return result;
    if (read32(decoder + 0x34) == 0 && ops->detect_mel(decoder) != 0)
        pointer(pointer(decoder))[0x16] = 1;
    data = pointer(decoder);
    rh = pointer(data);
    if (rh[0x18] != 0) {
        result = ops->dm_payload_read(decoder);
        if (result < 0) return parse_error(log, severity, result, 0x243);
        length += (u32)result;
        data = pointer(decoder);
        rh = pointer(data);
    }
    if (rh[0x19] == 0) {
        byte *slot = decoder + 0x31658 + rh[0x1b] * 8u;
        old = pointer(slot);
        set_pointer(slot, pointer(decoder + 0x316d0));
        set_pointer(decoder + 0x316d0, old);
        rh = pointer(data);
        set_pointer(data + 8, pointer(decoder + 0x31658 + rh[0x1b] * 8u));
    } else if (((read32(decoder + 0x7c) >> (rh[0x1b] & 31u)) & 1u) == 0) {
        identity_payload(rh, pointer(data + 8));
    } else {
        set_pointer(data + 8, pointer(decoder + 0x31658 + rh[0x1a] * 8u));
    }
    if (pointer(decoder + 0x316d0) == 0 || pointer(pointer(decoder) + 8) == 0) {
        if (log != 0 && severity > 2u) log("Error: payload buffer not allocated\n");
        return -2;
    }
    publish(decoder, header, payload, payload_size, dm_payload, dm_payload_size);
    return signed32(length);
}
