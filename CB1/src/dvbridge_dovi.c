/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Dolby Vision reference tone curve bridge.
 *
 * The reverse-engineered dovi_core engine produces the three-piece Dolby
 * curve as a 4096-entry table in the 12-bit PQ code domain. libplacebo asks a
 * tone-map function to fill its own LUT through struct pl_tone_map_function,
 * whose private pointer lets us hand over that table without touching
 * libplacebo itself. */

#include "dvbridge_dovi.h"

#include "dvbridge_policy.h"
#include "dovi_core/dovi_engine.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define DVBRIDGE_DOVI_FNV_OFFSET 2166136261u
#define DVBRIDGE_DOVI_FNV_PRIME 16777619u
#define DVBRIDGE_DOVI_DEFAULT_MAX_PQ 3079 /* 1000 nits in 12-bit PQ */

static uint32_t dovi_hash(const int* values, size_t count)
{
  uint32_t hash = DVBRIDGE_DOVI_FNV_OFFSET;
  for (size_t i = 0; i < count; ++i)
  {
    const uint32_t v = (uint32_t)values[i];
    for (int byte = 0; byte < 4; ++byte)
    {
      hash ^= (v >> (byte * 8)) & 0xffu;
      hash *= DVBRIDGE_DOVI_FNV_PRIME;
    }
  }
  return hash;
}

bool dvbridge_dovi_prepare(struct dvbridge_dovi* state,
                           int src_min_pq,
                           int src_max_pq,
                           int tgt_min_pq,
                           int tgt_max_pq,
                           int trim_slope,
                           int trim_offset,
                           int trim_power)
{
  if (!state)
    return false;

  state->ready = false;
  state->target_max_pq = 0;

  /* The curve generator only accepts a meaningful source range inside the
   * 12-bit PQ domain, and a target peak above zero. */
  if (src_min_pq < 0 || src_max_pq > 4095 || src_max_pq <= src_min_pq)
    return false;
  if (tgt_max_pq <= 0 || tgt_max_pq > 4095)
    return false;
  if (tgt_min_pq < 0 || tgt_min_pq >= tgt_max_pq)
    tgt_min_pq = 0;

  if (!cb1_dovi_generate_tone_lut_fp32(src_min_pq, src_max_pq, tgt_min_pq, tgt_max_pq,
                                       trim_slope, trim_offset, trim_power, state->lut))
    return false;

  const int values[6] = {src_min_pq, src_max_pq, tgt_min_pq, tgt_max_pq,
                         trim_slope, trim_offset};
  state->revision = dovi_hash(values, sizeof(values) / sizeof(values[0]));
  state->target_max_pq = tgt_max_pq;
  state->ready = true;
  return true;
}

bool dvbridge_dovi_select(struct dvbridge_dovi* state,
                          const struct dvbridge_policy* policy,
                          const void* metadata,
                          size_t bytes)
{
  if (!state || !policy || !metadata || bytes < sizeof(AVDOVIMetadata))
    return false;

  const AVDOVIMetadata* md = (const AVDOVIMetadata*)metadata;
  const AVDOVIDmData* l1 = av_dovi_find_level(md, 1);
  if (!l1 || !l1->l1.max_pq || l1->l1.max_pq > 4095 || l1->l1.min_pq > l1->l1.max_pq)
  {
    state->ready = false;
    return false;
  }

  int target_max_pq = DVBRIDGE_DOVI_DEFAULT_MAX_PQ;
  if (policy->tv.peak_nits > 0.0)
    target_max_pq = (int)lrintf(pl_hdr_rescale(PL_HDR_NITS, PL_HDR_PQ,
                                              (float)policy->tv.peak_nits) * 4095.0f);

  /* Only the L2 block that targets the configured display peak applies. The
   * fixed-point trims are centred on 2048 and scaled to the 2^28 domain used
   * by tone_curve_source. */
  int trim_slope = 0;
  int trim_offset = 0;
  int trim_power = 0;
  const AVDOVIDmData* l2 = av_dovi_find_level(md, 2);
  if (l2)
  {
    const int delta = (int)l2->l2.target_max_pq - target_max_pq;
    if (delta >= -1 && delta <= 1)
    {
      const double scale = 268435456.0 / 2048.0;
      trim_slope = (int)lround((l2->l2.trim_slope - 2048.0) * scale);
      trim_offset = (int)lround((l2->l2.trim_offset - 2048.0) * scale);
      trim_power = (int)lround((l2->l2.trim_power - 2048.0) * scale);
    }
  }

  /* Rebuilding the curve costs a few hundred microseconds. L1 usually holds
   * for a whole scene, so skip the work while the parameter set is unchanged. */
  const int values[6] = {l1->l1.min_pq, l1->l1.max_pq, 0, target_max_pq,
                         trim_slope, trim_offset};
  if (state->ready && state->revision == dovi_hash(values, 6))
    return true;

  return dvbridge_dovi_prepare(state, l1->l1.min_pq, l1->l1.max_pq, 0, target_max_pq,
                               trim_slope, trim_offset, trim_power);
}

static void dovi_tone_map(float* lut, const struct pl_tone_map_params* params)
{
  if (!lut || !params || !params->lut_size)
    return;

  const struct dvbridge_dovi* state =
      params->function ? (const struct dvbridge_dovi*)params->function->priv : NULL;

  /* libplacebo pre-fills the LUT with input values already converted to this
   * function's scaling (PQ), and expects an in-place transform. Leaving the
   * table untouched is therefore a correct identity fallback. */
  if (!state || !state->ready)
    return;

  for (size_t i = 0; i < params->lut_size; ++i)
  {
    int code = (int)lrintf(lut[i] * 4095.0f);
    code = code < 0 ? 0 : (code > 4095 ? 4095 : code);

    float y = state->lut[code];
    if (y < params->output_min)
      y = params->output_min;
    if (y > params->output_max)
      y = params->output_max;
    lut[i] = y;
  }
}

const struct pl_tone_map_function dvbridge_dovi_tone_map_function = {
    .name = "dovi",
    .description = "Dolby Vision reference tone curve (dovi_core)",
    .scaling = PL_HDR_PQ,
    .map = dovi_tone_map,
    .priv = NULL,
};
