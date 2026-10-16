/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_engine.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    /* Construct sample target descriptor */
    uint8_t raw_target[88] = {0};
    uint8_t vsvdb[32] = {
        0xeb, 0x01, 0x46, 0xd0, 0x00,
        0x20, 0x20, 0x05, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    struct cb1_dovi_target_desc target;
    int ret = cb1_dovi_vsvdb_parse(vsvdb, &target);
    assert(ret == 0);
    memcpy(raw_target, target.raw_target, sizeof(raw_target));

    /* Construct packed metadata buffer: 71-byte header + records */
    uint8_t packed_metadata[1024] = {0};
    /* Byte 70 is record count */
    packed_metadata[70] = 2;

    /* Record 0 at offset 71 (stride 261): Level 2 trim */
    uint8_t *rec0 = packed_metadata + 71;
    rec0[4] = 2; /* level 2 */
    /* Put 12-bit target max pq = 3079 */
    rec0[5] = (uint8_t)(3079 & 0xFF);
    rec0[6] = (uint8_t)((3079 >> 8) & 0xFF);

    /* Record 1 at offset 71 + 261: Level 8 trim */
    uint8_t *rec1 = packed_metadata + 71 + 261;
    rec1[4] = 8; /* level 8 */
    rec1[5] = 1; /* target display index */

    /* Run reduce_trims */
    int reduce_ret = cb1_dovi_reduce_trims(raw_target, packed_metadata);
    /* Function returns 0 on success or status code */
    assert(reduce_ret >= 0);

    printf("test_dovi_trim_reduction: PASSED (result=%d, records_remaining=%u)\n",
           reduce_ret, packed_metadata[70]);
    return 0;
}
