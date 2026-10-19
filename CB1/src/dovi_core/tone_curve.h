#ifndef DOVI_TONE_CURVE_H
#define DOVI_TONE_CURVE_H
#include "tone_controls.h"

/* Three-point tone-curve generation. Source fields are clamped in place.
 * With ratio NULL all four LUTs are filled; nonnull ratio skips the three
 * auxiliary maps. Objects need complete backing; LUT sizes follow the finite
 * domain documented by generate_tone_curve_lut. */
void create_tone_curve_3pxx(struct tone_curve_source *source,
    struct tone_curve_display *display, struct tone_curve_pair *param1,
    struct tone_curve_pair *param2, struct tone_curve_pair *param3,
    int *slope_over_rolloff, short *tone_i, short *tone_s,
    short *saturation_i, short *saturation_s, int *ratio,
    short lut_size, short clamp_output);
const struct tone_curve_ops *tone_curve_concrete_ops(void);
#endif
