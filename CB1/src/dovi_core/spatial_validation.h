#ifndef AM6B_SPATIAL_VALIDATION_H
#define AM6B_SPATIAL_VALIDATION_H
#include "rpu_validation.h"

/* Exact table shape: mode, component, sentinel-terminated allowed IDs. */
extern const int ver_spatial_filter_allowed[8][3][8];
extern const int hor_spatial_filter_allowed[8][3][8];

/* rpu_data+0 holds a readable 310-byte header pointer. Vertical mode at
 * header+0xec and horizontal mode at +0xe8 must index their 8-row tables.
 * Logger may mutate header bytes; diagnostics re-read fields after logging.
 * Invalid indices and pointer/concurrency faults are outside the contract.
 */
int validate_spatial_filter(const void *rpu_data, rpu_log_fn logger,
                               unsigned int severity);
#endif
