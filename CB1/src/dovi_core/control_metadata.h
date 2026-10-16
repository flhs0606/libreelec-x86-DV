/* Packed control-path metadata and HDMI preparation operations. */
#ifndef AM6B_CONTROL_METADATA_H
#define AM6B_CONTROL_METADATA_H

#include "control_abi_recovered.h"

enum {
    CONTROL_METADATA_BASE_BYTES = 71,
    CONTROL_METADATA_EXT_BYTES = 261
};

/* The control representation is big-endian packed bytes. It differs from the
 * parser's 76-byte base / 264-byte extension representation. All objects must
 * cover the unchecked reads and writes. Offsets use 32-bit wrapping
 * arithmetic; an empty input does not force chroma or inspect old extensions.
 */
typedef int (*control_metadata_logger)(const char *format, unsigned int length);
int read_md_with_logger(void *metadata, const char *input, int size, int *offset,
                        control_metadata_logger logger);
int read_md(void *metadata, const char *input, int size, int *offset);
void filter_md(void *current, const void *previous);

/* control_bindings.h supplies the l0/ext helpers and getters. The bound
 * form accepts a trim callback; the ordinary entry uses reduce_trims.
 * reduce_trims receives ctx+0x1048 (the target pointer field), not ctx.
 * Every binding is required; callbacks may change metadata and target state.
 */
struct control_metadata_ops {
    void (*update_l0)(void *context, void *metadata);
    void (*update_ext)(void *context, void *metadata);
    void (*reduce_trims)(void *target_pointer_field, void *metadata);
};
void update_md_for_hdmi_with_ops(void *context, void *metadata,
                               const struct control_metadata_ops *ops);

/* Getter output preserves the 984-byte configuration layout.
 * TMM/CC callbacks receive NULL linear-output slots and the DM handle. */
struct control_hdmi_ops {
    void (*get_config)(void *config, void *dm_handle);
    void (*get_tmm)(unsigned short *min_pq, unsigned int *min_linear,
                   unsigned short *max_pq, unsigned int *max_linear,
                   void *dm_handle);
    void (*get_cc)(unsigned short *min_pq, unsigned int *min_linear,
                  unsigned short *max_pq, unsigned int *max_linear,
                  void *dm_handle);
};
void update_dm_l0_for_hdmi_with_ops(void *context, void *metadata,
                                  const struct control_hdmi_ops *ops);
void update_dm_ext_for_hdmi_with_ops(void *context, void *metadata,
                                   const struct control_hdmi_ops *ops);

#endif
