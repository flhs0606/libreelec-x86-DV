/* Bitstream state shared by the RPU and metadata readers. */
#ifndef AM6B_RECONSTRUCTED_BITSTREAM_H
#define AM6B_RECONSTRUCTED_BITSTREAM_H

struct bitstream {
    unsigned int cache;          /* +0: next input bits, MSB first */
    unsigned int deficit;        /* +4: signed 32-bit refill position */
    const unsigned char *next;   /* +8: next two-byte refill */
    const unsigned char *tail;   /* +16: final byte / adjusted stop position */
    unsigned int tail_bits;      /* +24: bits preceding final stop bit */
    unsigned int bits_read;      /* +28: wrapping counter */
};

/*
 * These functions use an unchecked reader with two-byte cache refills. Supply a writable 32-byte state and a readable, padded input buffer;
 * at least four initial bytes and every two-byte refill must be accessible.
 * Reads are not bounds-checked. size is the nominal byte length
 * and must be >= 2 here. The unsigned fixed-width reader accepts
 * byte-sized widths 0..255 from the header parser; outside 1..32 its
 * masked-shift behavior is preserved, not a conventional wide-integer read.
 * Use widths 1..32 for signed fixed-width reads. Exp-Golomb
 * readers stop counting after 31 leading zero bits, even without a separator.
 */
void bitstream_init(struct bitstream *state,
                         const unsigned char *input, unsigned int size);
unsigned int read_u_1(struct bitstream *state);
unsigned int read_u_v(unsigned int width, struct bitstream *state);
int read_i_v(unsigned int width, struct bitstream *state);
unsigned int read_ue_v(struct bitstream *state);
int read_se_v(struct bitstream *state);

#endif
