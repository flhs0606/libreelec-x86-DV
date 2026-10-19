/* Reset decoder buffering, payload handles and sequence state; cleanup is a no-op. */
#include "rpu_lifecycle.h"

enum { RBSP_SEARCH_RPU = 0, RBSP_FOUND_RPU = 4 };

int rpu_decoder_reset(struct rpu_decoder_reset_view *rdc)
{
    unsigned int count, i, max_prev;

    if (rdc->payload_only != 0) {
        rdc->rbsp_parse_state = RBSP_FOUND_RPU;
        rdc->num_rpu_buffers = 1;
        rdc->rpu_input_buffer = 0;
        rdc->rpu_output_buffer = 0;
        rdc->rpu_buffer_level[0] = 0;
        rdc->mel = 0;
        return 0;
    }

    rdc->rbsp_parse_state = RBSP_SEARCH_RPU;
    rdc->rpu_input_buffer = 0;
    rdc->rpu_output_buffer = 0;
    count = rdc->num_rpu_buffers;
    for (i = 0; i < count; ++i)
        rdc->rpu_buffer_level[i] = 0;

    rdc->prev_sequence_header_available = 0;
    max_prev = rdc->rpu_context->max_prev_rpu_ids;
    count = max_prev == ~0u ? 0 : max_prev + 1;
    for (i = 0; i < count; ++i)
        rdc->payload_handle[i] = rdc->payload_buffer[i];
    rdc->payload_handle_empty = rdc->payload_buffer[count];

    rdc->last = 0;
    rdc->rpu_input_buffer = rdc->num_rpu_buffers - 1;
    rdc->rpu_output_buffer = 0;
    for (i = 0; i < rdc->num_rpu_buffers; ++i)
        rdc->rpu_buffer_level[i] = 0;
    rdc->mel = 0;
    return 0;
}

int rpu_decoder_cleanup(struct rpu_decoder_reset_view *rdc)
{
    (void)rdc;
    return 0;
}
