#ifndef CONTROL_INIT_H
#define CONTROL_INIT_H

#include "control_abi_recovered.h"

/* The arena contains internal pointers. It must be 8-byte aligned and
 * cover 20040 bytes. init_cp does not clear it or initialize mmg.reqNum;
 * callers supply a valid memory manager before initialization. Memory ops are
 * required whenever reqNum > 31 can be reached. Logger/dump callbacks may
 * redirect h_dm/h_mds_ext to valid objects; init_cp reloads both fields. */
#define CONTROL_ARENA_BYTES 20040u
typedef int (*control_memory_callback)(void *parameter, void *buffer);
struct control_memory_ops {
    int (*logger)(const char *format, const char *file, const char *function,
                  unsigned int line, const char *expression);
    void (*dump_stack)(void);
};

void *control_init_memory(void *memory);
unsigned long long init_cp_mmg(void);
int control_register_memory(unsigned int size, unsigned int align, int in_device,
                            void *parameter, control_memory_callback callback,
                            void *memory, const struct control_memory_ops *ops);
int control_assign_memory(void *pointer_field, void *buffer);
void *control_init_kernel(const void *config, void *kernel,
                          const struct control_memory_ops *ops);
void *control_init_dm(const void *config, void *dm);
int init_cp(void *arena, const struct control_memory_ops *ops);
int destroy_cp(void *arena);
void set_dovi2hdr10_mapping(void *context);
void set_support_el(void *context, int enabled);

/* The cli_param buffer is supplied explicitly by the outer binding. */
void populate_graphics_param_body(void *graphics, const void *cli);

#endif
