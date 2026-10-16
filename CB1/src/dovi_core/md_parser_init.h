/* Metadata parser creation and initialization. */
#ifndef AM6B_MD_PARSER_INIT_H
#define AM6B_MD_PARSER_INIT_H

#include "dv_md_parser_abi_recovered.h"

/* Static md_parser object: context followed by decoder storage.
 * create clears only the 112-byte parser prefix and always returns this
 * singleton. The fixed singleton is suitable for the module's level-0,
 * payload-only callback configuration; arbitrary flags need their own arena.
 */
#define MD_PARSER_SINGLETON_BYTES 334048u
dv_md_parser_ctx_t *dv_md_parser_create(void);

/* A nonnull handle needs decoder backing storage at handle+112. Config and
 * handle must not overlap. A null handle returns 2 without reading config.
 */
dv_md_parser_rc_t dv_md_parser_init(dv_md_parser_ctx_t *handle,
                                      const dv_md_parser_conf_t *config);

/* Release is a no-op; it does not reset or free parser storage. */
void metadata_parser_release(void);

#endif
