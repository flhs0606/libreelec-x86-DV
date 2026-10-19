#include "control_runtime.h"
#include "register_commit.h"
#include "dm_commit_bindings.h"

/* Each object starts at its own eight-byte boundary. No large temporary
 * context or host output is placed on the kernel stack. */
union runtime_arena {
    unsigned long long alignment;
    unsigned char bytes[CONTROL_ARENA_BYTES];
};
union runtime_metadata {
    unsigned long long alignment;
    unsigned char bytes[CONTROL_METADATA_BYTES];
};
union runtime_composition {
    unsigned long long alignment;
    unsigned char bytes[1800];
};
union runtime_register1 {
    unsigned long long alignment;
    unsigned char bytes[900];
};
union runtime_register2 {
    unsigned long long alignment;
    unsigned char bytes[96];
};
union runtime_register3 {
    unsigned long long alignment;
    unsigned char bytes[1024];
};
static union runtime_arena arena;
static union runtime_metadata metadata1, metadata2;
static union runtime_composition composition;
static union runtime_register1 register1;
static union runtime_register2 register2;
static union runtime_register3 register3;
static unsigned long long cli[14];
static struct control_path_storage storage;
static struct control_runtime_services services;
static struct dm_commit_bindings dm_bindings;
static unsigned char configured;

static int runtime_commit_metadata(void *extensions, void *dm)
{
    return commit_dm_metadata(extensions, dm, &dm_bindings);
}
static const struct register_commit_ops commit_ops = {
    0, runtime_commit_metadata, 0
};

int control_runtime_init(const struct control_runtime_services *new_services)
{
    if (new_services == 0 || new_services->cpu_version == 0 ||
        new_services->logger == 0 || new_services->memory.logger == 0 ||
        new_services->memory.dump_stack == 0 ||
        new_services->trim.assertion == 0 || new_services->trim.dump_stack == 0)
        return -1;
    services = *new_services;
    dm_bindings.trim = services.trim;
    /* NULL tone builder selects the implementation in tone_curve.c. */
    storage.arena = arena.bytes;
    storage.cli = cli;
    storage.metadata1 = metadata1.bytes;
    storage.metadata2 = metadata2.bytes;
    storage.composition = composition.bytes;
    storage.register1 = register1.bytes;
    storage.register2 = register2.bytes;
    storage.register3 = register3.bytes;
    configured = 1;
    return 0;
}

int control_runtime_process(int input_format, int output_format,
    const char *input_composition, int composition_size, const char *metadata,
    int metadata_size, int priority, int bit_depth, int chroma_format,
    int yuv_range, int graphics_min, int graphics_max, int target_min,
    int target_max, int no_el, const void *hdr10, void *output)
{
    struct control_path_ops ops;
    if (configured == 0) return -1;
    ops.commit = commit_reg_with_ops;
    ops.commit_ops = &commit_ops;
    ops.cpu_version = services.cpu_version;
    ops.logger = services.logger;
    ops.memory = &services.memory;
    ops.reduce_trims = 0;
    return control_path_with_ops(&storage, &ops, input_format, output_format,
        input_composition, composition_size, metadata, metadata_size, priority,
        bit_depth, chroma_format, yuv_range, graphics_min, graphics_max,
        target_min, target_max, no_el, hdr10, output);
}
