/* Serialize parser display metadata into the packed control format. */
#ifndef DM_SERIALIZATION_H
#define DM_SERIALIZATION_H

/* dm uses the 0x10744-byte vdr_dm_data_payload_t layout. length is
 * signed input capacity and becomes the written length only on success.
 * Failure is -12 and can leave a partially written sequence. Supply distinct
 * readable input, writable output and length objects. Unknown extensions retain
 * their unchecked input length, and need backing beyond the 256-byte union
 * when that length exceeds it. See ../docs/parser.md for serialization rules. */
int dm_rpu_payload_2_byte_sequence(const void *dm, char *output, int *length);

#endif
