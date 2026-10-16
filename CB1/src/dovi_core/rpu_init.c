/* Initialize the decoder arena using byte access for unaligned pointer fields. */
#include "rpu_init.h"

typedef char init_pointer_is_eight_bytes[(sizeof(void *) == 8) ? 1 : -1];
typedef char init_u32_is_four_bytes[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char init_u64_is_eight_bytes[(sizeof(unsigned long long) == 8) ? 1 : -1];

static const unsigned int init_level_limits[16][5] = {
    {1u, 1u, 9u, 2u, 0u},
    {4u, 4u, 11u, 2u, 3u},
    {8u, 8u, 17u, 2u, 7u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}, {0u, 0u, 0u, 0u, 0u},
    {0u, 0u, 0u, 0u, 0u}
};

static unsigned int init_get32(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static void init_put32(unsigned char *p, unsigned int value)
{
    unsigned int i;
    for (i = 0; i < 4; ++i)
        p[i] = (unsigned char)(value >> (i * 8));
}

static void init_put_pointer(unsigned char *p, const void *value)
{
    unsigned long long address = (unsigned long long)value;
    unsigned int i;
    for (i = 0; i < 8; ++i)
        p[i] = (unsigned char)(address >> (i * 8));
}

static void init_zero(unsigned char *p, unsigned int size)
{
    volatile unsigned char *destination = p;
    unsigned int i;
    for (i = 0; i < size; ++i)
        destination[i] = 0;
}

int rpu_decoder_init(void *decoder, const unsigned char *flags)
{
    unsigned char *rdc = (unsigned char *)decoder;
    unsigned int payload_only, buffers, i, max_prev, partitions;
    const unsigned int *limits;
    unsigned char *buffer, *levels, *context, *data, *header, *dm, *payload;

    init_zero(rdc, 0x316e8u);
    payload_only = init_get32(flags + 12);
    init_put32(rdc + 0x30, payload_only);
    init_put32(rdc + 0x34, init_get32(flags + 20));
    init_put32(rdc + 0x2c, init_get32(flags + 8));
    buffers = payload_only == 0 ? 64u : 1u;
    init_put32(rdc + 0x38, buffers);

    buffer = rdc + 0x316e8;
    levels = buffer + buffers * 0x1000u;
    init_put_pointer(rdc + 0x10, buffer);
    init_put_pointer(rdc + 0x18, levels);
    /* Clear precisely 4100 bytes, even with 64 buffers. */
    init_zero(buffer, 0x1004u);
    context = levels + buffers * 4u;
    init_put_pointer(rdc + 0x08, context);
    init_zero(context, 0x87cu);
    init_put32(rdc + 0x78, 0);
    init_put32(rdc + 0x7c, 0);
    init_put32(rdc + 0x80, 0);

    limits = init_level_limits[init_get32(flags + 4)];
    partitions = limits[0] * limits[1];
    max_prev = limits[4];
    init_put32(context + 0x66c, partitions);
    init_put32(context + 0x670, limits[2]);
    init_put32(context + 0x674, limits[3]);
    init_put32(context + 0x678, max_prev);

    data = context + 0x87c;
    init_put_pointer(rdc, data);
    init_zero(data, 0x18u);
    header = data + 0x18;
    init_put_pointer(data, header);
    init_zero(header, 0x136u);
    dm = header + 0x136;
    init_put_pointer(data + 0x10, dm);
    init_zero(dm, 0x10744u);
    payload = dm + 0x10744;
    init_put_pointer(rdc + 0x315d8, payload);
    /* Fixed two-payload clear; larger partition arenas retain caller bytes. */
    init_zero(payload, 0xe070u);

    for (i = 1; i < max_prev + 2u; ++i) {
        unsigned int units = i * partitions;
        init_put_pointer(rdc + 0x315d8 + i * 8u,
                            payload + (unsigned long long)units * 0x7038u);
    }
    for (i = 0; i < max_prev + 1u; ++i) {
        unsigned int units = i * partitions;
        init_put_pointer(rdc + 0x31658 + i * 8u,
                            payload + (unsigned long long)units * 0x7038u);
    }
    init_put_pointer(rdc + 0x316d0,
                        payload + (unsigned long long)((max_prev + 1u) * partitions) * 0x7038u);

    init_put32(rdc + 0x315d4, payload_only == 0 ? 0u : 4u);
    init_put32(rdc + 0x20, payload_only == 0 ? buffers - 1u : 0u);
    init_put32(rdc + 0x24, 0);
    for (i = 0; i < buffers; ++i)
        init_put32(levels + i * 4u, 0);
    return 0;
}
