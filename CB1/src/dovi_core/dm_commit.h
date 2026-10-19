#ifndef DM_COMMIT_H
#define DM_COMMIT_H

/* Explicit dependency operations are bound by dm_commit_bindings.c.
 * All operations are required when reached. Object sizes are metadata 520,
 * configuration 984, DM 4400 and kernel parameters 10928 bytes.
 * Naturally aligned, distinct full backing is required.
 */
struct dm_commit_ops {
    void *opaque;
    short (*lms_to_ipt)(void *, void *);
    short (*ipt_to_lms)(void *, void *);
    short (*invert_matrix)(void *, const void *, void *, short, short);
    void (*frame_format)(void *, const void *, void *);
    void (*ipt_input)(void *, unsigned int, unsigned int, void *);
    void (*assign_matrix)(void *, const void *, void *);
    void (*assign_vector)(void *, const void *, void *);
    void (*rgb_offset)(void *, const void *, int, int, void *);
    short (*rgb_to_lms_crosstalk)(void *, const void *, short, int, const void *, short, void *);
    short (*yuv_to_rgb_narrow)(void *, unsigned int, int, void *);
    short (*yuv_to_rgb)(void *, unsigned int, void *);
    void (*yuv_offset)(void *, unsigned int, int, void *);
    void (*range)(void *, int, unsigned int, unsigned short *, unsigned short *);
    void (*ta_tb)(void *, int, unsigned int, unsigned int, int *, int *);
    int (*power2)(void *, int, int, int);
    unsigned int (*de_gamma)(void *, unsigned short, unsigned short, unsigned short, unsigned int, unsigned short);
    short (*rgb_to_lms_default)(void *, unsigned int, void *);
    short (*rgb_to_lms_primaries)(void *, int, int, int, int, int, int, int, int, void *);
    void (*ipt_norm)(void *, unsigned int, unsigned int, void *, int *);
    void (*target_curve)(void *, const void *, void *);
    void (*hlg_signal)(void *, int, int, void *);
    void (*tone_control)(void *, const void *, const void *, void *, int);
    void (*tone_level)(void *, const void *, void *);
    int (*tone_curve)(void *, const void *, void *, void *);
    unsigned int (*output_map)(void *, void *, int, void *);
    void (*upsample)(void *, unsigned int, const void *, void *);
    void (*trim)(void *, unsigned short, void *, void *, void *);
};

unsigned int check_dm_metadata(const void *metadata, const void *previous);
void commit_dm_color_with_ops(void *metadata, void *configuration, void *color,
                              void *input_map, const struct dm_commit_ops *ops);
int commit_dm_metadata_with_ops(void *metadata, void *dm,
                                const struct dm_commit_ops *ops);
#endif
