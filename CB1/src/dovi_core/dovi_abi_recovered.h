/* Core decoder, payload and composer layouts. Keep field widths and offsets stable. */
#ifndef AM6B_DOVI_ABI_RECOVERED_H
#define AM6B_DOVI_ABI_RECOVERED_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/stddef.h>
typedef u8 dovi_u8;
typedef u16 dovi_u16;
typedef u32 dovi_u32;
typedef u64 dovi_u64;
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef uint8_t dovi_u8;
typedef uint16_t dovi_u16;
typedef uint32_t dovi_u32;
typedef uint64_t dovi_u64;
#endif

enum signal_format_e {
    FORMAT_DOVI = 0,
    FORMAT_HDR10 = 1,
    FORMAT_SDR = 2,
    FORMAT_DOVI_LL = 3,
    FORMAT_SDR_2020 = 4,
};

enum priority_mode_e {
    VIDEO_PRIORITY = 0,
    GRAPHIC_PRIORITY = 1,
    VIDEO_PRIORITY_DELAYED = 2,
};

enum graphics_format_e {
    GF_SDR_YUV = 0,
    GF_SDR_RGB = 1,
    GF_HDR_YUV = 2,
    GF_HDR_RGB = 3,
};

struct ext_level_1_s {
    dovi_u8 min_PQ_hi;
    dovi_u8 min_PQ_lo;
    dovi_u8 max_PQ_hi;
    dovi_u8 max_PQ_lo;
    dovi_u8 avg_PQ_hi;
    dovi_u8 avg_PQ_lo;
};

struct ext_level_2_s {
    dovi_u8 target_max_PQ_hi;
    dovi_u8 target_max_PQ_lo;
    dovi_u8 trim_slope_hi;
    dovi_u8 trim_slope_lo;
    dovi_u8 trim_offset_hi;
    dovi_u8 trim_offset_lo;
    dovi_u8 trim_power_hi;
    dovi_u8 trim_power_lo;
    dovi_u8 trim_chroma_weight_hi;
    dovi_u8 trim_chroma_weight_lo;
    dovi_u8 trim_saturation_gain_hi;
    dovi_u8 trim_saturation_gain_lo;
    dovi_u8 ms_weight_hi;
    dovi_u8 ms_weight_lo;
};

struct ext_level_4_s {
    dovi_u8 anchor_PQ_hi;
    dovi_u8 anchor_PQ_lo;
    dovi_u8 anchor_power_hi;
    dovi_u8 anchor_power_lo;
};

struct ext_level_5_s {
    dovi_u8 active_area_left_offset_hi;
    dovi_u8 active_area_left_offset_lo;
    dovi_u8 active_area_right_offset_hi;
    dovi_u8 active_area_right_offset_lo;
    dovi_u8 active_area_top_offset_hi;
    dovi_u8 active_area_top_offset_lo;
    dovi_u8 active_area_bottom_offset_hi;
    dovi_u8 active_area_bottom_offset_lo;
};

struct ext_level_6_s {
    dovi_u8 max_display_mastering_luminance_hi;
    dovi_u8 max_display_mastering_luminance_lo;
    dovi_u8 min_display_mastering_luminance_hi;
    dovi_u8 min_display_mastering_luminance_lo;
    dovi_u8 max_content_light_level_hi;
    dovi_u8 max_content_light_level_lo;
    dovi_u8 max_frame_average_light_level_hi;
    dovi_u8 max_frame_average_light_level_lo;
};

struct ext_level_255_s {
    dovi_u8 dm_run_mode;
    dovi_u8 dm_run_version;
    dovi_u8 dm_debug0;
    dovi_u8 dm_debug1;
    dovi_u8 dm_debug2;
    dovi_u8 dm_debug3;
};

struct ext_md_s {
    dovi_u32 available_level_mask;
    struct ext_level_1_s level_1;
    struct ext_level_2_s level_2;
    struct ext_level_4_s level_4;
    struct ext_level_5_s level_5;
    struct ext_level_6_s level_6;
    struct ext_level_255_s level_255;
};

struct composer_register_ipcore_s {
    dovi_u32 Composer_Mode;
    dovi_u32 VDR_Resolution;
    dovi_u32 Bit_Depth;
    dovi_u32 Coefficient_Log2_Denominator;
    dovi_u32 BL_Num_Pivots_Y;
    dovi_u32 BL_Pivot[5];
    dovi_u32 BL_Order;
    dovi_u32 BL_Coefficient_Y[8][3];
    dovi_u32 EL_NLQ_Offset_Y;
    dovi_u32 EL_Coefficient_Y[3];
    dovi_u32 Mapping_IDC_U;
    dovi_u32 BL_Num_Pivots_U;
    dovi_u32 BL_Pivot_U[3];
    dovi_u32 BL_Order_U;
    dovi_u32 BL_Coefficient_U[4][3];
    dovi_u32 MMR_Coefficient_U[22][2];
    dovi_u32 MMR_Order_U;
    dovi_u32 EL_NLQ_Offset_U;
    dovi_u32 EL_Coefficient_U[3];
    dovi_u32 Mapping_IDC_V;
    dovi_u32 BL_Num_Pivots_V;
    dovi_u32 BL_Pivot_V[3];
    dovi_u32 BL_Order_V;
    dovi_u32 BL_Coefficient_V[4][3];
    dovi_u32 MMR_Coefficient_V[22][2];
    dovi_u32 MMR_Order_V;
    dovi_u32 EL_NLQ_Offset_V;
    dovi_u32 EL_Coefficient_V[3];
};

struct dm_register_ipcore_1_s {
    dovi_u32 SRange;
    dovi_u32 Srange_Inverse;
    dovi_u32 Frame_Format_1;
    dovi_u32 Frame_Format_2;
    dovi_u32 Frame_Pixel_Def;
    dovi_u32 Y2RGB_Coefficient_1;
    dovi_u32 Y2RGB_Coefficient_2;
    dovi_u32 Y2RGB_Coefficient_3;
    dovi_u32 Y2RGB_Coefficient_4;
    dovi_u32 Y2RGB_Coefficient_5;
    dovi_u32 Y2RGB_Offset_1;
    dovi_u32 Y2RGB_Offset_2;
    dovi_u32 Y2RGB_Offset_3;
    dovi_u32 EOTF;
    dovi_u32 A2B_Coefficient_1;
    dovi_u32 A2B_Coefficient_2;
    dovi_u32 A2B_Coefficient_3;
    dovi_u32 A2B_Coefficient_4;
    dovi_u32 A2B_Coefficient_5;
    dovi_u32 C2D_Coefficient_1;
    dovi_u32 C2D_Coefficient_2;
    dovi_u32 C2D_Coefficient_3;
    dovi_u32 C2D_Coefficient_4;
    dovi_u32 C2D_Coefficient_5;
    dovi_u32 C2D_Offset;
    dovi_u32 Active_area_left_top;
    dovi_u32 Active_area_bottom_right;
};

struct dm_register_ipcore_2_s {
    dovi_u32 SRange;
    dovi_u32 Srange_Inverse;
    dovi_u32 Y2RGB_Coefficient_1;
    dovi_u32 Y2RGB_Coefficient_2;
    dovi_u32 Y2RGB_Coefficient_3;
    dovi_u32 Y2RGB_Coefficient_4;
    dovi_u32 Y2RGB_Coefficient_5;
    dovi_u32 Y2RGB_Offset_1;
    dovi_u32 Y2RGB_Offset_2;
    dovi_u32 Y2RGB_Offset_3;
    dovi_u32 Frame_Format;
    dovi_u32 EOTF;
    dovi_u32 A2B_Coefficient_1;
    dovi_u32 A2B_Coefficient_2;
    dovi_u32 A2B_Coefficient_3;
    dovi_u32 A2B_Coefficient_4;
    dovi_u32 A2B_Coefficient_5;
    dovi_u32 C2D_Coefficient_1;
    dovi_u32 C2D_Coefficient_2;
    dovi_u32 C2D_Coefficient_3;
    dovi_u32 C2D_Coefficient_4;
    dovi_u32 C2D_Coefficient_5;
    dovi_u32 C2D_Offset;
    dovi_u32 VDR_Resolution;
};

struct dm_register_ipcore_3_s {
    dovi_u32 D2C_coefficient_1;
    dovi_u32 D2C_coefficient_2;
    dovi_u32 D2C_coefficient_3;
    dovi_u32 D2C_coefficient_4;
    dovi_u32 D2C_coefficient_5;
    dovi_u32 B2A_Coefficient_1;
    dovi_u32 B2A_Coefficient_2;
    dovi_u32 B2A_Coefficient_3;
    dovi_u32 B2A_Coefficient_4;
    dovi_u32 B2A_Coefficient_5;
    dovi_u32 Eotf_param_1;
    dovi_u32 Eotf_param_2;
    dovi_u32 IPT_Scale;
    dovi_u32 IPT_Offset_1;
    dovi_u32 IPT_Offset_2;
    dovi_u32 IPT_Offset_3;
    dovi_u32 Output_range_1;
    dovi_u32 Output_range_2;
    dovi_u32 RGB2YUV_coefficient_register1;
    dovi_u32 RGB2YUV_coefficient_register2;
    dovi_u32 RGB2YUV_coefficient_register3;
    dovi_u32 RGB2YUV_coefficient_register4;
    dovi_u32 RGB2YUV_coefficient_register5;
    dovi_u32 RGB2YUV_offset_0;
    dovi_u32 RGB2YUV_offset_1;
    dovi_u32 RGB2YUV_offset_2;
};

struct dm_lut_ipcore_s {
    dovi_u32 TmLutI[256];
    dovi_u32 TmLutS[256];
    dovi_u32 SmLutI[256];
    dovi_u32 SmLutS[256];
    dovi_u32 G2L[256];
};

struct md_reister_ipcore_3_s {
    dovi_u32 raw_metadata[512];
    dovi_u32 size;
};

struct hdr_10_infoframe_s {
    dovi_u8 infoframe_type_code;
    dovi_u8 infoframe_version_number;
    dovi_u8 length_of_info_frame;
    dovi_u8 data_byte_1;
    dovi_u8 data_byte_2;
    dovi_u8 display_primaries_x_0_LSB;
    dovi_u8 display_primaries_x_0_MSB;
    dovi_u8 display_primaries_y_0_LSB;
    dovi_u8 display_primaries_y_0_MSB;
    dovi_u8 display_primaries_x_1_LSB;
    dovi_u8 display_primaries_x_1_MSB;
    dovi_u8 display_primaries_y_1_LSB;
    dovi_u8 display_primaries_y_1_MSB;
    dovi_u8 display_primaries_x_2_LSB;
    dovi_u8 display_primaries_x_2_MSB;
    dovi_u8 display_primaries_y_2_LSB;
    dovi_u8 display_primaries_y_2_MSB;
    dovi_u8 white_point_x_LSB;
    dovi_u8 white_point_x_MSB;
    dovi_u8 white_point_y_LSB;
    dovi_u8 white_point_y_MSB;
    dovi_u8 max_display_mastering_luminance_LSB;
    dovi_u8 max_display_mastering_luminance_MSB;
    dovi_u8 min_display_mastering_luminance_LSB;
    dovi_u8 min_display_mastering_luminance_MSB;
    dovi_u8 max_content_light_level_LSB;
    dovi_u8 max_content_light_level_MSB;
    dovi_u8 max_frame_average_light_level_LSB;
    dovi_u8 max_frame_average_light_level_MSB;
};

struct hdr10_param_s {
    dovi_u32 min_display_mastering_luminance;
    dovi_u32 max_display_mastering_luminance;
    dovi_u16 Rx;
    dovi_u16 Ry;
    dovi_u16 Gx;
    dovi_u16 Gy;
    dovi_u16 Bx;
    dovi_u16 By;
    dovi_u16 Wx;
    dovi_u16 Wy;
    dovi_u16 max_content_light_level;
    dovi_u16 max_pic_average_light_level;
};

struct dovi_setting_s {
    struct composer_register_ipcore_s comp_reg;
    struct dm_register_ipcore_1_s dm_reg1;
    struct dm_register_ipcore_2_s dm_reg2;
    struct dm_register_ipcore_3_s dm_reg3;
    struct dm_lut_ipcore_s dm_lut1;
    struct dm_lut_ipcore_s dm_lut2;
    struct md_reister_ipcore_3_s md_reg3;
    struct hdr_10_infoframe_s hdr_info;
    enum signal_format_e src_format;
    enum signal_format_e dst_format;
    bool el_flag;
    bool el_halfsize_flag;
    dovi_u32 video_width;
    dovi_u32 video_height;
    enum graphics_format_e g_format;
    dovi_u32 g_bitdepth;
    dovi_u32 dovi2hdr10_nomapping;
    dovi_u32 use_ll_flag;
    dovi_u32 ll_rgb_desired;
    dovi_u32 diagnostic_enable;
    dovi_u32 diagnostic_mux_select;
    dovi_u32 dovi_ll_enable;
    dovi_u32 vout_width;
    dovi_u32 vout_height;
    dovi_u8 vsvdb_tbl[32];
    struct ext_md_s ext_md;
};

struct dolby_vision_func_s {
    const char *version_info;
    void *(*metadata_parser_init)(int flag);
    int (*metadata_parser_reset)(int flag);
    int (*metadata_parser_process)(char *src_rpu, int rpu_len,
                                   char *dst_comp, int *comp_len,
                                   char *dst_md, int *md_len, bool src_eos);
    void (*metadata_parser_release)(void);
    int (*control_path)(enum signal_format_e in_format,
                        enum signal_format_e out_format,
                        char *in_comp, int in_comp_size,
                        char *in_md, int in_md_size,
                        enum priority_mode_e set_priority,
                        int set_bit_depth, int set_chroma_format,
                        int set_yuv_range, int set_graphic_min_lum,
                        int set_graphic_max_lum, int set_target_min_lum,
                        int set_target_max_lum, int set_no_el,
                        struct hdr10_param_s *p_hdr10_param,
                        struct dovi_setting_s *output);
    int (*reserved_func)(void);
};

extern const struct dolby_vision_func_s DV_func;
extern char dv_sign[512];

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(struct ext_level_1_s) == 6, "ext_level_1_s size");
_Static_assert(sizeof(struct ext_level_2_s) == 14, "ext_level_2_s size");
_Static_assert(sizeof(struct ext_level_4_s) == 4, "ext_level_4_s size");
_Static_assert(sizeof(struct ext_level_5_s) == 8, "ext_level_5_s size");
_Static_assert(sizeof(struct ext_level_6_s) == 8, "ext_level_6_s size");
_Static_assert(sizeof(struct ext_level_255_s) == 6, "ext_level_255_s size");
_Static_assert(sizeof(struct ext_md_s) == 52, "ext_md_s size");
_Static_assert(sizeof(struct composer_register_ipcore_s) == 692, "composer_register_ipcore_s size");
_Static_assert(sizeof(struct dm_register_ipcore_1_s) == 108, "dm_register_ipcore_1_s size");
_Static_assert(sizeof(struct dm_register_ipcore_2_s) == 96, "dm_register_ipcore_2_s size");
_Static_assert(sizeof(struct dm_register_ipcore_3_s) == 104, "dm_register_ipcore_3_s size");
_Static_assert(sizeof(struct dm_lut_ipcore_s) == 5120, "dm_lut_ipcore_s size");
_Static_assert(sizeof(struct md_reister_ipcore_3_s) == 2052, "md_reister_ipcore_3_s size");
_Static_assert(sizeof(struct hdr_10_infoframe_s) == 29, "hdr_10_infoframe_s size");
_Static_assert(sizeof(struct hdr10_param_s) == 28, "hdr10_param_s size");
_Static_assert(sizeof(struct dovi_setting_s) == 13468, "dovi_setting_s size");
_Static_assert(offsetof(struct ext_level_1_s, min_PQ_hi) == 0, "ext_level_1_s.min_PQ_hi offset");
_Static_assert(offsetof(struct ext_level_1_s, min_PQ_lo) == 1, "ext_level_1_s.min_PQ_lo offset");
_Static_assert(offsetof(struct ext_level_1_s, max_PQ_hi) == 2, "ext_level_1_s.max_PQ_hi offset");
_Static_assert(offsetof(struct ext_level_1_s, max_PQ_lo) == 3, "ext_level_1_s.max_PQ_lo offset");
_Static_assert(offsetof(struct ext_level_1_s, avg_PQ_hi) == 4, "ext_level_1_s.avg_PQ_hi offset");
_Static_assert(offsetof(struct ext_level_1_s, avg_PQ_lo) == 5, "ext_level_1_s.avg_PQ_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, target_max_PQ_hi) == 0, "ext_level_2_s.target_max_PQ_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, target_max_PQ_lo) == 1, "ext_level_2_s.target_max_PQ_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_slope_hi) == 2, "ext_level_2_s.trim_slope_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_slope_lo) == 3, "ext_level_2_s.trim_slope_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_offset_hi) == 4, "ext_level_2_s.trim_offset_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_offset_lo) == 5, "ext_level_2_s.trim_offset_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_power_hi) == 6, "ext_level_2_s.trim_power_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_power_lo) == 7, "ext_level_2_s.trim_power_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_chroma_weight_hi) == 8, "ext_level_2_s.trim_chroma_weight_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_chroma_weight_lo) == 9, "ext_level_2_s.trim_chroma_weight_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_saturation_gain_hi) == 10, "ext_level_2_s.trim_saturation_gain_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, trim_saturation_gain_lo) == 11, "ext_level_2_s.trim_saturation_gain_lo offset");
_Static_assert(offsetof(struct ext_level_2_s, ms_weight_hi) == 12, "ext_level_2_s.ms_weight_hi offset");
_Static_assert(offsetof(struct ext_level_2_s, ms_weight_lo) == 13, "ext_level_2_s.ms_weight_lo offset");
_Static_assert(offsetof(struct ext_level_4_s, anchor_PQ_hi) == 0, "ext_level_4_s.anchor_PQ_hi offset");
_Static_assert(offsetof(struct ext_level_4_s, anchor_PQ_lo) == 1, "ext_level_4_s.anchor_PQ_lo offset");
_Static_assert(offsetof(struct ext_level_4_s, anchor_power_hi) == 2, "ext_level_4_s.anchor_power_hi offset");
_Static_assert(offsetof(struct ext_level_4_s, anchor_power_lo) == 3, "ext_level_4_s.anchor_power_lo offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_left_offset_hi) == 0, "ext_level_5_s.active_area_left_offset_hi offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_left_offset_lo) == 1, "ext_level_5_s.active_area_left_offset_lo offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_right_offset_hi) == 2, "ext_level_5_s.active_area_right_offset_hi offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_right_offset_lo) == 3, "ext_level_5_s.active_area_right_offset_lo offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_top_offset_hi) == 4, "ext_level_5_s.active_area_top_offset_hi offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_top_offset_lo) == 5, "ext_level_5_s.active_area_top_offset_lo offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_bottom_offset_hi) == 6, "ext_level_5_s.active_area_bottom_offset_hi offset");
_Static_assert(offsetof(struct ext_level_5_s, active_area_bottom_offset_lo) == 7, "ext_level_5_s.active_area_bottom_offset_lo offset");
_Static_assert(offsetof(struct ext_level_6_s, max_display_mastering_luminance_hi) == 0, "ext_level_6_s.max_display_mastering_luminance_hi offset");
_Static_assert(offsetof(struct ext_level_6_s, max_display_mastering_luminance_lo) == 1, "ext_level_6_s.max_display_mastering_luminance_lo offset");
_Static_assert(offsetof(struct ext_level_6_s, min_display_mastering_luminance_hi) == 2, "ext_level_6_s.min_display_mastering_luminance_hi offset");
_Static_assert(offsetof(struct ext_level_6_s, min_display_mastering_luminance_lo) == 3, "ext_level_6_s.min_display_mastering_luminance_lo offset");
_Static_assert(offsetof(struct ext_level_6_s, max_content_light_level_hi) == 4, "ext_level_6_s.max_content_light_level_hi offset");
_Static_assert(offsetof(struct ext_level_6_s, max_content_light_level_lo) == 5, "ext_level_6_s.max_content_light_level_lo offset");
_Static_assert(offsetof(struct ext_level_6_s, max_frame_average_light_level_hi) == 6, "ext_level_6_s.max_frame_average_light_level_hi offset");
_Static_assert(offsetof(struct ext_level_6_s, max_frame_average_light_level_lo) == 7, "ext_level_6_s.max_frame_average_light_level_lo offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_run_mode) == 0, "ext_level_255_s.dm_run_mode offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_run_version) == 1, "ext_level_255_s.dm_run_version offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_debug0) == 2, "ext_level_255_s.dm_debug0 offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_debug1) == 3, "ext_level_255_s.dm_debug1 offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_debug2) == 4, "ext_level_255_s.dm_debug2 offset");
_Static_assert(offsetof(struct ext_level_255_s, dm_debug3) == 5, "ext_level_255_s.dm_debug3 offset");
_Static_assert(offsetof(struct ext_md_s, available_level_mask) == 0, "ext_md_s.available_level_mask offset");
_Static_assert(offsetof(struct ext_md_s, level_1) == 4, "ext_md_s.level_1 offset");
_Static_assert(offsetof(struct ext_md_s, level_2) == 10, "ext_md_s.level_2 offset");
_Static_assert(offsetof(struct ext_md_s, level_4) == 24, "ext_md_s.level_4 offset");
_Static_assert(offsetof(struct ext_md_s, level_5) == 28, "ext_md_s.level_5 offset");
_Static_assert(offsetof(struct ext_md_s, level_6) == 36, "ext_md_s.level_6 offset");
_Static_assert(offsetof(struct ext_md_s, level_255) == 44, "ext_md_s.level_255 offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, Composer_Mode) == 0, "composer_register_ipcore_s.Composer_Mode offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, VDR_Resolution) == 4, "composer_register_ipcore_s.VDR_Resolution offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, Bit_Depth) == 8, "composer_register_ipcore_s.Bit_Depth offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, Coefficient_Log2_Denominator) == 12, "composer_register_ipcore_s.Coefficient_Log2_Denominator offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Num_Pivots_Y) == 16, "composer_register_ipcore_s.BL_Num_Pivots_Y offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Pivot) == 20, "composer_register_ipcore_s.BL_Pivot offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Order) == 40, "composer_register_ipcore_s.BL_Order offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Coefficient_Y) == 44, "composer_register_ipcore_s.BL_Coefficient_Y offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_NLQ_Offset_Y) == 140, "composer_register_ipcore_s.EL_NLQ_Offset_Y offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_Coefficient_Y) == 144, "composer_register_ipcore_s.EL_Coefficient_Y offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, Mapping_IDC_U) == 156, "composer_register_ipcore_s.Mapping_IDC_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Num_Pivots_U) == 160, "composer_register_ipcore_s.BL_Num_Pivots_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Pivot_U) == 164, "composer_register_ipcore_s.BL_Pivot_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Order_U) == 176, "composer_register_ipcore_s.BL_Order_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Coefficient_U) == 180, "composer_register_ipcore_s.BL_Coefficient_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, MMR_Coefficient_U) == 228, "composer_register_ipcore_s.MMR_Coefficient_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, MMR_Order_U) == 404, "composer_register_ipcore_s.MMR_Order_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_NLQ_Offset_U) == 408, "composer_register_ipcore_s.EL_NLQ_Offset_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_Coefficient_U) == 412, "composer_register_ipcore_s.EL_Coefficient_U offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, Mapping_IDC_V) == 424, "composer_register_ipcore_s.Mapping_IDC_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Num_Pivots_V) == 428, "composer_register_ipcore_s.BL_Num_Pivots_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Pivot_V) == 432, "composer_register_ipcore_s.BL_Pivot_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Order_V) == 444, "composer_register_ipcore_s.BL_Order_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, BL_Coefficient_V) == 448, "composer_register_ipcore_s.BL_Coefficient_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, MMR_Coefficient_V) == 496, "composer_register_ipcore_s.MMR_Coefficient_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, MMR_Order_V) == 672, "composer_register_ipcore_s.MMR_Order_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_NLQ_Offset_V) == 676, "composer_register_ipcore_s.EL_NLQ_Offset_V offset");
_Static_assert(offsetof(struct composer_register_ipcore_s, EL_Coefficient_V) == 680, "composer_register_ipcore_s.EL_Coefficient_V offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, SRange) == 0, "dm_register_ipcore_1_s.SRange offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Srange_Inverse) == 4, "dm_register_ipcore_1_s.Srange_Inverse offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Frame_Format_1) == 8, "dm_register_ipcore_1_s.Frame_Format_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Frame_Format_2) == 12, "dm_register_ipcore_1_s.Frame_Format_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Frame_Pixel_Def) == 16, "dm_register_ipcore_1_s.Frame_Pixel_Def offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Coefficient_1) == 20, "dm_register_ipcore_1_s.Y2RGB_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Coefficient_2) == 24, "dm_register_ipcore_1_s.Y2RGB_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Coefficient_3) == 28, "dm_register_ipcore_1_s.Y2RGB_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Coefficient_4) == 32, "dm_register_ipcore_1_s.Y2RGB_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Coefficient_5) == 36, "dm_register_ipcore_1_s.Y2RGB_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Offset_1) == 40, "dm_register_ipcore_1_s.Y2RGB_Offset_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Offset_2) == 44, "dm_register_ipcore_1_s.Y2RGB_Offset_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Y2RGB_Offset_3) == 48, "dm_register_ipcore_1_s.Y2RGB_Offset_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, EOTF) == 52, "dm_register_ipcore_1_s.EOTF offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, A2B_Coefficient_1) == 56, "dm_register_ipcore_1_s.A2B_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, A2B_Coefficient_2) == 60, "dm_register_ipcore_1_s.A2B_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, A2B_Coefficient_3) == 64, "dm_register_ipcore_1_s.A2B_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, A2B_Coefficient_4) == 68, "dm_register_ipcore_1_s.A2B_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, A2B_Coefficient_5) == 72, "dm_register_ipcore_1_s.A2B_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Coefficient_1) == 76, "dm_register_ipcore_1_s.C2D_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Coefficient_2) == 80, "dm_register_ipcore_1_s.C2D_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Coefficient_3) == 84, "dm_register_ipcore_1_s.C2D_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Coefficient_4) == 88, "dm_register_ipcore_1_s.C2D_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Coefficient_5) == 92, "dm_register_ipcore_1_s.C2D_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, C2D_Offset) == 96, "dm_register_ipcore_1_s.C2D_Offset offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Active_area_left_top) == 100, "dm_register_ipcore_1_s.Active_area_left_top offset");
_Static_assert(offsetof(struct dm_register_ipcore_1_s, Active_area_bottom_right) == 104, "dm_register_ipcore_1_s.Active_area_bottom_right offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, SRange) == 0, "dm_register_ipcore_2_s.SRange offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Srange_Inverse) == 4, "dm_register_ipcore_2_s.Srange_Inverse offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Coefficient_1) == 8, "dm_register_ipcore_2_s.Y2RGB_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Coefficient_2) == 12, "dm_register_ipcore_2_s.Y2RGB_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Coefficient_3) == 16, "dm_register_ipcore_2_s.Y2RGB_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Coefficient_4) == 20, "dm_register_ipcore_2_s.Y2RGB_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Coefficient_5) == 24, "dm_register_ipcore_2_s.Y2RGB_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Offset_1) == 28, "dm_register_ipcore_2_s.Y2RGB_Offset_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Offset_2) == 32, "dm_register_ipcore_2_s.Y2RGB_Offset_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Y2RGB_Offset_3) == 36, "dm_register_ipcore_2_s.Y2RGB_Offset_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, Frame_Format) == 40, "dm_register_ipcore_2_s.Frame_Format offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, EOTF) == 44, "dm_register_ipcore_2_s.EOTF offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, A2B_Coefficient_1) == 48, "dm_register_ipcore_2_s.A2B_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, A2B_Coefficient_2) == 52, "dm_register_ipcore_2_s.A2B_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, A2B_Coefficient_3) == 56, "dm_register_ipcore_2_s.A2B_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, A2B_Coefficient_4) == 60, "dm_register_ipcore_2_s.A2B_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, A2B_Coefficient_5) == 64, "dm_register_ipcore_2_s.A2B_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Coefficient_1) == 68, "dm_register_ipcore_2_s.C2D_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Coefficient_2) == 72, "dm_register_ipcore_2_s.C2D_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Coefficient_3) == 76, "dm_register_ipcore_2_s.C2D_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Coefficient_4) == 80, "dm_register_ipcore_2_s.C2D_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Coefficient_5) == 84, "dm_register_ipcore_2_s.C2D_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, C2D_Offset) == 88, "dm_register_ipcore_2_s.C2D_Offset offset");
_Static_assert(offsetof(struct dm_register_ipcore_2_s, VDR_Resolution) == 92, "dm_register_ipcore_2_s.VDR_Resolution offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, D2C_coefficient_1) == 0, "dm_register_ipcore_3_s.D2C_coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, D2C_coefficient_2) == 4, "dm_register_ipcore_3_s.D2C_coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, D2C_coefficient_3) == 8, "dm_register_ipcore_3_s.D2C_coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, D2C_coefficient_4) == 12, "dm_register_ipcore_3_s.D2C_coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, D2C_coefficient_5) == 16, "dm_register_ipcore_3_s.D2C_coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, B2A_Coefficient_1) == 20, "dm_register_ipcore_3_s.B2A_Coefficient_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, B2A_Coefficient_2) == 24, "dm_register_ipcore_3_s.B2A_Coefficient_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, B2A_Coefficient_3) == 28, "dm_register_ipcore_3_s.B2A_Coefficient_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, B2A_Coefficient_4) == 32, "dm_register_ipcore_3_s.B2A_Coefficient_4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, B2A_Coefficient_5) == 36, "dm_register_ipcore_3_s.B2A_Coefficient_5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, Eotf_param_1) == 40, "dm_register_ipcore_3_s.Eotf_param_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, Eotf_param_2) == 44, "dm_register_ipcore_3_s.Eotf_param_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, IPT_Scale) == 48, "dm_register_ipcore_3_s.IPT_Scale offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, IPT_Offset_1) == 52, "dm_register_ipcore_3_s.IPT_Offset_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, IPT_Offset_2) == 56, "dm_register_ipcore_3_s.IPT_Offset_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, IPT_Offset_3) == 60, "dm_register_ipcore_3_s.IPT_Offset_3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, Output_range_1) == 64, "dm_register_ipcore_3_s.Output_range_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, Output_range_2) == 68, "dm_register_ipcore_3_s.Output_range_2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_coefficient_register1) == 72, "dm_register_ipcore_3_s.RGB2YUV_coefficient_register1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_coefficient_register2) == 76, "dm_register_ipcore_3_s.RGB2YUV_coefficient_register2 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_coefficient_register3) == 80, "dm_register_ipcore_3_s.RGB2YUV_coefficient_register3 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_coefficient_register4) == 84, "dm_register_ipcore_3_s.RGB2YUV_coefficient_register4 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_coefficient_register5) == 88, "dm_register_ipcore_3_s.RGB2YUV_coefficient_register5 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_offset_0) == 92, "dm_register_ipcore_3_s.RGB2YUV_offset_0 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_offset_1) == 96, "dm_register_ipcore_3_s.RGB2YUV_offset_1 offset");
_Static_assert(offsetof(struct dm_register_ipcore_3_s, RGB2YUV_offset_2) == 100, "dm_register_ipcore_3_s.RGB2YUV_offset_2 offset");
_Static_assert(offsetof(struct dm_lut_ipcore_s, TmLutI) == 0, "dm_lut_ipcore_s.TmLutI offset");
_Static_assert(offsetof(struct dm_lut_ipcore_s, TmLutS) == 1024, "dm_lut_ipcore_s.TmLutS offset");
_Static_assert(offsetof(struct dm_lut_ipcore_s, SmLutI) == 2048, "dm_lut_ipcore_s.SmLutI offset");
_Static_assert(offsetof(struct dm_lut_ipcore_s, SmLutS) == 3072, "dm_lut_ipcore_s.SmLutS offset");
_Static_assert(offsetof(struct dm_lut_ipcore_s, G2L) == 4096, "dm_lut_ipcore_s.G2L offset");
_Static_assert(offsetof(struct md_reister_ipcore_3_s, raw_metadata) == 0, "md_reister_ipcore_3_s.raw_metadata offset");
_Static_assert(offsetof(struct md_reister_ipcore_3_s, size) == 2048, "md_reister_ipcore_3_s.size offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, infoframe_type_code) == 0, "hdr_10_infoframe_s.infoframe_type_code offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, infoframe_version_number) == 1, "hdr_10_infoframe_s.infoframe_version_number offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, length_of_info_frame) == 2, "hdr_10_infoframe_s.length_of_info_frame offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, data_byte_1) == 3, "hdr_10_infoframe_s.data_byte_1 offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, data_byte_2) == 4, "hdr_10_infoframe_s.data_byte_2 offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_0_LSB) == 5, "hdr_10_infoframe_s.display_primaries_x_0_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_0_MSB) == 6, "hdr_10_infoframe_s.display_primaries_x_0_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_0_LSB) == 7, "hdr_10_infoframe_s.display_primaries_y_0_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_0_MSB) == 8, "hdr_10_infoframe_s.display_primaries_y_0_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_1_LSB) == 9, "hdr_10_infoframe_s.display_primaries_x_1_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_1_MSB) == 10, "hdr_10_infoframe_s.display_primaries_x_1_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_1_LSB) == 11, "hdr_10_infoframe_s.display_primaries_y_1_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_1_MSB) == 12, "hdr_10_infoframe_s.display_primaries_y_1_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_2_LSB) == 13, "hdr_10_infoframe_s.display_primaries_x_2_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_x_2_MSB) == 14, "hdr_10_infoframe_s.display_primaries_x_2_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_2_LSB) == 15, "hdr_10_infoframe_s.display_primaries_y_2_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, display_primaries_y_2_MSB) == 16, "hdr_10_infoframe_s.display_primaries_y_2_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, white_point_x_LSB) == 17, "hdr_10_infoframe_s.white_point_x_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, white_point_x_MSB) == 18, "hdr_10_infoframe_s.white_point_x_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, white_point_y_LSB) == 19, "hdr_10_infoframe_s.white_point_y_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, white_point_y_MSB) == 20, "hdr_10_infoframe_s.white_point_y_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_display_mastering_luminance_LSB) == 21, "hdr_10_infoframe_s.max_display_mastering_luminance_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_display_mastering_luminance_MSB) == 22, "hdr_10_infoframe_s.max_display_mastering_luminance_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, min_display_mastering_luminance_LSB) == 23, "hdr_10_infoframe_s.min_display_mastering_luminance_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, min_display_mastering_luminance_MSB) == 24, "hdr_10_infoframe_s.min_display_mastering_luminance_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_content_light_level_LSB) == 25, "hdr_10_infoframe_s.max_content_light_level_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_content_light_level_MSB) == 26, "hdr_10_infoframe_s.max_content_light_level_MSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_frame_average_light_level_LSB) == 27, "hdr_10_infoframe_s.max_frame_average_light_level_LSB offset");
_Static_assert(offsetof(struct hdr_10_infoframe_s, max_frame_average_light_level_MSB) == 28, "hdr_10_infoframe_s.max_frame_average_light_level_MSB offset");
_Static_assert(offsetof(struct hdr10_param_s, min_display_mastering_luminance) == 0, "hdr10_param_s.min_display_mastering_luminance offset");
_Static_assert(offsetof(struct hdr10_param_s, max_display_mastering_luminance) == 4, "hdr10_param_s.max_display_mastering_luminance offset");
_Static_assert(offsetof(struct hdr10_param_s, Rx) == 8, "hdr10_param_s.Rx offset");
_Static_assert(offsetof(struct hdr10_param_s, Ry) == 10, "hdr10_param_s.Ry offset");
_Static_assert(offsetof(struct hdr10_param_s, Gx) == 12, "hdr10_param_s.Gx offset");
_Static_assert(offsetof(struct hdr10_param_s, Gy) == 14, "hdr10_param_s.Gy offset");
_Static_assert(offsetof(struct hdr10_param_s, Bx) == 16, "hdr10_param_s.Bx offset");
_Static_assert(offsetof(struct hdr10_param_s, By) == 18, "hdr10_param_s.By offset");
_Static_assert(offsetof(struct hdr10_param_s, Wx) == 20, "hdr10_param_s.Wx offset");
_Static_assert(offsetof(struct hdr10_param_s, Wy) == 22, "hdr10_param_s.Wy offset");
_Static_assert(offsetof(struct hdr10_param_s, max_content_light_level) == 24, "hdr10_param_s.max_content_light_level offset");
_Static_assert(offsetof(struct hdr10_param_s, max_pic_average_light_level) == 26, "hdr10_param_s.max_pic_average_light_level offset");
_Static_assert(offsetof(struct dovi_setting_s, comp_reg) == 0, "dovi_setting_s.comp_reg offset");
_Static_assert(offsetof(struct dovi_setting_s, dm_reg1) == 692, "dovi_setting_s.dm_reg1 offset");
_Static_assert(offsetof(struct dovi_setting_s, dm_reg2) == 800, "dovi_setting_s.dm_reg2 offset");
_Static_assert(offsetof(struct dovi_setting_s, dm_reg3) == 896, "dovi_setting_s.dm_reg3 offset");
_Static_assert(offsetof(struct dovi_setting_s, dm_lut1) == 1000, "dovi_setting_s.dm_lut1 offset");
_Static_assert(offsetof(struct dovi_setting_s, dm_lut2) == 6120, "dovi_setting_s.dm_lut2 offset");
_Static_assert(offsetof(struct dovi_setting_s, md_reg3) == 11240, "dovi_setting_s.md_reg3 offset");
_Static_assert(offsetof(struct dovi_setting_s, hdr_info) == 13292, "dovi_setting_s.hdr_info offset");
_Static_assert(offsetof(struct dovi_setting_s, src_format) == 13324, "dovi_setting_s.src_format offset");
_Static_assert(offsetof(struct dovi_setting_s, dst_format) == 13328, "dovi_setting_s.dst_format offset");
_Static_assert(offsetof(struct dovi_setting_s, el_flag) == 13332, "dovi_setting_s.el_flag offset");
_Static_assert(offsetof(struct dovi_setting_s, el_halfsize_flag) == 13333, "dovi_setting_s.el_halfsize_flag offset");
_Static_assert(offsetof(struct dovi_setting_s, video_width) == 13336, "dovi_setting_s.video_width offset");
_Static_assert(offsetof(struct dovi_setting_s, video_height) == 13340, "dovi_setting_s.video_height offset");
_Static_assert(offsetof(struct dovi_setting_s, g_format) == 13344, "dovi_setting_s.g_format offset");
_Static_assert(offsetof(struct dovi_setting_s, g_bitdepth) == 13348, "dovi_setting_s.g_bitdepth offset");
_Static_assert(offsetof(struct dovi_setting_s, dovi2hdr10_nomapping) == 13352, "dovi_setting_s.dovi2hdr10_nomapping offset");
_Static_assert(offsetof(struct dovi_setting_s, use_ll_flag) == 13356, "dovi_setting_s.use_ll_flag offset");
_Static_assert(offsetof(struct dovi_setting_s, ll_rgb_desired) == 13360, "dovi_setting_s.ll_rgb_desired offset");
_Static_assert(offsetof(struct dovi_setting_s, diagnostic_enable) == 13364, "dovi_setting_s.diagnostic_enable offset");
_Static_assert(offsetof(struct dovi_setting_s, diagnostic_mux_select) == 13368, "dovi_setting_s.diagnostic_mux_select offset");
_Static_assert(offsetof(struct dovi_setting_s, dovi_ll_enable) == 13372, "dovi_setting_s.dovi_ll_enable offset");
_Static_assert(offsetof(struct dovi_setting_s, vout_width) == 13376, "dovi_setting_s.vout_width offset");
_Static_assert(offsetof(struct dovi_setting_s, vout_height) == 13380, "dovi_setting_s.vout_height offset");
_Static_assert(offsetof(struct dovi_setting_s, vsvdb_tbl) == 13384, "dovi_setting_s.vsvdb_tbl offset");
_Static_assert(offsetof(struct dovi_setting_s, ext_md) == 13416, "dovi_setting_s.ext_md offset");
_Static_assert(sizeof(struct dolby_vision_func_s) == 56, "dolby_vision_func_s size");
_Static_assert(offsetof(struct dolby_vision_func_s, version_info) == 0, "callback version_info offset");
_Static_assert(offsetof(struct dolby_vision_func_s, metadata_parser_init) == 8, "callback metadata_parser_init offset");
_Static_assert(offsetof(struct dolby_vision_func_s, metadata_parser_reset) == 16, "callback metadata_parser_reset offset");
_Static_assert(offsetof(struct dolby_vision_func_s, metadata_parser_process) == 24, "callback metadata_parser_process offset");
_Static_assert(offsetof(struct dolby_vision_func_s, metadata_parser_release) == 32, "callback metadata_parser_release offset");
_Static_assert(offsetof(struct dolby_vision_func_s, control_path) == 40, "callback control_path offset");
_Static_assert(offsetof(struct dolby_vision_func_s, reserved_func) == 48, "callback reserved_func offset");
#endif

#endif /* AM6B_DOVI_ABI_RECOVERED_H */
