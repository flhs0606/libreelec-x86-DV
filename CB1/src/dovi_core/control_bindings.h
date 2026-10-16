/* HDMI metadata preparation with optional callback bindings. */
#ifndef CONTROL_BINDINGS_H
#define CONTROL_BINDINGS_H

/* Requires valid context->h_dm active configuration and a non-null selected
 * control_get_dm_cfg source. Callers ignore getter failures and would consume
 * uninitialized locals.
 */
void update_dm_l0_for_hdmi(void *context, void *metadata);
void update_dm_ext_for_hdmi(void *context, void *metadata);
void update_md_for_hdmi(void *context, void *metadata);

/* reduce_trims is required only when serialized size triggers its branch.
 * It receives context+CONTROL_CONTEXT_TARGET_PARAM_OFFSET and metadata.
 * Metadata/context may be changed by it; the caller rereads them. */
typedef void (*control_reduce_trims_callback)(void *target_pointer_field,
                                              void *metadata);
void update_md_for_hdmi_bound(void *context, void *metadata,
                             control_reduce_trims_callback reduce_trims);

#endif
