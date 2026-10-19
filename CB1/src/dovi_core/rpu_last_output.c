/* Report whether the decoder has consumed its final queued output. */
#include "rpu_last_output.h"

int rpu_decoder_last_rpu_output(const struct rpu_decoder_prefix *rdc)
{
    if (rdc->rpu_buffer_level[rdc->rpu_output_buffer] != 0)
        return 0;
    return rdc->last != 0;
}
