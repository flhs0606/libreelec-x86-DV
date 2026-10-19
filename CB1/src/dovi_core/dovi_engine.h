/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DOVI_ENGINE_H
#define DOVI_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Target display capability parsed from 32-byte Dolby VSVDB */
struct cb1_dovi_target_desc {
    uint32_t length;
    uint32_t version;            /* 0: v0, 1: v1 (supports DM v1.x/v2.x), 2: v2 */
    uint32_t sink_dm_version;
    uint32_t min_pq;             /* 12-bit PQ [0..4095] */
    uint32_t max_pq;             /* 12-bit PQ [0..4095] */
    double   min_nits;           /* Physical min luminance */
    double   max_nits;           /* Physical max peak luminance */
    /* Color primary coordinates (CIE 1931 xy) */
    double rx, ry, gx, gy, bx, by, wx, wy;
    /* Raw 88-byte target descriptor for internal dovi_core control calls */
    uint8_t raw_target[88];
};

/* Parsed RPU analysis result */
struct cb1_dovi_rpu_result {
    bool is_valid;
    bool is_mel;
    uint8_t rpu_type;
    uint8_t main_cfg[1800];      /* 1,800-byte sparse hardware composer configuration */
    uint8_t dm_sequence[1024];   /* Serialized DM byte sequence */
    int dm_sequence_size;
};

/* Opaque instance context holding thread-safe Decoder and Control arenas */
struct cb1_dovi_instance;

/* Lifecycle management */
struct cb1_dovi_instance *cb1_dovi_create(void);
void cb1_dovi_destroy(struct cb1_dovi_instance *inst);
void cb1_dovi_reset(struct cb1_dovi_instance *inst);

/* 1. VSVDB Parsing: Decode 32-byte Dolby VSVDB into Target Descriptor */
int cb1_dovi_vsvdb_parse(const uint8_t *vsvdb_buf, struct cb1_dovi_target_desc *out_target);

/* 2. Safe RPU Ingress: Decode RPU payload with >=16B padding contract and bounds check */
bool cb1_dovi_parse_rpu(struct cb1_dovi_instance *inst, const uint8_t *rpu_buf, size_t size,
                        struct cb1_dovi_rpu_result *out_result);

/* 3. Official 3-piece Tone Curve & 4096-step LUT generation, normalized to FP32 [0.0f..1.0f] */
bool cb1_dovi_generate_tone_lut_fp32(int src_min_pq, int src_max_pq,
                                    int target_min_pq, int target_max_pq,
                                    int trim_slope, int trim_offset, int trim_power,
                                    float lut_fp32[4096]);

/* 4. Official Trim Reduction: Sort and prune Level 2 & Level 8 Trims in packed metadata */
int cb1_dovi_reduce_trims(const uint8_t raw_target[88], void *packed_metadata);

#ifdef __cplusplus
}
#endif

#endif /* DOVI_ENGINE_H */
