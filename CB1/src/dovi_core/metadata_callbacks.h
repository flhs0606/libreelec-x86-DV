/* Host metadata callback bodies with injectable core bindings. */
#ifndef AM6B_METADATA_CALLBACKS_H
#define AM6B_METADATA_CALLBACKS_H

#include "metadata_init_body.h"

/* Persistent event counters. frame_nr is incremented for every
 * event, including errors and unknown events, and is never reset by process.
 */
struct metadata_counters {
    unsigned int frame_nr;
    int comp_size;
    int md_size;
};

/* These bindings are required only on branches that call them. They are the
 * injectable dependency boundary. metadata_bindings.h connects the
 * composer, validators, serializer and parser stages for concrete execution.
 * All caller buffers must cover the actual reads/writes; memcpy ranges must
 * be disjoint. The helpers preserve the unchecked pointer contract.
 */
struct metadata_callback_ops {
    void (*compose)(const void *header, const void *rpu, void *main_cfg);
    int (*validate_composer)(const void *main_cfg, unsigned int frame,
                             msg_log_func_t logger, int severity);
    int (*validate_dm)(const void *dm, unsigned int frame,
                       msg_log_func_t logger, int severity);
    int (*serialize_dm)(const void *dm, char *output, int *length);
    int (*process)(dv_md_parser_ctx_t *parser, unsigned int au_flags,
                   const timestamp_t *pts, const unsigned char *input,
                   unsigned int size, int last, int *flags);
    int (*reset)(dv_md_parser_ctx_t *parser);
    msg_log_func_t logger;
};

void md_parser_evt_handler_body(
    struct metadata_app_context *ctx, struct metadata_counters *counters,
    unsigned int event, const timestamp_t *pts, const void *header,
    const void *rpu, int rpu_size, const void *dm, int dm_size, int status,
    const struct metadata_callback_ops *ops);

int metadata_parser_process_body(
    struct metadata_init_storage *storage,
    struct metadata_counters *counters, const char *input, int size,
    char *composer, int *composer_size, char *metadata, int *metadata_size,
    unsigned char eos, const struct metadata_callback_ops *ops);

int metadata_parser_reset_body(
    struct metadata_init_storage *storage, int flag,
    const struct metadata_callback_ops *ops);

#endif
