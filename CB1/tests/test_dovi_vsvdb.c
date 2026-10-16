/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_engine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    /* Sample 32-byte Dolby VSVDB block (V1 format)
     * Header: IEEE OUI 0x00D046, Tag 0x07 (Vendor Specific), Length 0x0B (11+1=12 bytes)
     * vsvdb[0] = (7 << 5) | (length - 1) = 0xEB
     */
    uint8_t vsvdb_v1[32] = {
        0xeb, 0x01, 0x46, 0xd0, 0x00, /* Header & OUI */
        0x20,                         /* v1, DM v1.x */
        0x20, 0x05,                   /* max lum = 1000 nits, min lum = 0.005 nits */
        0x00, 0x00, 0x00, 0x00,       /* Red, Green, Blue chromaticities */
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };

    struct cb1_dovi_target_desc target;
    int ret = cb1_dovi_vsvdb_parse(vsvdb_v1, &target);
    assert(ret == 0);
    assert(target.version == 1);
    assert(target.max_pq > 0 && target.max_pq <= 4095);
    assert(target.max_nits >= 100.0);

    /* Test invalid OUI rejection */
    uint8_t invalid_vsvdb[32] = {0};
    ret = cb1_dovi_vsvdb_parse(invalid_vsvdb, &target);
    assert(ret != 0);

    printf("test_dovi_vsvdb: PASSED (version=%u, max_pq=%u, max_nits=%.1f)\n",
           target.version, target.max_pq, target.max_nits);
    return 0;
}
