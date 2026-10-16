#include <stdalign.h>
/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_engine.h"
#include "dovi_bridge.h"

#include "control_target.h"
#include "control_luminance.h"
#include "control_abi_recovered.h"
#include "rpu_init.h"
#include "rpu_output.h"
#include "decoder_bindings.h"
#include "composer_conversion.h"
#include "dm_serialization.h"
#include "tone_controls.h"
#include "tone_parameters.h"
#include "tone_curve.h"
#include "tone_curve_lut.h"
#include "trim_reduction.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define CB1_DOVI_DECODER_ARENA_SIZE 340992u /* 0x53400 >= 0x437fa + 0xe070 */

struct cb1_dovi_instance {
    alignas(64) uint8_t decoder_arena[CB1_DOVI_DECODER_ARENA_SIZE];
    pthread_mutex_t lock;
    bool decoder_initialized;
};

struct cb1_dovi_instance *cb1_dovi_create(void)
{
    struct cb1_dovi_instance *inst = calloc(1, sizeof(*inst));
    if (!inst) return NULL;

    pthread_mutex_init(&inst->lock, NULL);

    /* Initialize runtime bridge services */
    dovi_bridge_init_services();

    /* Initialize Payload-only RPU decoder arena according to contract */
    unsigned char flags[28] = {0};
    /* flags[4]: level 0 (1 partition, max_prev = 0) */
    flags[4] = 0;
    /* flags[12]: payload_only = 1 */
    flags[12] = 1;
    /* flags[8]: disable_crc_check = 0 */
    flags[8] = 0;
    /* flags[20]: disable_MEL_detection = 0 */
    flags[20] = 0;

    rpu_decoder_init(inst->decoder_arena, flags);
    inst->decoder_initialized = true;

    return inst;
}

void cb1_dovi_destroy(struct cb1_dovi_instance *inst)
{
    if (!inst) return;
    pthread_mutex_destroy(&inst->lock);
    free(inst);
}

void cb1_dovi_reset(struct cb1_dovi_instance *inst)
{
    if (!inst) return;
    pthread_mutex_lock(&inst->lock);
    unsigned char flags[28] = {0};
    flags[4] = 0;
    flags[12] = 1;
    rpu_decoder_init(inst->decoder_arena, flags);
    inst->decoder_initialized = true;
    pthread_mutex_unlock(&inst->lock);
}

static inline uint32_t read_target_u32(const uint8_t *p, unsigned int offset)
{
    const uint8_t *src = p + offset;
    return (uint32_t)src[0] | ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) | ((uint32_t)src[3] << 24);
}

int cb1_dovi_vsvdb_parse(const uint8_t *vsvdb_buf, struct cb1_dovi_target_desc *out_target)
{
    if (!vsvdb_buf || !out_target) return -1;

    memset(out_target, 0, sizeof(*out_target));
    int ret = vsvdb_to_target_param(vsvdb_buf, out_target->raw_target);
    if (ret != 0) return ret;

    out_target->length = read_target_u32(out_target->raw_target, CONTROL_TARGET_VSVDB_LENGTH_OFFSET);
    out_target->version = read_target_u32(out_target->raw_target, CONTROL_TARGET_VSVDB_VERSION_OFFSET);
    out_target->sink_dm_version = read_target_u32(out_target->raw_target, CONTROL_TARGET_SINK_DM_VERSION_OFFSET);
    out_target->min_pq = read_target_u32(out_target->raw_target, CONTROL_TARGET_TMINPQ_OFFSET);
    out_target->max_pq = read_target_u32(out_target->raw_target, CONTROL_TARGET_TMAXPQ_OFFSET);

    /* TMIN and TMAX are scaled where 10,000 units = 1 nit (100,000,000 = 10,000 nits) */
    uint32_t tmin_raw = read_target_u32(out_target->raw_target, CONTROL_TARGET_TMIN_OFFSET);
    uint32_t tmax_raw = read_target_u32(out_target->raw_target, CONTROL_TARGET_TMAX_OFFSET);
    out_target->min_nits = (double)tmin_raw / 10000.0;
    out_target->max_nits = (double)tmax_raw / 10000.0;

    /* Primaries are stored shifted by 14 bits (Q26 fixed-point normalized) */
    out_target->rx = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_RX_OFFSET) / 67108864.0;
    out_target->ry = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_RY_OFFSET) / 67108864.0;
    out_target->gx = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_GX_OFFSET) / 67108864.0;
    out_target->gy = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_GY_OFFSET) / 67108864.0;
    out_target->bx = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_BX_OFFSET) / 67108864.0;
    out_target->by = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_BY_OFFSET) / 67108864.0;
    out_target->wx = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_WX_OFFSET) / 67108864.0;
    out_target->wy = (double)read_target_u32(out_target->raw_target, CONTROL_TARGET_WY_OFFSET) / 67108864.0;

    return 0;
}

bool cb1_dovi_parse_rpu(struct cb1_dovi_instance *inst, const uint8_t *rpu_buf, size_t size,
                        struct cb1_dovi_rpu_result *out_result)
{
    if (!inst || !rpu_buf || !out_result) return false;
    memset(out_result, 0, sizeof(*out_result));

    /* Requirement 5: Check minimum length and presence of non-zero payload */
    if (size < 5 || size > 4096) return false;

    bool has_nonzero = false;
    for (size_t i = 0; i < size; ++i) {
        if (rpu_buf[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    if (!has_nonzero) return false;

    /* Requirement 5.1: Ensure safe buffer with at least 16 bytes of zero-padding at the tail */
    uint8_t safe_input[4096 + 32] = {0};
    memcpy(safe_input, rpu_buf, size);

    /* Strip Annex-B start code if present */
    const uint8_t *payload_ptr = safe_input;
    size_t payload_size = size;

    if (payload_size >= 4 && payload_ptr[0] == 0 && payload_ptr[1] == 0 &&
        payload_ptr[2] == 0 && payload_ptr[3] == 1) {
        payload_ptr += 4;
        payload_size -= 4;
    } else if (payload_size >= 3 && payload_ptr[0] == 0 && payload_ptr[1] == 0 &&
               payload_ptr[2] == 1) {
        payload_ptr += 3;
        payload_size -= 3;
    }

    if (payload_size < 2) return false;

    pthread_mutex_lock(&inst->lock);

    void *header = NULL;
    void *payload = NULL;
    int parsed_payload_size = 0;
    void *dm_payload = NULL;
    int dm_payload_size = 0;

    int res = rpu_decoder_process_buffer(inst->decoder_arena, (unsigned char *)payload_ptr,
                                        (int)payload_size, &header, &payload,
                                        &parsed_payload_size, &dm_payload, &dm_payload_size);
    if (res < 0 || !header || !payload) {
        pthread_mutex_unlock(&inst->lock);
        return false;
    }

    out_result->is_valid = true;
    const uint8_t *h_bytes = (const uint8_t *)header;
    out_result->rpu_type = h_bytes[0];

    /* Check if MEL was detected (stored in decoder flag byte +0x16) */
    uint8_t *data_ptr = *(uint8_t **)inst->decoder_arena;
    if (data_ptr && data_ptr[0x16] != 0) {
        out_result->is_mel = true;
    }

    /* Convert to 1,800-byte sparse composer configuration */
    comp_rpu_2_main_cfg(header, payload, out_result->main_cfg);

    /* Serialize DM payload to packed byte sequence */
    if (dm_payload && dm_payload_size > 0) {
        int seq_capacity = sizeof(out_result->dm_sequence);
        int ser_res = dm_rpu_payload_2_byte_sequence(dm_payload, out_result->dm_sequence,
                                                     &seq_capacity);
        if (ser_res == 0) {
            out_result->dm_sequence_size = seq_capacity;
        }
    }

    pthread_mutex_unlock(&inst->lock);
    return true;
}

bool cb1_dovi_generate_tone_lut_fp32(int src_min_pq, int src_max_pq,
                                    int target_min_pq, int target_max_pq,
                                    int trim_slope, int trim_offset, int trim_power,
                                    float lut_fp32[4096])
{
    if (!lut_fp32) return false;

    struct tone_curve_source source = {0};
    source.min_pq = src_min_pq;
    source.max_pq = src_max_pq;
    source.crush = src_min_pq << 19;
    source.clip = src_max_pq << 19;
    source.mid = (source.crush + source.clip) / 2;
    source.trim_slope = trim_slope;
    source.trim_offset = trim_offset;
    source.trim_power = trim_power;

    struct tone_curve_display display = {0};
    display.min_pq = target_min_pq << 19;
    display.max_pq = target_max_pq << 19;
    display.mode = 1; /* Forward Tone Mapping (Source -> Target Display) */
    display.rolloff = 1;
    display.rolloff_inverse = 1;

    struct tone_curve_pair param1 = {0}, param2 = {0}, param3 = {0};
    int slope_over_rolloff = 0;
    short lut515[516] = {0};
    short lut4k[4096] = {0};

    int target_min = 0, target_mid = 0, target_max = 0, source_mid = 0;
    int slope = 0, source_target_ratio = 0, screen_ratio = 0;

    calc_mapped_params(source.clip, &source, &display, &target_min, &target_mid,
                       &target_max, &source_mid, &slope, &source_target_ratio, &screen_ratio);
    calc_curve_params(source.crush, source_mid, source.clip, target_min, target_mid,
                      target_max, slope, &display, &param1, &param2, &param3, &slope_over_rolloff);

    generate_tone_curve_lut(source.crush, source.clip, &param1, &param2, &param3,
                           slope_over_rolloff, &display,
                           source.trim_slope, source.trim_power, source.trim_offset,
                           lut515, lut4k, 512, 1);

    /* Requirement 3: Convert 16-bit fixed-point (0..4095 PQ) to normalized FP32 [0.0f..1.0f] */
    for (int i = 0; i < 4096; ++i) {
        float val = (float)lut4k[i] / 4095.0f;
        if (val < 0.0f) val = 0.0f;
        if (val > 1.0f) val = 1.0f;
        lut_fp32[i] = val;
    }

    return true;
}

int cb1_dovi_reduce_trims(const uint8_t raw_target[88], void *packed_metadata)
{
    if (!raw_target || !packed_metadata) return -1;
    const void *target_ptr = raw_target;
    return reduce_trims(&target_ptr, packed_metadata);
}
