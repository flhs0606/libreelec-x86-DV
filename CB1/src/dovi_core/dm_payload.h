/* Display metadata and extension readers. */
#ifndef AM6B_DM_PAYLOAD_H
#define AM6B_DM_PAYLOAD_H

#include "bitstream.h"
#define DM_PAYLOAD_BYTES 0x10744u
#define EXT_BLOCK_BYTES 0x108u

/* The event slot passed to ext_block_payload_read
 * contains a callback pointer, rather than being the callback itself.
 * context has availability/index arrays at +0x67c/+0x77c. */
int ext_block_payload_read(void *context, void *event_callback_slot,
    void *record, struct bitstream *bits, unsigned char *current_index);
int ext_blocks_read(void *decoder, void *blocks, unsigned char *count,
                        int payload_num);
int dm_data_payload_read(void *decoder);
void update_dm_extensions(void *dm, const void *blocks,
                             unsigned int count, void *scratch);
void remove_dm_extensions(void *dm, const unsigned char *remove,
                             void *scratch);
void store_md(void *decoder, const void *dm);

/* These readers do not check storage extents. Supply a complete
 * writable decoder/root/header/context/DM arena and readable padded input.
 * Extension storage needs 264-byte records plus sentinel backing; a full
 * DM frame can write beyond its nominal 255 records before count saturation.
 */
#endif
