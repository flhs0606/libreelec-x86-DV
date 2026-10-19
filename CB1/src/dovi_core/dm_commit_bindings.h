#ifndef DM_COMMIT_BINDINGS_H
#define DM_COMMIT_BINDINGS_H
#include "tone_controls.h"
#include "kernel_maps.h"

/* All metadata/color/map/tone operations, including the deep curve builder,
 * are concrete. tone_curve.build NULL selects the default builder;
 * a supplied callback is an explicit override. Keep the bindings object valid.
 * Trim assertion/dump callbacks remain explicit diagnostic sinks. */
struct dm_commit_bindings {
    struct tone_curve_ops tone_curve;
    KernelTrimOps trim;
};
int commit_dm_metadata(void *metadata,void *dm,const struct dm_commit_bindings *bindings);
void commit_dm_color(void *metadata,void *configuration,void *color,void *input_map);
#endif
