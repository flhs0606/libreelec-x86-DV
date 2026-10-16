/* Decoder fields used by the MEL predicate. */
#ifndef AM6B_DETECT_MEL_H
#define AM6B_DETECT_MEL_H

#define MEL_PAYLOAD_MIN_SIZE 0x68c8u

struct mel_rpu_root {
    const void *header;
    const unsigned char *payload;
};

struct mel_decoder {
    const struct mel_rpu_root *root;
};

/* Caller supplies a decoder with a readable 0x68c8-byte payload. */
int detect_mel(const struct mel_decoder *decoder);

#endif
