#ifndef DOVI_RPU_HEADER_H
#define DOVI_RPU_HEADER_H

#define RPU_HEADER_SIZE 310u
#define RPU_HEADER_DECODER_SIZE 202472u

typedef void (*rpu_header_log_fn)(const char *format, ...);
typedef void (*rpu_header_event_fn)(unsigned int severity,
                                       unsigned int event_code,
                                       const char *message);

/*
 * decoder+0 points to a 24-byte rpu_data object whose first pointer is the
 * writable 310-byte header. decoder+0x40 contains a 32-byte bitstream.
 * See dovi_abi_recovered.h for the shared decoder and header layouts.
 * A valid padded bitstream and writable decoder/header backing are required.
 */
int rpu_header_read(void *decoder);

#endif
