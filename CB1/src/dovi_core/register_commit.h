#ifndef DOVI_REGISTER_COMMIT_H
#define DOVI_REGISTER_COMMIT_H

#include "control_bindings.h"

/* Exact 984-byte configuration commit. Valid dm is required: the function
 * reads dm->pDmCfgToBe before its nominal null check. ks can be null only on
 * the unchanged-configuration return. Layouts are defined in dm_commit_abi.h. */
int control_commit_dm_config(const void *config, void *kernel, void *dm);

/* A NULL commit_config selects control_commit_dm_config; a nonnull callback
 * overrides it. commit_metadata is required and is supplied by
 * dm_commit_bindings.c in the runtime. NULL reduce_trims selects reduce_trims.
 * Callbacks may replace context pointers, which are reread.
 */
struct register_commit_ops {
    int (*commit_config)(const void *config, void *kernel, void *dm);
    int (*commit_metadata)(void *extensions, void *dm);
    control_reduce_trims_callback reduce_trims;
};

/* Register commit with explicit operation bindings. Registers cover 900,
 * 96 and 1024 bytes; LUTs cover 5120 bytes each. Raw metadata serialization
 * is unchecked and may exceed the nominal 844-byte raw register array; the
 * caller must supply every byte accessed. Objects must remain disjoint.
 */
int commit_reg_with_ops(void *context, int input_format, int output_format,
                        void *metadata, const void *composition,
                        const void *target, const void *graphics,
                        const void *source, int priority,
                        void *register1, void *register2, void *register3,
                        void *lut1, void *lut2,
                        const struct register_commit_ops *ops);

#endif
