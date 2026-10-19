/* Connect decoder processing to header, payload and validation stages. */
#include "decoder_bindings.h"
#include "rpu_output.h"
#include "rpu_header.h"
#include "rpu_validation.h"
#include "spatial_validation.h"
#include "rpu_payload.h"
#include "detect_mel.h"
#include "dm_payload.h"
#include "rpu_push_data.h"

static int detect_mel_stage(void *decoder)
{
    return detect_mel((const struct mel_decoder *)decoder);
}

static void dump_buffer_info(void *decoder)
{
    dump_rpu_buffer_info((struct rpu_push_view *)decoder);
}

static const struct rpu_process_ops process_ops = {
    rpu_header_read,
    validate_level,
    validate_spatial_filter,
    rpu_payload_read,
    validate_profile,
    detect_mel_stage,
    dm_data_payload_read
};

int rpu_decoder_process_buffer(void *decoder, unsigned char *buffer,
    int size, void **header, void **payload, int *payload_size,
    void **dm_payload, int *dm_payload_size)
{
    return rpu_decoder_process_buffer_with_ops(decoder, buffer, size,
        header, payload, payload_size, dm_payload, dm_payload_size, &process_ops);
}

static const struct rpu_output_ops output_ops = {
    rpu_decoder_process_buffer,
    dump_buffer_info
};

int rpu_decoder_get_output(void *decoder, void **header, void **payload,
    int *payload_size, void **dm_payload, int *dm_payload_size)
{
    return rpu_decoder_get_output_with_ops(decoder, header, payload,
        payload_size, dm_payload, dm_payload_size, &output_ops);
}
