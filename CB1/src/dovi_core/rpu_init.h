/* Initialize caller-owned decoder storage. */
#ifndef AM6B_RPU_INIT_H
#define AM6B_RPU_INIT_H

/*
 * flags is the disjoint, readable 28-byte rpu_init_flags_t layout.
 * Reads LE u32 level (+4), disable_crc_check (+8), payload_only (+12), and
 * disable_MEL_detection (+20). Supported level indices are 0..15.
 * decoder must be writable for the arena described in ../docs/parser.md.
 * The function performs no validation and always returns zero.
 */
int rpu_decoder_init(void *decoder, const unsigned char *flags);

#endif
