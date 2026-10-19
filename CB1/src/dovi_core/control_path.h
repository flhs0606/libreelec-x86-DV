/* Control callback with explicit persistent storage and service bindings. */
#ifndef CONTROL_PATH_H
#define CONTROL_PATH_H

#include "control_init.h"
#include "control_metadata.h"
#include "control_bindings.h"
#include "control_output_abi.h"

struct register_commit_ops;
typedef int (*control_commit_callback)(void *context, int input_format,
    int output_format, void *metadata, const void *composition, const void *target,
    const void *graphics, const void *source, int priority, void *register1,
    void *register2, void *register3, void *lut1, void *lut2,
    const struct register_commit_ops *ops);

/* These are persistent objects supplied by their owner. Arena
 * covers 20040 bytes, cli 112, metadata1/2 66626 each, composition 1800,
 * register1 900, register2 96 and register3 1024. initialized is a byte.
 * Distinct writable objects and full readable input backing are required.
 * The function ignores physical composition size and consumes 1800-byte
 * records until its equality-based termination condition is reached.
 */
struct control_path_storage {
    void *arena;
    void *cli;
    void *metadata1;
    void *metadata2;
    void *composition;
    void *register1;
    void *register2;
    void *register3;
    unsigned char initialized;
    int input_format;
    int output_format;
};

struct control_path_ops {
    control_commit_callback commit;
    const struct register_commit_ops *commit_ops;
    int (*cpu_version)(int level);
    control_metadata_logger logger;
    const struct control_memory_ops *memory;
    control_reduce_trims_callback reduce_trims;
};
/* A NULL reduce_trims selects the reduce_trims implementation.
 * commit can be commit_reg_with_ops; its metadata binding remains explicit. */

/* output covers the 13468-byte prefix described by control_output_abi.h.
 * module.c checks that prefix against the host output. FORMAT_INVALID clears only
 * initialized. Every successful ordinary call destroys the context without
 * clearing initialized.
 */
int control_path_with_ops(struct control_path_storage *storage,
    const struct control_path_ops *ops, int input_format, int output_format,
    const char *composition, int composition_size, const char *metadata,
    int metadata_size, int priority, int bit_depth, int chroma_format,
    int yuv_range, int graphics_min, int graphics_max, int target_min,
    int target_max, int no_el, const void *hdr10, void *output);

#endif
