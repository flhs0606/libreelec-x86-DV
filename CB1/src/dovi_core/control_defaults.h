#ifndef DOVI_CONTROL_DEFAULTS_H
#define DOVI_CONTROL_DEFAULTS_H

#include "control_abi_recovered.h"

/* Fixed-value DM defaults preserve every field/padding byte not explicitly
 * written by the helper. Buffers are ordinary readable/writable
 * memory, may be unaligned, and must satisfy the corresponding DM_*_BYTES.
 * No helper performs null/bounds checks. Extensions and configuration must
 * not overlap. Extensions stores two pointers to its own trim-array storage;
 * that object must remain at the same address after initialization. */
void control_init_grc_signal(void *signal);
void control_init_src_signal(void *signal);
void control_init_tgt_signal(void *signal);
void control_init_tone(void *tone);
/* The function ignores platform and always selects CPlatformCpu. h_mmg is
 * stored verbatim without dereferencing; it can be null. Return is config. */
void *control_init_config(int platform, void *h_mmg, void *config);
void control_init_extensions(const void *config, void *extensions);

#endif
