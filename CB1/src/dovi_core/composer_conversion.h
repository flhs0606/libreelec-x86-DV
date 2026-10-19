#ifndef AM6B_COMPOSER_CONVERSION_H
#define AM6B_COMPOSER_CONVERSION_H

#define COMPOSER_HEADER_BYTES 0x136u
#define COMPOSER_PAYLOAD_BYTES 0x7038u
#define COMPOSER_MAIN_CFG_BYTES 0x708u

/* Sparse conversion of the little-endian layouts. Objects must be stable,
 * accessible and disjoint. This function has no pointer/count/order guards.
 * Counts and orders outside nominal array limits can read/write past the
 * declared ABI records; callers must supply storage for every accessed byte.
 * Inactive fields and unused coefficients are retained, not initialized.
 */
void comp_rpu_2_main_cfg(const void *header, const void *payload, void *output);

#endif
