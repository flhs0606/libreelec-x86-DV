/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_engine.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>

int main(void)
{
    /* Source: 0.005 to 4000 nits (PQ ~ 62 to 3696)
     * Target: 0.005 to 800 nits  (PQ ~ 62 to 3079)
     */
    int src_min_pq = 62;
    int src_max_pq = 3696;
    int tgt_min_pq = 62;
    int tgt_max_pq = 3079;

    int trim_slope = 0;
    int trim_offset = 0;
    int trim_power = 0;

    float lut_fp32[4096] = {0};

    bool ok = cb1_dovi_generate_tone_lut_fp32(src_min_pq, src_max_pq,
                                             tgt_min_pq, tgt_max_pq,
                                             trim_slope, trim_offset, trim_power,
                                             lut_fp32);
    assert(ok);

    /* Verify non-negative, bounded in [0.0, 1.0], and monotonic non-decreasing */
    for (int i = 0; i < 4096; ++i) {
        assert(isfinite(lut_fp32[i]));
        assert(lut_fp32[i] >= 0.0f);
        assert(lut_fp32[i] <= 1.0f);
        if (i > 0) {
            assert(lut_fp32[i] >= lut_fp32[i - 1]);
        }
    }

    /* Verify tone curve compression at the upper end */
    assert(lut_fp32[4095] > 0.5f);

    printf("test_dovi_tone_curve: PASSED (lut[0]=%.4f, lut[2048]=%.4f, lut[4095]=%.4f)\n",
           lut_fp32[0], lut_fp32[2048], lut_fp32[4095]);
    return 0;
}
