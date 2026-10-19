/* Storage for metadata initialization and sample-event processing. */
#ifndef AM6B_METADATA_INIT_BODY_H
#define AM6B_METADATA_INIT_BODY_H

#include "md_parser_init.h"

/* Metadata application context. The output payloads and main_cfg
 * remain opaque here because the initializer only clears/binds them.
 */
struct metadata_app_context {
    char *md_in;
    char *comp_out;
    char *dm_out;
    struct rpu_data_header_s *rpu_data_header;
    struct vdr_rpu_data_payload_s *vdr_rpu_data_payload;
    struct vdr_dm_data_payload_s *vdr_dm_data_payload;
    unsigned char main_cfg[1800];
    char *dm_byte_sequence;
    dv_md_parser_conf_t conf;
    dv_md_parser_ctx_t *h_md_parser;
};

/* This view preserves contiguous static storage and padding,
 * including bytes that the callback does not clear.
 */
struct metadata_init_storage {
    struct metadata_app_context ctx;
    unsigned char rpu_data_header[310];
    unsigned char header_padding[2];
    unsigned char vdr_rpu_data_payload[28728];
    unsigned char vdr_dm_data_payload[67396];
    unsigned char dm_payload_padding[4];
    unsigned char dm_byte_sequence[66626];
    unsigned char sequence_padding[2];
    int rc;
};

/* The runtime supplies persistent storage, a logger and an event handler.
 * Both callbacks must be nonnull. metadata_bindings.h supplies concrete stages.
 * The flag argument is ignored.
 */
struct metadata_app_context *metadata_parser_init_body(
    struct metadata_init_storage *storage,
    md_parser_evt_cb_func_t event_handler, msg_log_func_t logger, int flag);

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(struct metadata_app_context) == 1920, "app context size");
_Static_assert(offsetof(struct metadata_app_context, md_in) == 0, "app input offset");
_Static_assert(offsetof(struct metadata_app_context, comp_out) == 8, "app composer offset");
_Static_assert(offsetof(struct metadata_app_context, dm_out) == 16, "app dm offset");
_Static_assert(offsetof(struct metadata_app_context, rpu_data_header) == 24, "app header offset");
_Static_assert(offsetof(struct metadata_app_context, vdr_rpu_data_payload) == 32, "app rpu offset");
_Static_assert(offsetof(struct metadata_app_context, vdr_dm_data_payload) == 40, "app payload offset");
_Static_assert(offsetof(struct metadata_app_context, main_cfg) == 48, "app main config offset");
_Static_assert(offsetof(struct metadata_app_context, dm_byte_sequence) == 1848, "app sequence offset");
_Static_assert(offsetof(struct metadata_app_context, conf) == 1856, "app config offset");
_Static_assert(offsetof(struct metadata_app_context, h_md_parser) == 1912, "app handle offset");
_Static_assert(offsetof(struct metadata_init_storage, rpu_data_header) == 1920, "header offset");
_Static_assert(offsetof(struct metadata_init_storage, vdr_rpu_data_payload) == 2232, "rpu payload offset");
_Static_assert(offsetof(struct metadata_init_storage, vdr_dm_data_payload) == 30960, "dm payload offset");
_Static_assert(offsetof(struct metadata_init_storage, dm_byte_sequence) == 98360, "sequence offset");
_Static_assert(offsetof(struct metadata_init_storage, rc) == 164988, "rc offset");
_Static_assert(sizeof(struct metadata_init_storage) == 164992, "metadata storage size");
#endif

#endif
