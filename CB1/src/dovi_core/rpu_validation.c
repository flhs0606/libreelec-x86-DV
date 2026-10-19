/* Validate RPU level and profile constraints. */
#include "rpu_validation.h"

typedef unsigned char byte;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef char validation_u32_width[(sizeof(u32) == 4) ? 1 : -1];
typedef char validation_ptr_width[(sizeof(void *) == 8) ? 1 : -1];

typedef struct {
    u32 num_y_partitions;
    u32 num_x_partitions;
    u32 num_pivots;
    u32 nlq_num_pivots;
    u32 prev_rpu_id;
} level_limit;

typedef struct {
    u32 mapping_method[16];
    u32 mapping_cs[16];
    u32 mapping_cf[4];
    u32 exp_resample;
    u32 res_upsample;
} profile_limit;

typedef char level_row_width[(sizeof(level_limit) == 20) ? 1 : -1];
typedef char profile_row_width[(sizeof(profile_limit) == 152) ? 1 : -1];

/* Exact 16-row, little-endian tables in .rodata. Unlisted fields are zero. */
const level_limit level_limits[16] = {
    { 1, 1, 9, 2, 0 },
    { 4, 4, 11, 2, 3 },
    { 8, 8, 17, 2, 7 },
};

#define PROFILE_ZERO { { 0, ~0u }, { 0, ~0u }, { 0, ~0u }, 0, 0 }
#define PROFILE_INVALID { { ~0u }, { ~0u }, { ~0u }, 0, 0 }
const profile_limit profile_limits[16] = {
    PROFILE_ZERO,
    { { 0, 1, ~0u }, { 0, ~0u }, { 0, ~0u }, 1, 0 },
    { { 0, 3, 1, 2, ~0u }, { 0, 1, ~0u }, { 0, 1, 2, ~0u }, 1, 1 },
    PROFILE_ZERO, PROFILE_ZERO, PROFILE_ZERO,
    PROFILE_INVALID, PROFILE_INVALID, PROFILE_INVALID,
    PROFILE_INVALID, PROFILE_INVALID, PROFILE_INVALID,
    PROFILE_INVALID, PROFILE_INVALID, PROFILE_INVALID,
    PROFILE_INVALID,
};
#undef PROFILE_ZERO
#undef PROFILE_INVALID

static const byte *read_pointer(const byte *bytes)
{
    u64 value = 0;
    u32 index;

    for (index = 0; index != 8; ++index)
        value |= (u64)bytes[index] << (index * 8);
    return (const byte *)value;
}

int validate_level(const void *rpu_data, rpu_log_fn msg_log,
                   unsigned int print_severity)
{
    const byte *header = read_pointer((const byte *)rpu_data);
    u32 level = header[5];
    const level_limit *limit;
    u32 value, component;

    /* The level table contains 16 rows; invalid indices are excluded. */
    if (level >= 16)
        return -4;
    limit = &level_limits[level];

    value = header[0x134];
    if (value > limit->num_y_partitions) {
        if (msg_log && print_severity > 2)
            msg_log("RPU warning INVALID LEVEL: num_y_partitions = %d, level limit            = %d\n",
                    value, limit->num_y_partitions);
        return -4;
    }
    value = header[0x133];
    if (value > limit->num_x_partitions) {
        if (msg_log && print_severity > 2)
            msg_log("RPU warning INVALID LEVEL: num_x_partitions = %d limits.num_x_partitions = %d\n",
                    value, limit->num_x_partitions);
        return -4;
    }
    for (component = 0; component != 3; ++component) {
        value = header[0x25 + component];
        if (value > limit->num_pivots) {
            if (msg_log && print_severity > 2)
                msg_log("RPU warning INVALID LEVEL: num_pivots[%d]   = %d limits.num_pivots       = %d\n",
                        component, value, limit->num_pivots);
            return -4;
        }
    }
    value = header[0xf1];
    if (value > limit->nlq_num_pivots) {
        if (msg_log && print_severity > 2)
            msg_log("RPU warning INVALID LEVEL: nlq_num_pivots   = %d limits.nlq_num_pivots   = %d\n",
                    value, limit->nlq_num_pivots);
        return -4;
    }
    value = header[0x1b];
    if (value > limit->prev_rpu_id) {
        if (msg_log && print_severity > 2)
            msg_log("RPU warning INVALID LEVEL: vdr_rpu_id       = %d limits.prev_rpu_id      = %d\n",
                    value, limit->prev_rpu_id);
        return -4;
    }
    value = header[0x1a];
    if (value > limit->prev_rpu_id) {
        if (msg_log && print_severity > 2)
            msg_log("RPU warning INVALID LEVEL: prev_vdr_rpu_id  = %d limits.prev_rpu_id      = %d\n",
                    value, limit->prev_rpu_id);
        return -4;
    }
    return 0;
}

static int allowed(const u32 *entries, u32 count, u32 value)
{
    u32 index;
    for (index = 0; index != count; ++index) {
        if (entries[index] == ~0u)
            return 0;
        if (entries[index] == value)
            return 1;
    }
    return 0;
}

int validate_profile(const void *rpu_data, rpu_log_fn msg_log,
                     unsigned int print_severity)
{
    const byte *root = (const byte *)rpu_data;
    const byte *header = read_pointer(root);
    const byte *payload;
    const profile_limit *limit;
    const char *message;
    u32 profile = header[4], x_count = header[0x133], y_count = header[0x134];
    u32 partition, component, pivot, pivots, value;

    if (profile >= 16)
        return -5;
    limit = &profile_limits[profile];

    if (x_count != 0 && y_count != 0) {
        payload = read_pointer(root + 8);
        for (partition = 0; partition != x_count * y_count; ++partition) {
            const byte *mapping = payload + (u64)partition * 0x7038u;
            for (component = 0; component != 3; ++component) {
                pivots = header[0x25 + component];
                if (pivots < 2)
                    return -2;
                for (pivot = 0; pivot < pivots - 1; ++pivot) {
                    value = mapping[component * 15 + pivot];
                    if (!allowed(limit->mapping_method, 16, value)) {
                        message = "Invalid mapping method for this profile\n";
                        goto invalid;
                    }
                }
            }
        }
    }

    if (!allowed(limit->mapping_cs, 16, header[0x1c])) {
        message = "Invalid mapping color space for this profile\n";
        goto invalid;
    }
    if (!allowed(limit->mapping_cf, 4, header[0x1d])) {
        message = "Invalid mapping chroma format for this profile\n";
        goto invalid;
    }
    if (limit->exp_resample == 0 && (header[0x13] || header[8])) {
        message = "Explicit resampling not allowed in this profile\n";
        goto invalid;
    }
    return 0;

invalid:
    if (msg_log && print_severity > 2)
        msg_log(message);
    return -5;
}
