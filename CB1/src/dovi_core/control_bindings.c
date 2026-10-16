#include "control_bindings.h"
#include "control_metadata.h"
#include "control_getters.h"
#include "control_infoframe.h"
#include "trim_reduction.h"

static void get_config(void *config, void *handle)
{
    (void)control_get_dm_cfg(config, handle);
}

static void get_tmm(unsigned short *min_pq, unsigned int *min_linear,
                    unsigned short *max_pq, unsigned int *max_linear,
                    void *handle)
{
    (void)control_get_stb_tmm(min_pq, min_linear, max_pq, max_linear, handle);
}

static void get_cc(unsigned short *min_pq, unsigned int *min_linear,
                   unsigned short *max_pq, unsigned int *max_linear,
                   void *handle)
{
    (void)control_get_stb_cc(min_pq, min_linear, max_pq, max_linear, handle);
}

static const struct control_hdmi_ops getter_ops = {get_config, get_tmm, get_cc};

void update_dm_l0_for_hdmi(void *context, void *metadata)
{
    update_dm_l0_for_hdmi_with_ops(context, metadata, &getter_ops);
}

void update_dm_ext_for_hdmi(void *context, void *metadata)
{
    update_dm_ext_for_hdmi_with_ops(context, metadata, &getter_ops);
}

void update_md_for_hdmi_bound(void *context, void *metadata,
                             control_reduce_trims_callback reduce_trims)
{
    const struct control_metadata_ops ops = {
        update_dm_l0_for_hdmi, update_dm_ext_for_hdmi, reduce_trims
    };
    update_md_for_hdmi_with_ops(context, metadata, &ops);
}

static void reduce_metadata(void *target_pointer_field, void *metadata)
{
    (void)reduce_trims(target_pointer_field, metadata);
}

void update_md_for_hdmi(void *context, void *metadata)
{
    update_md_for_hdmi_bound(context, metadata, reduce_metadata);
}
