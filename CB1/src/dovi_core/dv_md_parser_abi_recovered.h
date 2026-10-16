/* Parser configuration, event and context layouts shared by the core stages. */
#ifndef AM6B_DV_MD_PARSER_ABI_RECOVERED_H
#define AM6B_DV_MD_PARSER_ABI_RECOVERED_H

#include "dovi_abi_recovered.h"

/* Named typedefs for the parser API enumerations. */
typedef enum {
    RPU_BASE_PROFILE = 0,
    RPU_MAIN_PROFILE = 1,
    RPU_HIGH_PROFILE = 2,
    RPU_ETSI2_PROFILE = 3,
    RPU_ETSI3_PROFILE = 4,
    RPU_ETSI4_PROFILE = 5,
} rpu_profile_e;

typedef enum {
    RPU_LEVEL_1 = 0,
    RPU_LEVEL_2 = 1,
    RPU_LEVEL_3 = 2,
} rpu_level_e;

typedef enum _dv_md_parser_evt_e_ {
    MD_PARSER_SAMPLE_AVAIL_EVT = 0,
    MD_PARSER_ERROR_EVT = 1,
    MD_PARSER_EVT_MAX = 2,
} dv_md_parser_evt_t;

typedef enum _dv_md_parser_rc_e_ {
    DV_MD_PARSER_OK = 0,
    DV_MD_PARSER_UNINITIALIZE = 1,
    DV_MD_PARSER_INVALID_CONTEXT_HANDLE = 2,
    DV_MD_PARSER_OUT_OF_MEMORY = 3,
    DV_MD_PARSER_INVALID_BISTREAM_DATA = 4,
    DV_MD_PARSER_NOT_SUPPORTED_ERR = 5,
    DV_MD_PARSER_INVALID_LEVEL = 6,
    DV_MD_PARSER_INVALID_PROFILE = 7,
    DV_MD_PARSER_INVALID_SPATIAL_FILTER = 8,
    DV_MD_PARSER_INVALID_ARGUMENT = 9,
    DV_MD_PARSER_INTERNAL_ERR = 10,
    DV_MD_PARSER_VERSION_ERR = 11,
    DV_MD_PARSER_UNKNOWN_ERR = 12,
} dv_md_parser_rc_t;

typedef struct rpu_init_flags_s {
    rpu_profile_e profile;
    rpu_level_e level;
    int disable_crc_check;
    int payload_only;
    int input_is_RBSP;
    int disable_MEL_detection;
    int allow_changing_sequence_header;
} rpu_init_flags_t;

typedef struct _timestamp_s_ {
    dovi_u64 time_val;
    dovi_u32 time_scale;
} timestamp_t;

struct rpu_data_header_s;
struct vdr_rpu_data_payload_s;
struct vdr_dm_data_payload_s;
struct rpu_decoder_ctx_s;

typedef int (*msg_log_func_t)(const char *format, ...);
typedef void (*md_parser_evt_cb_func_t)(
    void *context, dv_md_parser_evt_t event, timestamp_t *timestamp,
    struct rpu_data_header_s *rpu_header,
    struct vdr_rpu_data_payload_s *rpu_payload, int rpu_payload_size,
    struct vdr_dm_data_payload_s *dm_payload, int dm_payload_size,
    int status);

typedef struct _dv_md_parser_conf_s_ {
    md_parser_evt_cb_func_t pf_md_parser_cb;
    void *p_md_parser_cb_ctx;
    rpu_init_flags_t flags;
    msg_log_func_t pf_msg_log;
} dv_md_parser_conf_t;

typedef struct _dv_md_parser_ctx_s_ {
    dovi_u8 f_init;
    md_parser_evt_cb_func_t pf_md_parser_cb;
    void *p_md_parser_cb_ctx;
    dv_md_parser_rc_t last_rc;
    rpu_init_flags_t flags;
    struct rpu_decoder_ctx_s *p_rpu_dcd_ctx;
    timestamp_t pts;
    timestamp_t curr_pts;
    msg_log_func_t pf_msg_log;
    dovi_u64 ui64_sample;
} dv_md_parser_ctx_t;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(rpu_profile_e) == 4, "rpu_profile_e size");
_Static_assert(sizeof(rpu_level_e) == 4, "rpu_level_e size");
_Static_assert(sizeof(dv_md_parser_evt_t) == 4, "dv_md_parser_evt_t size");
_Static_assert(sizeof(dv_md_parser_rc_t) == 4, "dv_md_parser_rc_t size");
_Static_assert(sizeof(rpu_init_flags_t) == 28, "rpu_init_flags_t size");
_Static_assert(sizeof(timestamp_t) == 16, "timestamp_t size");
_Static_assert(sizeof(dv_md_parser_conf_t) == 56, "dv_md_parser_conf_t size");
_Static_assert(sizeof(dv_md_parser_ctx_t) == 112, "dv_md_parser_ctx_t size");
_Static_assert(offsetof(rpu_init_flags_t, profile) == 0, "flags.profile offset");
_Static_assert(offsetof(rpu_init_flags_t, level) == 4, "flags.level offset");
_Static_assert(offsetof(rpu_init_flags_t, disable_crc_check) == 8, "flags.crc offset");
_Static_assert(offsetof(rpu_init_flags_t, payload_only) == 12, "flags.payload offset");
_Static_assert(offsetof(rpu_init_flags_t, input_is_RBSP) == 16, "flags.rbsp offset");
_Static_assert(offsetof(rpu_init_flags_t, disable_MEL_detection) == 20, "flags.mel offset");
_Static_assert(offsetof(rpu_init_flags_t, allow_changing_sequence_header) == 24,
               "flags.sequence offset");
_Static_assert(offsetof(timestamp_t, time_val) == 0, "timestamp value offset");
_Static_assert(offsetof(timestamp_t, time_scale) == 8, "timestamp scale offset");
_Static_assert(offsetof(dv_md_parser_conf_t, pf_md_parser_cb) == 0, "conf.cb offset");
_Static_assert(offsetof(dv_md_parser_conf_t, p_md_parser_cb_ctx) == 8, "conf.ctx offset");
_Static_assert(offsetof(dv_md_parser_conf_t, flags) == 16, "conf.flags offset");
_Static_assert(offsetof(dv_md_parser_conf_t, pf_msg_log) == 48, "conf.log offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, f_init) == 0, "ctx.init offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, pf_md_parser_cb) == 8, "ctx.cb offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, p_md_parser_cb_ctx) == 16, "ctx.cb_ctx offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, last_rc) == 24, "ctx.rc offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, flags) == 28, "ctx.flags offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, p_rpu_dcd_ctx) == 56, "ctx.decoder offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, pts) == 64, "ctx.pts offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, curr_pts) == 80, "ctx.curr_pts offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, pf_msg_log) == 96, "ctx.log offset");
_Static_assert(offsetof(dv_md_parser_ctx_t, ui64_sample) == 104, "ctx.sample offset");
#endif

#endif /* AM6B_DV_MD_PARSER_ABI_RECOVERED_H */
