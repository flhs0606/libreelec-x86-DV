#include "control_path.h"
#include "control_target.h"
#include "control_infoframe.h"

typedef char control_path_pointer_64[(sizeof(void *) == 8) ? 1 : -1];
static unsigned int read32(const void *object, unsigned int offset)
{
    const unsigned char *p = (const unsigned char *)object + offset;
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) |
        ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}
static void write32(void *object, unsigned int offset, unsigned int value)
{
    unsigned char *p = (unsigned char *)object + offset;
    unsigned int i;
    for (i = 0; i < 4; ++i) p[i] = (unsigned char)(value >> (i * 8));
}
static void write_pointer(void *object, unsigned int offset, const void *value)
{
    unsigned char *p = (unsigned char *)object + offset;
    unsigned long long address = (unsigned long long)value;
    unsigned int i;
    for (i = 0; i < 8; ++i) p[i] = (unsigned char)(address >> (i * 8));
}
static void copy_bytes(void *destination, const void *source, unsigned int count)
{
    unsigned char *d = (unsigned char *)destination;
    const unsigned char *s = (const unsigned char *)source;
    unsigned int i;
    for (i = 0; i < count; ++i) d[i] = s[i];
}
static void zero_bytes(void *destination, unsigned int count)
{
    unsigned char *d = (unsigned char *)destination;
    unsigned int i;
    for (i = 0; i < count; ++i) d[i] = 0;
}
static unsigned int extension_length(const unsigned char *extension)
{
    return ((unsigned int)extension[0] << 24) |
        ((unsigned int)extension[1] << 16) |
        ((unsigned int)extension[2] << 8) | (unsigned int)extension[3];
}
static void pack_byte(unsigned char **destination, unsigned int *word,
                      unsigned int *shift, unsigned char value)
{
    *word |= (unsigned int)value << *shift;
    *shift += 8;
    if (*shift == 32) {
        write32(*destination, 0, *word);
        *destination += 4;
        *shift = 0;
        *word = 0;
    }
}

/* Size is calculated before any raw or extension payload output is written.
 * Overflow retains those previous output bytes after clearing availability.
 * The first metadata byte shares word0 with the serialized length; subsequent
 * bytes start with scene_refresh_flag in word1.
 */
static void output_metadata(void *output, const unsigned char *metadata,
                            const struct control_path_ops *ops)
{
    unsigned int count = metadata[70], size = 71, i, j, words, word, shift;
    unsigned char *raw = (unsigned char *)output + CONTROL_OUTPUT_MD_REG3_OFFSET;
    unsigned char *ext = (unsigned char *)output + CONTROL_OUTPUT_EXT_MD_OFFSET;
    unsigned char *destination;
    for (i = 0; i < count; ++i)
        size += 5 + extension_length(metadata + 71 + i * 261);
    /* AArch64 signed divide-by-four lowering rounds toward zero. */
    words = (unsigned int)((int)(size + 2) / 4 + 1);
    write32(raw, 2048, words);
    if (words > 512) {
        ops->logger("error!! core3 md overflow %d!!\n", words);
        write32(raw, 2048, 512);
        return;
    }
    write32(raw, 0, (size << 8) | metadata[0]);
    destination = raw + 4;
    word = metadata[1];
    shift = 8;
    for (j = 2; j < 71; ++j) pack_byte(&destination, &word, &shift, metadata[j]);
    for (i = 0; i < (unsigned int)metadata[70]; ++i) {
        const unsigned char *record = metadata + 71 + i * 261;
        unsigned int length = extension_length(record), limit = 0;
        unsigned int offset = 0, mask = 0;
        for (j = 0; j < 5; ++j) pack_byte(&destination, &word, &shift, record[j]);
        switch (record[4]) {
        case 1: limit = 6; offset = 4; mask = 1; break;
        case 2: limit = 14; offset = 10; mask = 2; break;
        case 4: limit = 4; offset = 24; mask = 4; break;
        case 5: limit = 8; offset = 28; mask = 8; break;
        case 6: limit = 8; offset = 36; mask = 16; break;
        case 255: limit = 6; offset = 44; mask = 0x80000000u; break;
        default: break;
        }
        if (mask != 0) {
            write32(ext, 0, read32(ext, 0) | mask);
            if ((int)length < (int)limit) limit = length;
            copy_bytes(ext + offset, record + 5, limit);
        }
        if ((int)length > 0)
            for (j = 0; j < length; ++j)
                pack_byte(&destination, &word, &shift, record[5 + j]);
    }
    if (shift != 0) write32(destination, 0, word);
}

int control_path_with_ops(struct control_path_storage *storage,
    const struct control_path_ops *ops, int input_format, int output_format,
    const char *composition, int composition_size, const char *metadata,
    int metadata_size, int priority, int bit_depth, int chroma_format,
    int yuv_range, int graphics_min, int graphics_max, int target_min,
    int target_max, int no_el, const void *hdr10, void *output)
{
    unsigned char source[48], graphics[64], target[88], infoframe[29];
    unsigned char *context = (unsigned char *)storage->arena;
    unsigned char *cli = (unsigned char *)storage->cli;
    unsigned char *current = (unsigned char *)storage->metadata1;
    unsigned char *previous = (unsigned char *)storage->metadata2;
    int offset = 0, composition_offset = 0, result, fallback = 0;
    unsigned int width, height;
    zero_bytes(source, sizeof(source));
    zero_bytes(graphics, sizeof(graphics));
    zero_bytes(target, sizeof(target));
    zero_bytes(infoframe, sizeof(infoframe));
    if (input_format == -1) {
        destroy_cp(context);
        storage->initialized = 0;
        return 0;
    }
    if (storage->initialized == 0) {
        zero_bytes(context, (unsigned int)init_cp_mmg());
        result = init_cp(context, ops->memory);
        if (result != 0) {
            ops->logger("\001" "3Control exuecution library initialization failed\n", 0);
            return -1;
        }
        storage->initialized = 1;
    }
    if (input_format == 2) { metadata_size = 0; composition_size = 0; }
    zero_bytes(cli, 112);
    write_pointer(cli, 0, composition);
    write_pointer(cli, 8, metadata);
    write32(cli, 20, (unsigned int)target_min);
    write32(cli, 24, (unsigned int)target_max);
    write32(cli, 28, (unsigned int)yuv_range);
    write32(cli, 32, (unsigned int)chroma_format);
    write32(cli, 36, (unsigned int)bit_depth);
    write32(cli, 40, read32(output, CONTROL_OUTPUT_USE_LL_FLAG_OFFSET));
    write32(cli, 44, read32(output, CONTROL_OUTPUT_LL_RGB_DESIRED_OFFSET));
    write32(cli, 48, read32(output, CONTROL_OUTPUT_DOVI2HDR10_NOMAPPING_OFFSET));
    write32(cli, 52, (unsigned int)priority);
    write32(cli, 68, (unsigned int)no_el);
    if (graphics_min != 0 || graphics_max != 0) {
        write32(cli, 76, 1);
        write32(cli, 80, read32(output, CONTROL_OUTPUT_G_BITDEPTH_OFFSET));
        write32(cli, 84, (unsigned int)graphics_min);
        write32(cli, 88, (unsigned int)graphics_max);
        write32(cli, 96, read32(output, CONTROL_OUTPUT_G_FORMAT_OFFSET));
        write32(cli, 100, 2);
    }
    storage->input_format = input_format;
    storage->output_format = output_format;
    if (input_format == 0) {
        if (metadata_size > 0) {
            result = read_md_with_logger(current, metadata, metadata_size, &offset, ops->logger);
            if (result < 0) { destroy_cp(context); return result; }
            offset = 0;
        }
    } else { composition = (const char *)0; metadata = (const char *)0; }
    write32(context, CONTROL_CONTEXT_DIAGNOSTIC_ENABLE_OFFSET, 0);
    write32(context, CONTROL_CONTEXT_DIAGNOSTIC_MUX_SELECT_OFFSET, 0);
    context[CONTROL_CONTEXT_DEBUG_MODE_OFFSET] = 0;
    context[CONTROL_CONTEXT_DEBUG_MODE_OFFSET + 1] = 0;
    width = read32(output, CONTROL_OUTPUT_VIDEO_WIDTH_OFFSET) >> 16;
    height = read32(output, CONTROL_OUTPUT_VIDEO_HEIGHT_OFFSET) >> 16;
    write32(context, CONTROL_CONTEXT_WIDTH_OFFSET, width);
    write32(context, CONTROL_CONTEXT_HEIGHT_OFFSET, height);
    write32(cli, 60, width);
    write32(cli, 64, height);
    if (width == 0 || height == 0) { width = 1920; height = 1080; }
    write32(source, 0, read32(cli, 36));
    write32(source, 4, read32(cli, 32));
    write32(source, 8, read32(cli, 28));
    write32(source, 12, width);
    write32(source, 16, height);
    copy_bytes(source + 20, hdr10, 28);
    write32(graphics, 4, width);
    write32(graphics, 8, height);
    if (read32(cli, 76) != 0) populate_graphics_param_body(graphics, cli);
    write32(target, 8, read32(cli, 40));
    write32(target, 12, read32(cli, 44));
    if (read32(output, CONTROL_OUTPUT_VOUT_WIDTH_OFFSET) == 0 ||
        read32(output, CONTROL_OUTPUT_VOUT_HEIGHT_OFFSET) == 0) {
        write32(target, 0, width); write32(target, 4, height);
    } else {
        write32(target, 0, read32(output, CONTROL_OUTPUT_VOUT_WIDTH_OFFSET));
        write32(target, 4, read32(output, CONTROL_OUTPUT_VOUT_HEIGHT_OFFSET));
    }
    if (storage->output_format == 0) {
        if (vsvdb_to_target_param((unsigned char *)output + CONTROL_OUTPUT_VSVDB_TBL_OFFSET, target) == -1)
            fallback = 1;
    }
    if (storage->output_format == 1 || fallback != 0) {
        write32(target, 40, read32(cli, 20));
        write32(target, 44, read32(cli, 24));
    }
    if (read32(cli, 48) != 0) set_dovi2hdr10_mapping(context);
    for (;;) {
        if (read32(cli, 92) == 1) write32(cli, 52, 1);
        if (storage->input_format == 0) {
            if (no_el == 0) write32(cli, 104, 1);
            set_support_el(context, (int)read32(cli, 104));
            if (read32(cli, 72) == 0) {
                copy_bytes(storage->composition, composition + composition_offset, 1800);
                composition_offset = (int)((unsigned int)composition_offset + 1800);
            }
            if (read32(cli, 16) != 0) {
                unsigned char *temporary = current; current = previous; previous = temporary;
            }
            result = read_md_with_logger(current, metadata, metadata_size, &offset, ops->logger);
            if (result < 0) { destroy_cp(context); return -1; }
            if (read32(cli, 16) != 0) filter_md(current, previous);
        }
        if (fallback != 0 || storage->output_format == 1) {
            get_info_frame(context, target, storage->input_format, hdr10, current, infoframe);
            copy_bytes((unsigned char *)output + CONTROL_OUTPUT_HDR_INFO_OFFSET, infoframe, 29);
        }
        result = ops->commit(context, storage->input_format, storage->output_format,
            current, storage->composition, target, graphics, source,
            (int)read32(cli, 52), storage->register1, storage->register2,
            storage->register3, (unsigned char *)output + CONTROL_OUTPUT_DM_LUT1_OFFSET,
            (unsigned char *)output + CONTROL_OUTPUT_DM_LUT2_OFFSET, ops->commit_ops);
        if (storage->output_format == 0) {
            write32(output, CONTROL_OUTPUT_DIAGNOSTIC_ENABLE_OFFSET,
                read32(context, CONTROL_CONTEXT_DIAGNOSTIC_ENABLE_OFFSET));
            write32(output, CONTROL_OUTPUT_DIAGNOSTIC_MUX_SELECT_OFFSET,
                read32(context, CONTROL_CONTEXT_DIAGNOSTIC_MUX_SELECT_OFFSET));
            write32(output, CONTROL_OUTPUT_DOVI_LL_ENABLE_OFFSET,
                read32(context, CONTROL_CONTEXT_DOVI_LL_ENABLE_OFFSET));
        }
        copy_bytes((unsigned char *)output + CONTROL_OUTPUT_DM_REG1_OFFSET, storage->register1, 108);
        copy_bytes(output, (unsigned char *)storage->register1 + 176, 692);
        copy_bytes((unsigned char *)output + CONTROL_OUTPUT_DM_REG2_OFFSET, storage->register2, 96);
        copy_bytes((unsigned char *)output + CONTROL_OUTPUT_DM_REG3_OFFSET,
                   (unsigned char *)storage->register3 + 24, 104);
        if (storage->output_format == 0) {
            if (storage->input_format == 0) {
                if (ops->reduce_trims != 0)
                    update_md_for_hdmi_bound(context, current, ops->reduce_trims);
                else update_md_for_hdmi(context, current);
            }
            else get_fixed_metadata(context, current);
            write32(output, CONTROL_OUTPUT_EXT_MD_OFFSET, 0);
            output_metadata(output, current, ops);
        }
        if (metadata_size == offset ||
            (composition_size == composition_offset && read32(cli, 72) == 0)) break;
    }
    if (ops->cpu_version(0) != 0x28 && ops->cpu_version(0) != 0x29 &&
        ops->cpu_version(0) != 0x2b) {
        zero_bytes(output, 692);
        zero_bytes((unsigned char *)output + CONTROL_OUTPUT_DM_REG1_OFFSET, 108);
        zero_bytes((unsigned char *)output + CONTROL_OUTPUT_DM_LUT1_OFFSET, 5120);
    }
    destroy_cp(context);
    return result;
}
