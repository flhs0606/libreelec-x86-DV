/* SPDX-License-Identifier: GPL-2.0-only */
#include "dovi_bridge.h"
#include "control_runtime.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int dovi_bridge_cpu_version(int level)
{
    (void)level;
    /* Return 0x2b (flagship S922X-J / T4c) to preserve COMP_REG, DM_REG1 and LUT1 */
    return 0x2b;
}

int dovi_bridge_control_log(const char *format, unsigned int length)
{
    (void)length;
    if (!format) return 0;
    /* Strip kernel log level prefix if present (e.g. "\0016") */
    if (format[0] == '\001' && format[1] != '\0')
        format += 2;
    return fprintf(stderr, "[dovi_core] %s", format);
}

int dovi_bridge_memory_log(const char *format, const char *file,
                           const char *function, unsigned int line,
                           const char *expression)
{
    return fprintf(stderr, "[dovi_core:mem] %s (%s:%u %s: %s)\n",
                   format ? format : "", file ? file : "", line,
                   function ? function : "", expression ? expression : "");
}

void dovi_bridge_trim_assertion(const char *file, const char *function,
                                int line, const char *expression)
{
    fprintf(stderr, "[dovi_core:trim_assert] %s:%d %s: %s\n",
            file ? file : "", line, function ? function : "", expression ? expression : "");
}

void dovi_bridge_dump_stack(void)
{
    /* No-op in user space */
}

int dovi_bridge_init_services(void)
{
    const struct control_runtime_services services = {
        .cpu_version = dovi_bridge_cpu_version,
        .logger = dovi_bridge_control_log,
        .memory = {
            .logger = dovi_bridge_memory_log,
            .dump_stack = dovi_bridge_dump_stack
        },
        .trim = {
            .assertion = dovi_bridge_trim_assertion,
            .dump_stack = dovi_bridge_dump_stack
        }
    };
    return control_runtime_init(&services);
}
