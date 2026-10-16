#ifndef DOVI_MEMORY_QUERY_H
#define DOVI_MEMORY_QUERY_H

/*
 * Config bytes +4 hold the level and +12 hold payload_only, both LE u32.
 * The parser wrapper passes its config+0x10 to the decoder query.
 * Inputs must be readable; the supported level domain is 0..15.
 */
unsigned long long rpu_decoder_mem_query(const unsigned char *config);
unsigned long long md_parser_mem_query(const unsigned char *config);

#endif
