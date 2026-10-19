#include "control_init.h"
#include "control_defaults.h"
typedef char control_pointer_64[(sizeof(void *) == 8) ? 1 : -1];
typedef char control_integer_32[(sizeof(unsigned int) == 4) ? 1 : -1];

static unsigned int read32(const void *object, unsigned int offset)
{
    const unsigned char *p = (const unsigned char *)object + offset;
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
           ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static void write16(void *object, unsigned int offset, unsigned short value)
{
    unsigned char *p = (unsigned char *)object + offset;
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void write32(void *object, unsigned int offset, unsigned int value)
{
    unsigned char *p = (unsigned char *)object + offset;
    unsigned int i;
    for (i = 0; i < 4; ++i) p[i] = (unsigned char)(value >> (i * 8));
}

static unsigned long long read_pointer(const void *object, unsigned int offset)
{
    const unsigned char *p = (const unsigned char *)object + offset;
    unsigned long long value = 0;
    unsigned int i;
    for (i = 0; i < sizeof(value); ++i) value |= (unsigned long long)p[i] << (i * 8);
    return value;
}

static void write_pointer(void *object, unsigned int offset, unsigned long long value)
{
    unsigned char *p = (unsigned char *)object + offset;
    unsigned int i;
    for (i = 0; i < sizeof(value); ++i) p[i] = (unsigned char)(value >> (i * 8));
}

void *control_init_memory(void *memory)
{
    write32(memory, DM_MMG_MDSEXTSIZE_OFFSET, DM_EXTENSIONS_BYTES);
    write32(memory, DM_MMG_DMCTXTSIZE_OFFSET, DM_CONTEXT_BYTES);
    write32(memory, DM_MMG_DMKSSIZE_OFFSET, DM_KERNEL_BYTES);
    return memory;
}

unsigned long long init_cp_mmg(void)
{
    unsigned char memory[DM_MMG_BYTES];
    unsigned int total;
    control_init_memory(memory);
    total = read32(memory, DM_MMG_DMCTXTSIZE_OFFSET) +
            read32(memory, DM_MMG_MDSEXTSIZE_OFFSET) +
            read32(memory, DM_MMG_DMKSSIZE_OFFSET);
    return (unsigned long long)total + CONTROL_CONTEXT_BYTES;
}

int control_register_memory(unsigned int size, unsigned int align, int in_device,
                            void *parameter, control_memory_callback callback,
                            void *memory, const struct control_memory_ops *ops)
{
    int count = (int)read32(memory, DM_MMG_REQNUM_OFFSET);
    unsigned char *item;
    if (count > 31) {
        ops->logger("\001" "0### ASSERTION FAILED %s: %s: %d: %s\n", "/mnt/fileroot/yao.liu/androidq/hardware/amlogic/dolby_vision/dv_source_code/libs/dm/dlb_dm_process/src/CdmMmg.c",
                    "RegMemReq", 38u, "hMmg->reqNum < MMG_REQ_ITEM_MAX");
        ops->dump_stack();
        count = (int)read32(memory, DM_MMG_REQNUM_OFFSET);
        if (count == 32) return -1;
    }
    if (align == 0) align = 4;
    /* This signed index is not bounds-checked. */
    item = (unsigned char *)memory + (long long)count * DM_MEMORY_REQUEST_BYTES;
    write32(item, DM_MEMORY_REQUEST_SIZE_OFFSET, size);
    write32(item, DM_MEMORY_REQUEST_ALIGN_OFFSET, align);
    write32(item, DM_MEMORY_REQUEST_INDEV_OFFSET, (unsigned int)in_device);
    write_pointer(item, DM_MEMORY_REQUEST_PCBPARAM_OFFSET, (unsigned long long)parameter);
    write_pointer(item, DM_MEMORY_REQUEST_PCBFUN_OFFSET, (unsigned long long)callback);
    write_pointer((unsigned char *)memory + (long long)count * (long long)sizeof(unsigned long long),
                  DM_MMG_MMGBUFS_OFFSET, 0);
    write32(memory, DM_MMG_REQNUM_OFFSET, (unsigned int)count + 1u);
    return 0;
}

int control_assign_memory(void *pointer_field, void *buffer)
{
    write_pointer(pointer_field, 0, (unsigned long long)buffer);
    return 0;
}

void *control_init_kernel(const void *config, void *kernel,
                          const struct control_memory_ops *ops)
{
    void *memory;
    static const unsigned int sizes[] = {
        0x530u, 0xd36u, 0xd34u, 0x532u, 0x1a1cu, 0x2222u, 0x2220u, 0x1a1eu
    };
    unsigned int i;
    if (!config || !kernel) return 0;
    memory = (void *)read_pointer(config, DM_CONFIG_HMMG_OFFSET);
    if (!memory) return 0;
    /* rowPitchNum is a ushort; its following padding remains untouched. */
    write16(kernel, DM_KERNEL_ROWPITCHNUM_OFFSET, 0x1000u);
    (void)control_register_memory(0x32a0000u, 16u, read32(config, 0x18u) != 0,
                                 (unsigned char *)kernel + DM_KERNEL_FRMBUF0_OFFSET,
                                 control_assign_memory, memory, ops);
    for (i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
        write16(kernel, sizes[i], 511u);
    return kernel;
}

void *control_init_dm(const void *config, void *dm)
{
    (void)config;
    write32(dm, DM_CONTEXT_DMCFGTOBEVALID_OFFSET, 0);
    write32(dm, DM_CONTEXT_DMCFGACTVALID_OFFSET, 0);
    write_pointer(dm, DM_CONTEXT_PDMCFGACT_OFFSET,
                  (unsigned long long)((unsigned char *)dm + DM_CONTEXT_DMCFG_OFFSET));
    write_pointer(dm, DM_CONTEXT_PDMCFGTOBE_OFFSET,
                  (unsigned long long)((unsigned char *)dm + DM_CONTEXT_DMCFG_OFFSET + DM_CONFIG_BYTES));
    write32(dm, DM_CONTEXT_MDSEXTFRCCMMT_OFFSET, 0);
    write32(dm, DM_CONTEXT_MDSEXTACTVALID_OFFSET, 0);
    return dm;
}

int init_cp(void *arena, const struct control_memory_ops *ops)
{
    unsigned char *context = (unsigned char *)arena;
    void *config = context + CONTROL_CONTEXT_DM_CFG_OFFSET;
    void *kernel = context + CONTROL_CONTEXT_BYTES;
    void *extensions = context + CONTROL_CONTEXT_BYTES + DM_KERNEL_BYTES;
    void *dm = (unsigned char *)extensions + DM_EXTENSIONS_BYTES;
    write32(context, CONTROL_CONTEXT_LAST_INPUT_FORMAT_OFFSET, 0xffffffffu);
    write32(context, CONTROL_CONTEXT_LAST_OUTPUT_FORMAT_OFFSET, 0xffffffffu);
    write32(context, CONTROL_CONTEXT_SUPPORT_EL_OFFSET, 0);
    write32(context, CONTROL_CONTEXT_LAST_GRAPHIC_ON_OFFSET, 0);
    write32(context, CONTROL_CONTEXT_DOVI2HDR10_NOMAPPING_OFFSET, 1);
    write_pointer(context, CONTROL_CONTEXT_H_KS_OFFSET, (unsigned long long)kernel);
    write_pointer(context, CONTROL_CONTEXT_H_MDS_EXT_OFFSET, (unsigned long long)extensions);
    write_pointer(context, CONTROL_CONTEXT_H_DM_OFFSET, (unsigned long long)dm);
    (void)control_init_config(0, context + CONTROL_CONTEXT_MMG_OFFSET, config);
    write32(config, DM_CONFIG_SRCSIGENV_OFFSET + DM_SOURCE_SIGNAL_EOTF_OFFSET, 1);
    if (!control_init_kernel(config, kernel, ops)) return -1;
    dm = (void *)read_pointer(context, CONTROL_CONTEXT_H_DM_OFFSET);
    if (!control_init_dm(config, dm)) return -1;
    extensions = (void *)read_pointer(context, CONTROL_CONTEXT_H_MDS_EXT_OFFSET);
    control_init_extensions(config, extensions);
    control_init_tone((unsigned char *)config + DM_CONFIG_TMCTRL_OFFSET);
    return 0;
}

int destroy_cp(void *arena)
{
    (void)arena;
    return 0;
}

void set_dovi2hdr10_mapping(void *context)
{
    write32(context, CONTROL_CONTEXT_DOVI2HDR10_NOMAPPING_OFFSET, 0);
}

void set_support_el(void *context, int enabled)
{
    write32(context, CONTROL_CONTEXT_SUPPORT_EL_OFFSET, enabled != 0);
}
