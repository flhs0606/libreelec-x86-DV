/* Decoder fields used by byte ingress and buffer diagnostics. */
#ifndef AM6B_RPU_PUSH_DATA_H
#define AM6B_RPU_PUSH_DATA_H

#include "rpu_setters.h"

enum rbsp_ingress_state {
    RBSP_SEARCH_RPU = 0,
    RBSP_FOUND_START_00 = 1,
    RBSP_FOUND_START_0000 = 2,
    RBSP_FOUND_START_000001 = 3,
    RBSP_FOUND_RPU = 4,
    RBSP_FOUND_00 = 5,
    RBSP_FOUND_0000 = 6
};

struct rpu_push_view {
    void *rpu_data;
    void *rpu_context;
    unsigned char *rpu_buffer; /* +0x10; contiguous 4096-byte slots */
    int *rpu_buffer_level; /* +0x18 */
    unsigned int rpu_input_buffer; /* +0x20 */
    unsigned int rpu_output_buffer;
    int last; /* +0x28 */
    int disable_crc_check;
    int payload_only; /* +0x30 */
    int disable_MEL_detection;
    unsigned int num_rpu_buffers; /* +0x38 */
    unsigned char before_msg_log[0x3158c];
    rpu_msg_log_func_t msg_log; /* +0x315c8 */
    unsigned int print_severity; /* +0x315d0; unsigned comparison */
    unsigned int rbsp_parse_state; /* +0x315d4 */
};

typedef char push_view_size[(sizeof(struct rpu_push_view) == 0x315d8) ? 1 : -1];
#if defined(__GNUC__) || defined(__clang__)
typedef char push_state_offset[
    (__builtin_offsetof(struct rpu_push_view, rbsp_parse_state) == 0x315d4) ? 1 : -1];
typedef char push_logger_offset[
    (__builtin_offsetof(struct rpu_push_view, msg_log) == 0x315c8) ? 1 : -1];
#endif

/* Nested buffers must be valid for every unchecked read/write. The function
 * checks a stream slot's level only AFTER writes, and payload-only mode scans
 * backwards for a nonzero byte without checking the input's lower bound.
 * Input and slot buffers must not overlap; logging must not mutate storage.
 * Supply a nonzero payload byte and padding for every accessed byte.
 */
int rpu_decoder_push_data(struct rpu_push_view *rdc,
                              const unsigned char *buffer,
                              int buffer_size, int last);
void dump_rpu_buffer_info(struct rpu_push_view *rdc);

#endif
