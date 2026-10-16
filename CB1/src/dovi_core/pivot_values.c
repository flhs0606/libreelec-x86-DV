/* Accumulate RPU mapping and nonlinear-quantizer pivot positions. */
#include "pivot_values.h"

typedef char pivot_uint32_is_four_bytes[(sizeof(unsigned int) == 4) ? 1 : -1];

static unsigned int load16(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static void store16(unsigned char *p, unsigned int value)
{
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static int accumulate(unsigned char *header, unsigned int source,
                      unsigned int destination, unsigned int count,
                      unsigned int depth)
{
    unsigned int limit = 1u << (depth & 31u);
    unsigned int value = load16(header + source);
    unsigned int i;

    store16(header + destination, value);
    if ((limit & 0x80000000u) != 0u || value >= limit)
        return -13;
    for (i = 1; i < count; ++i) {
        value = (load16(header + source + i * 2u) +
                 load16(header + destination + (i - 1u) * 2u)) & 0xffffu;
        store16(header + destination + i * 2u, value);
        if (value >= limit)
            return -13;
    }
    return 0;
}

int assign_pivot_values(struct pivot_rpu_root *root)
{
    unsigned char *header = root->header;
    unsigned int component;

    for (component = 0; component < 3u; ++component) {
        int result = accumulate(header, 0x28u + component * 0x20u,
                                0x88u + component * 0x20u,
                                header[0x25u + component], header[0x0fu]);
        if (result != 0)
            return result;
    }
    return 0;
}

int assign_nlq_pivot_values(struct pivot_rpu_root *root)
{
    unsigned char *header = root->header;
    return accumulate(header, 0xf2u, 0x112u, header[0xf1u], header[0x10u]);
}
