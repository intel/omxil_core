/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __WRS_OMXIL_H264_PARSER
#define ___WRS_OMXIL_H264_PARSER

#ifdef __cplusplus
extern "C" {
#endif

#define USE_PV_BITOPS	1
#define USE_MIXVBP_DEF	1
#define USE_VUI_PARSER  0

#if USE_PV_BITOPS
/* Port from PV OpenCORE 2.02 */
typedef struct _nal_stream_t {
	unsigned char *data;
	unsigned int numbytes;
	unsigned int bytepos;
	unsigned int bitbuf;
	unsigned int databitpos;
	unsigned int bitpos;
} nal_stream;
#endif

#if USE_MIXVBP_DEF
/* Import from MI-X VBP header */
#define MAX_NUM_SPS	32
/* status codes */
typedef enum _h264_Status
{
	H264_STATUS_EOF          =  1,   // end of file
	H264_STATUS_OK           =  0,   // no error
	H264_STATUS_NO_MEM       =  2,   // out of memory
	H264_STATUS_FILE_ERROR   =  3,   // file error
	H264_STATUS_NOTSUPPORT   =  4,   // not supported mode
	H264_STATUS_PARSE_ERROR  =  5,   // fail in parse MPEG-4 stream
	H264_STATUS_ERROR        =  6,   // unknown/unspecified error
	H264_NAL_ERROR,
	H264_SPS_INVALID_PROFILE,
	H264_SPS_INVALID_LEVEL,
	H264_SPS_INVALID_SEQ_PARAM_ID,
	H264_SPS_ERROR,
	H264_PPS_INVALID_PIC_ID,
	H264_PPS_INVALID_SEQ_ID,
	H264_PPS_ERROR,
	H264_SliceHeader_INVALID_MB,
	H264_SliceHeader_ERROR,
	H264_FRAME_DONE,
	H264_SLICE_DONE,        
	H264_STATUS_POLL_ONCE_ERROR,
	H264_STATUS_DEC_MEMINIT_ERROR,
	H264_STATUS_NAL_UNIT_TYPE_ERROR,
	H264_STATUS_SEI_ERROR,
	H264_STATUS_SEI_DONE,
} h264_Status;

typedef enum _h264_Profile
{
	h264_ProfileBaseline = 66,          /** Baseline profile */
	h264_ProfileMain = 77,              /** Main profile */
	h264_ProfileExtended = 88,          /** Extended profile */
	h264_ProfileHigh = 100 ,            /** High profile */
	h264_ProfileHigh10 = 110,           /** High 10 profile */
	h264_ProfileHigh422 = 122,          /** High profile 4:2:2 */
	h264_ProfileHigh444 = 144,          /** High profile 4:4:4 */
} h264_Profile;

typedef enum _h264_Level
{
	h264_Level1b        = 9,            /** Level 1b */
	h264_Level1         = 10,           /** Level 1 */
	h264_Level11        = 11,           /** Level 1.1 */
	h264_Level12        = 12,           /** Level 1.2 */
	h264_Level13        = 13,           /** Level 1.3 */
	h264_Level2         = 20,           /** Level 2 */
	h264_Level21        = 21,           /** Level 2.1 */
	h264_Level22        = 22,           /** Level 2.2 */
	h264_Level3         = 30,           /** Level 3 */
	h264_Level31        = 31,           /** Level 3.1 */
	h264_Level32        = 32,           /** Level 3.2 */
	h264_Level4         = 40,           /** Level 4 */
	h264_Level41        = 41,           /** Level 4.1 */
	h264_Level42        = 42,           /** Level 4.2 */
	h264_Level5         = 50,           /** Level 5 */
	h264_Level51        = 51,           /** Level 5.1 */
	h264_LevelReserved = 255  /** Unknown profile */
} h264_Level;

#endif

/*
 * VUI
 */

#define Extended_SAR	255

typedef struct _VUI_t {
	unsigned char aspect_ratio_info_present_flag;
	unsigned char aspect_ratio_idc;
	unsigned short sar_width;
	unsigned short sar_height;
	unsigned char overscan_info_present_flag;
	unsigned char overscan_appropriate_flag;
	unsigned char video_signal_type_present_flag;
	unsigned char video_format;
	unsigned char video_full_range_flag;
	unsigned char colour_description_present_flag;
	unsigned char colour_primaries;
	unsigned char transfer_characteristics;
	unsigned char matrix_coefficients;
	unsigned char chroma_loc_info_present_flag;
	unsigned int chroma_sample_loc_type_top_field;
	unsigned int chroma_sample_loc_type_bottom_field;
	unsigned int timing_info_present_flag;
	unsigned int num_units_in_tick;
	unsigned int time_scale;
	unsigned int fixed_frame_rate_flag;
	/* TBD */
} VUI;

int nal_sps_vui_parse(const unsigned char *buffer, int *framerate);

/*
 * SPS
 */

typedef struct _SPS_t {
	unsigned char profile_idc;
	unsigned char constraint_flags;
	unsigned short level_idc;
	unsigned int seq_parameter_set_id;
	unsigned int chroma_format_idc;
	unsigned int separate_colour_plane_flag;
	unsigned int bit_depth_luma_minus8;
	unsigned int bit_depth_chroma_minus8;
	unsigned short qpprime_y_zero_transform_bypass_flag;
	unsigned short seq_scaling_matrix_present_flag;
	unsigned char *seq_scaling_list_present_flag;
	unsigned char scaling_list_4x4[6][16];
	unsigned char scaling_list_8x8[2][64];
	unsigned char use_default_scaling_matrix_4x4_flag[6];
	unsigned char use_default_scaling_matrix_8x8_flag[6];
	unsigned int log2_max_frame_num_minus4;
	unsigned int pic_order_cnt_type;
	unsigned int log2_max_pic_order_cnt_lsb_minus4;
	unsigned int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	unsigned int num_ref_frames_in_pic_order_cnt_cycle;
	int *offset_for_ref_frame;
	unsigned int max_num_ref_frames;
	unsigned int gaps_in_frame_num_value_allowed_flag;
	unsigned int pic_width_in_mbs_minus1;
	unsigned int pic_height_in_map_units_minus1;
	unsigned char frame_mbs_only_flag;
	unsigned char mb_adaptive_frame_field_flag;
	unsigned char direct_8x8_inference_flag;
	unsigned char frame_cropping_flag;
	unsigned int frame_crop_left_offset;
	unsigned int frame_crop_right_offset;
	unsigned int frame_crop_top_offset;
	unsigned int frame_crop_bottom_offset;
	unsigned int vui_parameters_present_flag;
	VUI *vui;
} SPS;

int nal_sps_parse(unsigned char *buffer, unsigned int len,
		  unsigned int *width, unsigned int *height,
		  unsigned int *stride, unsigned int *sliceheight);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ___WRS_OMXIL_H264_PARSER */
