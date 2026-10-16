/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef DOVI_BRIDGE_H
#define DOVI_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/* User-space runtime bridge for dovi_core on x86 platforms. */

/* CPU Version: Fixed at 0x2b (S922X-J / T4c flagship) to prevent register/LUT zeroing */
int dovi_bridge_cpu_version(int level);

/* Diagnostic and error logging */
int dovi_bridge_control_log(const char *format, unsigned int length);
int dovi_bridge_memory_log(const char *format, const char *file,
                           const char *function, unsigned int line,
                           const char *expression);
void dovi_bridge_trim_assertion(const char *file, const char *function,
                                int line, const char *expression);
void dovi_bridge_dump_stack(void);

/* Initialize control runtime services in user space */
int dovi_bridge_init_services(void);

#ifdef __cplusplus
}
#endif

#endif /* DOVI_BRIDGE_H */
