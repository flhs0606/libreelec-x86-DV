/* Prefix of rpu_decoder_ctx_s used by rpu_decoder_last_rpu_output. */
#ifndef AM6B_RPU_LAST_OUTPUT_H
#define AM6B_RPU_LAST_OUTPUT_H

struct rpu_decoder_prefix {
    void *rpu_data;             /* +0x00 */
    void *rpu_context;          /* +0x08 */
    void *rpu_buffer;           /* +0x10; pointee unused here */
    int *rpu_buffer_level;      /* +0x18 */
    unsigned int rpu_input_buffer;  /* +0x20 */
    unsigned int rpu_output_buffer; /* +0x24 */
    int last;                   /* +0x28 */
};

typedef char rpu_last_output_pointer_is_eight_bytes[
    (sizeof(void *) == 8) ? 1 : -1];
typedef char rpu_last_output_int_is_four_bytes[
    (sizeof(int) == 4) ? 1 : -1];
typedef char rpu_last_output_prefix_is_48_bytes[
    (sizeof(struct rpu_decoder_prefix) == 48) ? 1 : -1];
#if defined(__GNUC__) || defined(__clang__)
typedef char rpu_last_output_level_offset[
    (__builtin_offsetof(struct rpu_decoder_prefix, rpu_buffer_level) == 0x18) ? 1 : -1];
typedef char rpu_last_output_index_offset[
    (__builtin_offsetof(struct rpu_decoder_prefix, rpu_output_buffer) == 0x24) ? 1 : -1];
typedef char rpu_last_output_last_offset[
    (__builtin_offsetof(struct rpu_decoder_prefix, last) == 0x28) ? 1 : -1];
#endif

/* The caller must supply valid pointers and indices. */
int rpu_decoder_last_rpu_output(const struct rpu_decoder_prefix *rdc);

#endif
