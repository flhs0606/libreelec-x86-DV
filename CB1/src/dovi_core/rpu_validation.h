#ifndef DOVI_RPU_VALIDATION_H
#define DOVI_RPU_VALIDATION_H

typedef void (*rpu_log_fn)(const char *format, ...);

/* `rpu_data` has a header pointer at +0 and a payload pointer at +8. */
int validate_level(const void *rpu_data, rpu_log_fn msg_log,
                   unsigned int print_severity);
int validate_profile(const void *rpu_data, rpu_log_fn msg_log,
                     unsigned int print_severity);

#endif
