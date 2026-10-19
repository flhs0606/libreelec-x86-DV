/* Read active or pending DM configuration and luminance bounds. */
#include "control_getters.h"
#include "control_abi_recovered.h"
#include "control_luminance.h"
#include "byte_utils.h"

typedef unsigned char byte;
typedef char getter_u16[(sizeof(unsigned short) == 2) ? 1 : -1];
typedef char getter_u32[(sizeof(unsigned int) == 4) ? 1 : -1];
typedef char getter_ptr64[(sizeof(void *) == 8) ? 1 : -1];

static unsigned int getter_read32(const byte *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static unsigned short getter_read16(const byte *p)
{
    return (unsigned short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

static const byte *getter_pointer(const byte *p)
{
    unsigned long long address = (unsigned long long)getter_read32(p) |
        ((unsigned long long)getter_read32(p + 4) << 32);
    return (const byte *)address;
}

int control_get_dm_cfg(void *config, const void *dm_handle)
{
    const byte *handle = (const byte *)dm_handle, *source;
    unsigned int pending;
    if (!handle)
        return -1;
    pending = getter_read32(handle + DM_CONTEXT_DMCFGTOBEVALID_OFFSET);
    if (pending == 0 && getter_read32(handle + DM_CONTEXT_DMCFGACTVALID_OFFSET) == 0)
        return -1;
    source = getter_pointer(handle + (pending != 0 ? DM_CONTEXT_PDMCFGTOBE_OFFSET : DM_CONTEXT_PDMCFGACT_OFFSET));
    if (!source)
        return -1;
    /* mem_cpy_byte copies forward even when the input and output overlap. */
    mem_cpy_byte(config, source, DM_CONFIG_BYTES);
    return 0;
}

int control_get_stb_cc(unsigned short *min_pq, unsigned int *min_linear,
                       unsigned short *max_pq, unsigned int *max_linear,
                       const void *dm_handle)
{
    const byte *handle = (const byte *)dm_handle;
    if (!handle || getter_read32(handle + DM_CONTEXT_DMCFGACTVALID_OFFSET) == 0)
        return -1;
    if (min_pq)
        *min_pq = getter_read16(handle + 0xa94);
    if (min_linear)
        *min_linear = control_pq12_to_l(getter_read16(handle + 0xa94));
    if (max_pq)
        *max_pq = getter_read16(handle + 0xa96);
    if (max_linear)
        *max_linear = control_pq12_to_l(getter_read16(handle + 0xa96));
    return 0;
}
