/* Metadata callback pipeline backed by the parser singleton. */
#ifndef AM6B_METADATA_BINDINGS_H
#define AM6B_METADATA_BINDINGS_H

#include "metadata_callbacks.h"

/* Caller-owned app storage and counters; the parser/decoder is a shared
 * singleton, so only one pipeline may be active. Initialization replaces its
 * callback context and preserves counters. Start with zeroed, aligned storage.
 * Logger must be nonnull and stable, and may record messages but must not
 * mutate input/storage or recursively enter pipeline operations. Input and
 * outputs retain the unchecked buffer contracts of metadata_callbacks.h:
 * composer needs 8192 writable bytes and metadata needs 1024, disjoint from
 * input/storage. Input needs readable padding and a nonzero payload byte;
 * lengths do not guard the trailing-zero scans. Init leaves output
 * pointers null; bind them before invoking a direct sample event or T35 call.
 * This is a core binding, not the T4c host registration adapter.
 */
struct metadata_pipeline {
    struct metadata_init_storage storage;
    struct metadata_counters counters;
    msg_log_func_t logger;
};

struct metadata_app_context *metadata_pipeline_init(
    struct metadata_pipeline *pipeline, msg_log_func_t logger, int flag);
int metadata_pipeline_process(struct metadata_pipeline *pipeline,
    const char *input, int size, char *composer, int *composer_size,
    char *metadata, int *metadata_size, unsigned char eos);
int metadata_pipeline_reset(struct metadata_pipeline *pipeline, int flag);

/* Also callable for the parser event ABI with pipeline->storage.ctx as
 * context. Real composer, validators and serializer execute on sample events.
 */
void metadata_pipeline_event(void *context, dv_md_parser_evt_t event,
    timestamp_t *pts, struct rpu_data_header_s *header,
    struct vdr_rpu_data_payload_s *rpu, int rpu_size,
    struct vdr_dm_data_payload_s *dm, int dm_size, int status);

_Static_assert(offsetof(struct metadata_pipeline, storage) == 0,
               "pipeline starts with metadata storage");
_Static_assert(offsetof(struct metadata_init_storage, ctx) == 0,
               "metadata storage starts with event context");

#endif
