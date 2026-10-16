/* Decoder entry points with the parsing and validation stages bound. */
#ifndef AM6B_DECODER_BINDINGS_H
#define AM6B_DECODER_BINDINGS_H

/* Same unchecked storage contract as rpu_output.h. All parsing and
 * validation stages are bound in decoder_bindings.c; no operation table is needed.
 * This binds the decoder core, not the T4c kernel host callback table. */
int rpu_decoder_process_buffer(void *decoder, unsigned char *buffer,
    int size, void **header, void **payload, int *payload_size,
    void **dm_payload, int *dm_payload_size);
int rpu_decoder_get_output(void *decoder, void **header, void **payload,
    int *payload_size, void **dm_payload, int *dm_payload_size);

#endif
