/* Decoder fields used by reset and cleanup. */
#ifndef AM6B_RPU_LIFECYCLE_H
#define AM6B_RPU_LIFECYCLE_H

struct rpu_context_reset_view {
    unsigned char before_max_prev_rpu_ids[0x678];
    unsigned int max_prev_rpu_ids; /* +0x678 */
};

struct rpu_decoder_reset_view {
    void *rpu_data; /* +0x00000 */
    struct rpu_context_reset_view *rpu_context; /* +0x00008 */
    void *rpu_buffer; /* +0x00010 */
    int *rpu_buffer_level; /* +0x00018 */
    unsigned int rpu_input_buffer; /* +0x00020 */
    unsigned int rpu_output_buffer; /* +0x00024 */
    int last; /* +0x00028 */
    int disable_crc_check; /* +0x0002c */
    int payload_only; /* +0x00030 */
    int disable_MEL_detection; /* +0x00034 */
    unsigned int num_rpu_buffers; /* +0x00038 */
    unsigned char before_prev_sequence_header_available[60];
    int prev_sequence_header_available; /* +0x00078 */
    unsigned char before_rbsp_parse_state[202072];
    int rbsp_parse_state; /* +0x315d4 */
    void *payload_buffer[16]; /* +0x315d8 */
    void *payload_handle[15]; /* +0x31658 */
    void *payload_handle_empty; /* +0x316d0 */
    void *event_callback; /* +0x316d8 */
    unsigned char mel; /* +0x316e0 */
};

typedef char reset_pointer_is_eight_bytes[(sizeof(void *) == 8) ? 1 : -1];
typedef char reset_int_is_four_bytes[(sizeof(int) == 4) ? 1 : -1];
typedef char reset_context_prefix_is_1660_bytes[
    (sizeof(struct rpu_context_reset_view) == 1660) ? 1 : -1];
typedef char reset_decoder_is_202472_bytes[
    (sizeof(struct rpu_decoder_reset_view) == 202472) ? 1 : -1];
#if defined(__GNUC__) || defined(__clang__)
typedef char reset_payload_only_offset[
    (__builtin_offsetof(struct rpu_decoder_reset_view, payload_only) == 0x30) ? 1 : -1];
typedef char reset_context_max_prev_offset[
    (__builtin_offsetof(struct rpu_context_reset_view, max_prev_rpu_ids) == 0x678) ? 1 : -1];
typedef char reset_state_offset[
    (__builtin_offsetof(struct rpu_decoder_reset_view, rbsp_parse_state) == 0x315d4) ? 1 : -1];
typedef char reset_payload_buffer_offset[
    (__builtin_offsetof(struct rpu_decoder_reset_view, payload_buffer) == 0x315d8) ? 1 : -1];
typedef char reset_payload_handle_offset[
    (__builtin_offsetof(struct rpu_decoder_reset_view, payload_handle) == 0x31658) ? 1 : -1];
typedef char reset_mel_offset[
    (__builtin_offsetof(struct rpu_decoder_reset_view, mel) == 0x316e0) ? 1 : -1];
#endif

int rpu_decoder_reset(struct rpu_decoder_reset_view *rdc);
int rpu_decoder_cleanup(struct rpu_decoder_reset_view *rdc);

#endif
