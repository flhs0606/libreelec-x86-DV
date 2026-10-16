/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "dvbridge_policy.h"
#include <math.h>

enum dvbridge_policy_status dvbridge_policy_validate(const struct dvbridge_policy *policy)
{
    if (!policy || policy->mode < DVBRIDGE_MODE_DISABLED ||
        policy->mode > DVBRIDGE_MODE_ENHANCED_DV ||
        policy->enhancement < DVBRIDGE_ENHANCEMENT_NATURAL ||
        policy->enhancement > DVBRIDGE_ENHANCEMENT_INTENSE)
        return DVBRIDGE_POLICY_INVALID_VALUE;
    const struct dvbridge_tv_profile *tv = &policy->tv;
    if (tv->panel < DVBRIDGE_PANEL_UNSET || tv->panel > DVBRIDGE_PANEL_LCD ||
        tv->gamut < DVBRIDGE_GAMUT_UNSET || tv->gamut > DVBRIDGE_GAMUT_BT2020 ||
        !isfinite(tv->peak_nits) || tv->peak_nits < 0 || tv->peak_nits > 10000)
        return DVBRIDGE_POLICY_INVALID_VALUE;
    if (tv->peak_nits == 0 && tv->panel == DVBRIDGE_PANEL_UNSET &&
        tv->gamut == DVBRIDGE_GAMUT_UNSET)
        return policy->mode >= DVBRIDGE_MODE_HDR10_EXPERT ?
            DVBRIDGE_POLICY_TV_PROFILE_REQUIRED : DVBRIDGE_POLICY_VALID;
    if (tv->gamut == DVBRIDGE_GAMUT_UNSET)
        return DVBRIDGE_POLICY_TV_PROFILE_REQUIRED;
    return tv->peak_nits > 0 ? DVBRIDGE_POLICY_VALID : DVBRIDGE_POLICY_INVALID_VALUE;
}

bool dvbridge_identity_equal(const struct dvbridge_identity *a, const struct dvbridge_identity *b)
{
    return a && b && a->stream == b->stream && a->picture == b->picture &&
           a->revision == b->revision;
}

#include "dovi_engine.h"

bool dvbridge_policy_apply_vsvdb(struct dvbridge_tv_profile *tv, const uint8_t vsvdb[32])
{
    if (!tv || !vsvdb) return false;
    struct cb1_dovi_target_desc target;
    if (cb1_dovi_vsvdb_parse(vsvdb, &target) != 0 || target.max_nits <= 0.0)
        return false;

    tv->peak_nits = target.max_nits;

    /* Classify gamut from primary red coordinate (target.rx) */
    if (target.rx >= 0.68) {
        tv->gamut = DVBRIDGE_GAMUT_BT2020;
    } else if (target.rx >= 0.65) {
        tv->gamut = DVBRIDGE_GAMUT_P3_D65;
    } else {
        tv->gamut = DVBRIDGE_GAMUT_BT709;
    }

    if (tv->panel == DVBRIDGE_PANEL_UNSET) {
        tv->panel = (target.min_nits <= 0.005) ? DVBRIDGE_PANEL_OLED : DVBRIDGE_PANEL_LCD;
    }
    return true;
}
