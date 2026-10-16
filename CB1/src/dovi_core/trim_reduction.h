#ifndef DOVI_TRIM_REDUCTION_H
#define DOVI_TRIM_REDUCTION_H
/* Packed control metadata: count at byte 70, records at 71 with stride 261.
 * reduce_trims requires at most sixteen level-2 and sixteen level-8 records,
 * matching the fixed local arrays. target_pointer_address points to
 * an actual 64-bit pointer to the 88-byte target descriptor, not to a context.
 * Direct sorting requires caller arrays large enough for all level-2 records.
 */
int reduce_trims(const void *target_pointer_address, void *packed_metadata);
int sort_l2_by_tmax_lum(const void *packed_metadata, int *indices, int *max_pq);
unsigned int find_chroma_dist(const int *primaries, unsigned int linear, int pq_scaled);
extern _Thread_local unsigned char chroma_distance_state[116];
extern const int chroma_xyz_to_lms[3][3];
extern const short chroma_lms_to_ictcp[3][3];
extern const int trim_display_table[7][4];
extern const int trim_primaries_table[20][8];
#endif
