#ifndef AM6B_COMPOSER_VALIDATION_H
#define AM6B_COMPOSER_VALIDATION_H

typedef int (*composer_log_fn)(const char *format, ...);

/* Read-only validators of stable little-endian records. Logger callbacks may
 * record diagnostics but must not mutate input. Returns 0 or -2, continuing
 * after errors. DM storage includes +0x4c and num_ext_blocks(+0x4a) records of
 * stride 0x108, even when 255 is diagnosed. Composer requires 0x708 bytes.
 */
int validate_dm_payload(const void *dm, unsigned int frame,
                            composer_log_fn log, int log_level);
int validate_composer_config(const void *composer, unsigned int frame,
                                 composer_log_fn log, int log_level);

#endif
