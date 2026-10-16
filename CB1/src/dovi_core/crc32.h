#ifndef DOVI_RPU_CRC32_H
#define DOVI_RPU_CRC32_H

/*
 * CRC helper for RPU data.
 * The seed is complemented before consuming bytes; the result is not
 * complemented. A zero length accepts any data pointer, including NULL.
 */
unsigned int rpu_crc32(unsigned int seed,
                          const unsigned char *data,
                          unsigned long long length);

#endif
