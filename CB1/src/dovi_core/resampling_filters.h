#ifndef AM6B_RESAMPLING_FILTERS_H
#define AM6B_RESAMPLING_FILTERS_H

/* Raw decoder layout, not the T4c host callback ABI. decoder+0 is an
 * rpu_data pointer: +0 header, +8 payload. decoder+8 is the RPU context;
 * decoder+0x40 is bitstream; +0x315c8 log callback, +0x315d0 severity.
 * Payload partition stride is 0x7038 bytes.
 * All objects and padded bitstream input must be accessible. These routines
 * do not check indices or byte lengths.
 */
int rpu_data_chroma_resampling_filter_1D_exp_read(
    void *decoder, int partition, int component, int direction);
int rpu_data_spatial_resampling_filter_exp_read(
    void *decoder, int pivot, int direction, int phases);

#endif
