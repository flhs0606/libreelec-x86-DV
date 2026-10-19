/* Parser status, sample-count and error-message accessors. */
#include "parser_diagnostics.h"

typedef char parser_requires_64_bit_pointers[(sizeof(void *) == 8) ? 1 : -1];
typedef char parser_requires_32_bit_int[(sizeof(int) == 4) ? 1 : -1];
typedef char parser_requires_64_bit_ull[(sizeof(unsigned long long) == 8) ? 1 : -1];

static unsigned int read_le32(const unsigned char *bytes)
{
    return (unsigned int)bytes[0]
         | ((unsigned int)bytes[1] << 8)
         | ((unsigned int)bytes[2] << 16)
         | ((unsigned int)bytes[3] << 24);
}

static unsigned long long read_le_pointer(const unsigned char *bytes)
{
    unsigned long long value = 0;
    unsigned int index;

    for (index = 0; index < 8; ++index)
        value |= (unsigned long long)bytes[index] << (8 * index);
    return value;
}

static const char unknown_error[] = "Unknown error_code";

/* Array index is error_code + 22 for the inclusive range [-22, 0]. */
static const char *const error_strings[23] = {
    "The nalu input buffer is full. Increase NUM_RPU_BUFFERS?",
    "End of stream",
    "Need more data",
    unknown_error,
    unknown_error,
    unknown_error,
    unknown_error,
    unknown_error,
    "Value is out of it's legal range",
    "Mapping or NLQ pivot values are to large for current bitdepth",
    "Output buffer is too small",
    "DM metadata extension can be 1 or 2 only",
    "invalid DM metadata extension size",
    "Wrong RPU version number",
    "Unspecified error",
    "Invalid argument",
    "The rpu stream violates the spatial filter and mode limitations",
    "The rpu stream violates the profile limitations",
    "The rpu stream violates the level limitations",
    "Not supported",
    "Invalid RPU, skip frame",
    "Not enough memory available",
    "RPU_ERR_NONE",
};

const char *rpu_error_code_2_str(int error_code)
{
    if (error_code < -22 || error_code > 0)
        return unknown_error;
    return error_strings[error_code + 22];
}

const char *dv_md_parser_get_errstr(int error_code)
{
    return rpu_error_code_2_str(error_code > 11 ? 12 : error_code);
}

int dv_md_parser_get_last_err(const void *handle)
{
    unsigned int bits;

    if (!handle)
        return 10;
    bits = read_le32((const unsigned char *)handle + 0x18);
    if (bits <= 0x7fffffffu)
        return (int)bits;
    return -1 - (int)~bits;
}

void *dv_md_parser_get_rpu_dcd_ctx(const void *handle)
{
    if (!handle || *(const unsigned char *)handle == 0)
        return 0;
    return (void *)read_le_pointer((const unsigned char *)handle + 0x38);
}
