/* decoder output and processing control flow, with explicit call boundaries. */
#ifndef AM6B_RPU_OUTPUT_H
#define AM6B_RPU_OUTPUT_H

typedef void (*output_log_fn)(const char *format, ...);
typedef int (*output_read_fn)(void *decoder);
typedef int (*output_validate_fn)(const void *rpu_data,
                                     output_log_fn log, unsigned int severity);

/* Every member is required when its branch is reached. decoder_bindings.c
 * supplies the parsing and validation operations used by the runtime. */
struct rpu_process_ops {
    output_read_fn header_read;
    output_validate_fn validate_level;
    output_validate_fn validate_spatial_filter;
    output_read_fn payload_read;
    output_validate_fn validate_profile;
    output_read_fn detect_mel;
    output_read_fn dm_payload_read;
};

typedef int (*output_process_fn)(void *decoder, unsigned char *buffer,
                                    int size, void **header, void **payload,
                                    int *payload_size, void **dm_payload,
                                    int *dm_payload_size);
struct rpu_output_ops {
    output_process_fn process_buffer;
    void (*dump_buffer_info)(void *decoder);
};

/* The opaque decoder uses a 202472-byte context. Buffers are 4096
 * bytes each. Valid indices/backing and five distinct writable output slots
 * are required. These preserve the unchecked API, including queue
 * consumption on processing errors and the mask-based ring advance. */
int rpu_decoder_get_output_with_ops(void *decoder, void **header,
    void **payload, int *payload_size, void **dm_payload, int *dm_payload_size,
    const struct rpu_output_ops *ops);

/* Uses bitstream_init and rpu_error_code_2_str directly; parsing and
 * validation are dispatched through the seven operations above. */
int rpu_decoder_process_buffer_with_ops(void *decoder, unsigned char *buffer,
    int size, void **header, void **payload, int *payload_size,
    void **dm_payload, int *dm_payload_size, const struct rpu_process_ops *ops);

#endif
