#ifndef AM6B_LUMINANCE_PQ_H
#define AM6B_LUMINANCE_PQ_H

/* Decoder luminance-to-PQ conversion, saturated to the 12-bit range. */
unsigned int luminance_to_pq12(unsigned int value);

#endif
