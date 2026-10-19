/* Accessors for DM configuration and luminance bounds. */
#ifndef CONTROL_GETTERS_H
#define CONTROL_GETTERS_H

/* Copy the 984-byte to-be configuration when its validity flag is nonzero,
 * otherwise the active configuration. Invalid/null handles or selected source
 * pointers return -1 with destination unchanged. Copy runs forward bytewise,
 * including when source/destination overlap; storage must cover every access. */
int control_get_dm_cfg(void *config, const void *dm_handle);

/* Optional outputs are written in min_pq, min_linear, max_pq, max_linear
 * order. Invalid/null active handles return -1 with all outputs unchanged.
 * Linear values use the clamped PQ lookup/interpolation. */
int control_get_stb_cc(unsigned short *min_pq, unsigned int *min_linear,
                       unsigned short *max_pq, unsigned int *max_linear,
                       const void *dm_handle);

#endif
