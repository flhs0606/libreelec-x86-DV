/* Accumulate RPU bytes into decoder slots and report buffer state. */
#include "rpu_push_data.h"

static int signed_bits(unsigned int value)
{
    return value <= 0x7fffffffu ? (int)value : -1 - (int)~value;
}

static void append_byte(struct rpu_push_view *rdc, unsigned char byte)
{
    unsigned int slot = rdc->rpu_input_buffer;
    int old_level = rdc->rpu_buffer_level[slot];
    unsigned char *destination = rdc->rpu_buffer + (unsigned long long)slot * 4096;
    rdc->rpu_buffer_level[slot] = signed_bits((unsigned int)old_level + 1u);
    destination[old_level] = byte;
}

void dump_rpu_buffer_info(struct rpu_push_view *rdc)
{
    unsigned int i;
    if (!rdc->msg_log)
        return;
    rdc->msg_log("RPU buffer:\n");
    rdc->msg_log("Level   ");
    for (i = 0; i < rdc->num_rpu_buffers; ++i)
        rdc->msg_log(" %4d", rdc->rpu_buffer_level[i]);
    rdc->msg_log("\n");
    rdc->msg_log("Buffer  ");
    for (i = 0; i < rdc->num_rpu_buffers; ++i)
        rdc->msg_log(" %4d", i);
    rdc->msg_log("\n");
}

int rpu_decoder_push_data(struct rpu_push_view *rdc,
                              const unsigned char *buffer,
                              int buffer_size, int last)
{
    int i;
    if (!rdc || buffer_size < 0 || (buffer_size > 0 && !buffer))
        return -7;
    if (last)
        rdc->last = 1;

    if (buffer_size && rdc->payload_only) {
        unsigned long long count, offset;
        unsigned char *destination;
        while (buffer[buffer_size - 1] == 0)
            --buffer_size;
        count = (unsigned long long)(long long)buffer_size;
        destination = rdc->rpu_buffer + (unsigned long long)rdc->rpu_input_buffer * 4096;
        for (offset = 0; offset < count; ++offset)
            destination[offset] = buffer[offset];
        rdc->rpu_buffer_level[rdc->rpu_input_buffer] = buffer_size;
    } else {
        for (i = 0; i < buffer_size; ++i) {
            unsigned char byte = buffer[i];
            switch (rdc->rbsp_parse_state) {
            case RBSP_SEARCH_RPU:
                if (byte == 0)
                    rdc->rbsp_parse_state = RBSP_FOUND_START_00;
                break;
            case RBSP_FOUND_START_00:
                rdc->rbsp_parse_state = byte == 0 ? RBSP_FOUND_START_0000 : RBSP_SEARCH_RPU;
                break;
            case RBSP_FOUND_START_0000:
                if (byte == 1)
                    rdc->rbsp_parse_state = RBSP_FOUND_START_000001;
                else
                    rdc->rbsp_parse_state = byte == 0 ? RBSP_FOUND_START_0000 : RBSP_SEARCH_RPU;
                break;
            case RBSP_FOUND_START_000001:
                if (byte == 0x19) {
                    unsigned int next = (rdc->num_rpu_buffers - 1u) & (rdc->rpu_input_buffer + 1u);
                    rdc->rpu_input_buffer = next;
                    if (rdc->rpu_buffer_level[next] != 0) {
                        dump_rpu_buffer_info(rdc);
                        return -22;
                    }
                    rdc->rbsp_parse_state = RBSP_FOUND_RPU;
                } else {
                    rdc->rbsp_parse_state = RBSP_SEARCH_RPU;
                }
                break;
            case RBSP_FOUND_RPU:
                if (byte == 0)
                    rdc->rbsp_parse_state = RBSP_FOUND_00;
                else
                    append_byte(rdc, byte);
                break;
            case RBSP_FOUND_00:
                if (byte == 0) {
                    rdc->rbsp_parse_state = RBSP_FOUND_0000;
                } else {
                    rdc->rbsp_parse_state = RBSP_FOUND_RPU;
                    append_byte(rdc, 0);
                    append_byte(rdc, byte);
                }
                break;
            case RBSP_FOUND_0000:
                if (byte == 3) {
                    append_byte(rdc, 0);
                    append_byte(rdc, 0);
                    rdc->rbsp_parse_state = RBSP_FOUND_RPU;
                } else if (byte == 1) {
                    rdc->rbsp_parse_state = RBSP_FOUND_START_000001;
                } else if (byte == 0) {
                    rdc->rbsp_parse_state = RBSP_FOUND_START_0000;
                } else if (byte == 2) {
                    return -1;
                } else {
                    rdc->rbsp_parse_state = RBSP_FOUND_RPU;
                    append_byte(rdc, 0);
                    append_byte(rdc, 0);
                    append_byte(rdc, byte);
                }
                break;
            default:
                break;
            }
            if (rdc->rpu_buffer_level[rdc->rpu_input_buffer] > 4096) {
                dump_rpu_buffer_info(rdc);
                return -22;
            }
        }
    }
    if (rdc->print_severity > 6u)
        dump_rpu_buffer_info(rdc);
    return 0;
}
