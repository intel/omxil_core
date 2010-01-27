/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#include <h264_parser.h>

//#define LOG_NDEBUG 0

#define LOG_TAG "h264_parser"
#include <log.h>

#if USE_PV_BITOPS
/*
 * NOTE: The original idea of this code for bit operation is from PV's OpenCORE 2.02
 * Wind River Systems modified PV source for se(v) operation.
 *
 */

#define PV_CLZ(A,B) while (((B) & 0x8000) == 0) {(B) <<=1; A++;}

static const unsigned int mask[33] =
{
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
	0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
	0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
	0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
	0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
	0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
	0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
	0xffffffff
};

static int read_bits(nal_stream *pstream, unsigned char nbits, unsigned int *outdata)
{
	unsigned char *bits;
	unsigned int databitpos = pstream->databitpos;
	unsigned int bitpos = pstream->bitpos;
	unsigned int databytepos;

	if ((databitpos + nbits) > (pstream->numbytes << 3))
	{
		*outdata = 0;
		// buffer over run
		LOGE("%s: buffer over run",__func__);
		return -1;
	}

	//  databitpos += nbits;

	if (nbits > (32 - bitpos))    /* not enough bits */
	{
		databytepos = databitpos >> 3;    /* byte aligned position */
		bitpos = databitpos & 7; /* update bit position */
		bits = &pstream->data[databytepos];
		pstream->bitbuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
	}

	pstream->databitpos += nbits;
	pstream->bitpos      = (unsigned char)(bitpos + nbits);

	*outdata = (pstream->bitbuf >> (32 - pstream->bitpos)) & mask[(unsigned short)nbits];
	return 0;
}

static void show_bits(nal_stream *pstream, unsigned char nbits, unsigned int *outdata)
{
	unsigned char *bits;
	unsigned int databitpos = pstream->databitpos;
	unsigned int bitpos = pstream->bitpos;
	unsigned int databytepos;

	uint i;

	if (nbits > (32 - bitpos))    /* not enough bits */
	{
		databytepos = databitpos >> 3; /* byte aligned position */
		bitpos = databitpos & 7; /* update bit position */
		if (databytepos > pstream->numbytes - 4)
		{
			pstream->bitbuf = 0;
			for (i = 0; i < pstream->numbytes - databytepos; i++)
			{
				pstream->bitbuf |= pstream->data[databytepos+i];
				pstream->bitbuf <<= 8;
			}
			pstream->bitbuf <<= 8 * (3 - i);
		}
		else
		{
			bits = &pstream->data[databytepos];
			pstream->bitbuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
		}
		pstream->bitpos = bitpos;
	}

	bitpos += nbits;

	*outdata = (pstream->bitbuf >> (32 - bitpos)) & mask[(unsigned short)nbits];
}

static void flush_bits(nal_stream *pstream, unsigned char nbits)
{
	unsigned char *bits;
	unsigned int databitpos = pstream->databitpos;
	unsigned int bitpos = pstream->bitpos;
	unsigned int databytepos;

	if ((databitpos + nbits) > (unsigned int)(pstream->numbytes << 3)) {
		// buffer over run
		LOGE("%s: buffer over run",__func__);
	}

	databitpos += nbits;
	bitpos     += nbits;

	if (bitpos > 32)
	{
		databytepos = databitpos >> 3;    /* byte aligned position */
		bitpos = databitpos & 7; /* update bit position */
		bits = &pstream->data[databytepos];
		pstream->bitbuf = (bits[0] << 24) | (bits[1] << 16) | (bits[2] << 8) | bits[3];
	}

	pstream->databitpos = databitpos;
	pstream->bitpos     = bitpos;
}

static void read_ue_bits(nal_stream *pstream, unsigned int *coded_val)
{
	unsigned int temp;
	unsigned int tmp_cnt;
	int leading_zeros = 0;

	show_bits(pstream, 16, &temp);
	tmp_cnt = temp  | 0x1;

	PV_CLZ(leading_zeros, tmp_cnt)

	if (leading_zeros < 8)
	{
		*coded_val = (temp >> (15 - (leading_zeros << 1))) - 1;
		flush_bits(pstream, (leading_zeros << 1) + 1);
	}
	else
	{
		read_bits(pstream, (leading_zeros << 1) + 1, &temp);
		*coded_val = temp - 1;
	}
}

static void read_se_bits(nal_stream *pstream, int *coded_val)
{
	int leading_zeros = 0;
	unsigned int temp;

	read_bits(pstream, 1, &temp);
	while (!temp)
	{
		leading_zeros++;
		if (read_bits(pstream, 1, &temp))
		{
			break;
		}
	}
	read_bits(pstream, leading_zeros, &temp);
}
#endif /* USE_PV_BITOPS */

#if USE_MIXVBP_DEF
static h264_Status h264_scaling_list(nal_stream *pstream,
			unsigned char *scaling_list,
			int sizeof_scaling_list,
			unsigned char *use_default_scaling_matrix)
{
	int j, scanj;
	int delta_scale, last_scale, next_scale;

	last_scale = 8;
	next_scale = 8;
	scanj = 0;

	for(j=0; j<sizeof_scaling_list; j++) {
		if(next_scale!=0) {
			read_se_bits(pstream, &delta_scale);
			next_scale = (last_scale + delta_scale + 256) % 256;
			*use_default_scaling_matrix = (unsigned char) (scanj==0 && next_scale==0);
		}

		scaling_list[scanj] = (next_scale==0) ? last_scale:next_scale;
		last_scale = scaling_list[scanj];
		scanj ++;
	}

	return H264_STATUS_OK;
}
#endif

static int _sps_parse(nal_stream *pstream, SPS *sps)
{
	h264_Status status = H264_SPS_ERROR;
	unsigned int val;
	int sval;

	read_bits(pstream, 8, &val);
	sps->profile_idc = (unsigned char)val;
	LOGV("%s: sps->profile_idc = %d", __func__, val);

	switch(sps->profile_idc) {
		case h264_ProfileBaseline:
		case h264_ProfileMain:
		case h264_ProfileExtended:
		case h264_ProfileHigh10:
		case h264_ProfileHigh422:
		case h264_ProfileHigh444:
		case h264_ProfileHigh:
			break;
		default:
			return H264_SPS_INVALID_PROFILE;
	}

	read_bits(pstream, 8, &val);
	sps->constraint_flags = (unsigned char)val;
	LOGV("%s: sps->constraint_flags = %d", __func__, val);

	read_bits(pstream, 8, &val);
	sps->level_idc = (unsigned short)val;
	LOGV("%s: sps->level_idc = %d", __func__, val);

	switch(sps->level_idc)
	{
		case h264_Level1b:
		case h264_Level1:
		case h264_Level11:
		case h264_Level12:
		case h264_Level13:
		case h264_Level2:
		case h264_Level21:
		case h264_Level22:
		case h264_Level3:
		case h264_Level31:
		case h264_Level32:
		case h264_Level4:
		case h264_Level41:
		case h264_Level42:
		case h264_Level5:
		case h264_Level51:
			break;
		default:
			return H264_SPS_INVALID_LEVEL;
	}

	do {
		read_ue_bits(pstream, &val);
		sps->seq_parameter_set_id = val;
		LOGV("%s: sps->seq_parameter_set_id = %d", __func__, val);
		if (val > MAX_NUM_SPS - 1) break;

		if((sps->profile_idc == h264_ProfileHigh) || 
		   (sps->profile_idc == h264_ProfileHigh10) ||
		   (sps->profile_idc == h264_ProfileHigh422) ||
		   (sps->profile_idc == h264_ProfileHigh444))
		{

			read_ue_bits(pstream, &val);
			sps->chroma_format_idc = val;
			LOGV("%s: sps->chroma_format_idc = %d", __func__, val);

			if (val == 3) {
				read_bits(pstream, 1, &val);
				sps->separate_colour_plane_flag = val;
				LOGV("%s: sps->separate_colour_plane_flag = %d", __func__, val);
			}

			read_ue_bits(pstream, &val);
			sps->bit_depth_luma_minus8 = val;
			LOGV("%s: sps->bit_depth_luma_minus8 = %d", __func__, val);

			read_ue_bits(pstream, &val);
			sps->bit_depth_chroma_minus8 = val;
			LOGV("%s: sps->bit_depth_chroma_minus8 = %d", __func__, val);

			read_bits(pstream, 1, &val);
			sps->qpprime_y_zero_transform_bypass_flag = (unsigned short)val;
			LOGV("%s: sps->qpprime_y_zero_transform_bypass_flag = %d", __func__, val);

			read_bits(pstream, 1, &val);
			sps->seq_scaling_matrix_present_flag = (unsigned short)val;
			LOGV("%s: sps->seq_scaling_matrix_present_flag = %d", __func__, val);

			if (val) {
				unsigned int i;
				unsigned int _count_i = (sps->chroma_format_idc != 3)?8:12;
				sps->seq_scaling_list_present_flag = malloc(sizeof(unsigned char)*_count_i);
				for(i=0; i<_count_i; i++) {
					read_bits(pstream, 1, &val);
					sps->seq_scaling_list_present_flag[i] = (unsigned char)val;
					if (val) {
						if (i<6) {
							h264_scaling_list(pstream,
								sps->scaling_list_4x4[i], 16,
								&sps->use_default_scaling_matrix_4x4_flag[i]);
						} else {
							h264_scaling_list(pstream,
								sps->scaling_list_8x8[i-6], 64,
								&sps->use_default_scaling_matrix_8x8_flag[i-6]);
						}
					}
				}
			}
		} else {
			sps->chroma_format_idc = 1;
			sps->seq_scaling_matrix_present_flag = 0;

			sps->bit_depth_luma_minus8 = 0;
			sps->bit_depth_chroma_minus8 = 0;
		}

		read_ue_bits(pstream, &val);
		sps->log2_max_frame_num_minus4 = val;
		LOGV("%s: sps->log2_max_frame_num_minus4 = %d", __func__, val);

		read_ue_bits(pstream, &val);
		sps->pic_order_cnt_type = val;
		LOGV("%s: sps->pic_order_cnt_type = %d", __func__, val);

		if (val == 0) {
			read_ue_bits(pstream, &val);
			sps->log2_max_pic_order_cnt_lsb_minus4 = val;
			LOGV("%s: sps->log2_max_pic_order_cnt_lsb_minus4 = %d", __func__, val);
		} else if (val == 1) {
			unsigned int i;

			read_bits(pstream, 1, &val);
			sps->delta_pic_order_always_zero_flag = val;
			LOGV("%s: sps->delta_pic_order_always_zero_flag = %d", __func__, val);

			read_se_bits(pstream, &sval);
			sps->offset_for_non_ref_pic = sval;
			LOGV("%s: sps->offset_for_non_ref_pic = %d", __func__, val);

			read_se_bits(pstream, &sval);
			sps->offset_for_top_to_bottom_field = sval;
			LOGV("%s: sps->offset_for_top_to_bottom_field = %d", __func__, val);

			read_ue_bits(pstream, &val);
			sps->num_ref_frames_in_pic_order_cnt_cycle = val;
			LOGV("%s: sps->num_ref_frames_in_pic_order_cnt_cycle = %d", __func__, val);

			sps->offset_for_ref_frame = malloc(sizeof(int)*val);
			for (i=0; i<sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
				read_se_bits(pstream, &sval);
				sps->offset_for_ref_frame[i] = sval;
			}

		}

		read_ue_bits(pstream, &val);
		sps->max_num_ref_frames = val;
		LOGV("%s: sps->max_num_ref_frames = %d", __func__, val);

		read_bits(pstream, 1, &val);
		sps->gaps_in_frame_num_value_allowed_flag = val;
		LOGV("%s: sps->gaps_in_frame_num_value_allowed_flag = %d", __func__, val);

		read_ue_bits(pstream, &val);
		sps->pic_width_in_mbs_minus1 = val;
		LOGV("%s: sps->pic_width_in_mbs_minus1 = %d", __func__, val);

		read_ue_bits(pstream, &val);
		sps->pic_height_in_map_units_minus1 = val;
		LOGV("%s: sps->pic_height_in_map_units_minus1 = %d", __func__, val);

		read_bits(pstream, 1, &val);
		sps->frame_mbs_only_flag = (unsigned char)val;
		LOGV("%s: sps->frame_mbs_only_flag = %d", __func__, val);

		if (!val) {
			read_bits(pstream, 1, &val);
			sps->mb_adaptive_frame_field_flag = (unsigned char)val;
			LOGV("%s: sps->mb_adaptive_frame_field_flag = %d", __func__, val);
		}

		read_bits(pstream, 1, &val);
		sps->direct_8x8_inference_flag = (unsigned char)val;
		LOGV("%s: sps->direct_8x8_inference_flag = %d", __func__, val);

		read_bits(pstream, 1, &val);
		sps->frame_cropping_flag = (unsigned char)val;
		LOGV("%s: sps->frame_cropping_flag = %d", __func__, val);

		if (val) {
			read_ue_bits(pstream, &val);
			sps->frame_crop_left_offset = val;
			LOGV("%s: sps->frame_crop_left_offset = %d", __func__, val);

			read_ue_bits(pstream, &val);
			sps->frame_crop_right_offset = val;
			LOGV("%s: sps->frame_crop_right_offset = %d", __func__, val);

			read_ue_bits(pstream, &val);
			sps->frame_crop_top_offset = val;
			LOGV("%s: sps->frame_crop_top_offset = %d", __func__, val);

			read_ue_bits(pstream, &val);
			sps->frame_crop_bottom_offset = val;
			LOGV("%s: sps->frame_crop_bottom_offset = %d", __func__, val);
		}

		read_bits(pstream, 1, &val);
		sps->vui_parameters_present_flag = val;
		LOGV("%s: sps->vui_parameters_present_flag = %d", __func__, val);

		status = H264_STATUS_OK;
	} while(0);

	return status;
}

static int _vui_parse(nal_stream *pstream, SPS *sps)
{
	unsigned int val;
	int sval;
	h264_Status status = H264_SPS_ERROR;
	VUI* vui = sps->vui;

	if (!vui) {
		LOGE("%s: Invalid VUI", __func__);
		return status;
	}

	read_bits(pstream, 1, &val);
	vui->aspect_ratio_info_present_flag = (unsigned char)val;
	LOGV("%s: vui->aspect_ratio_info_present_flag = %d", __func__, val);

	if (val) {
		read_bits(pstream, 8, &val);
		vui->aspect_ratio_idc = (unsigned char)val;
		LOGV("%s: vui->aspect_ratio_idc = %d", __func__, val);

		if (val == Extended_SAR) {
			read_bits(pstream, 16, &val);
			vui->sar_width = (unsigned short)val;
			LOGV("%s: vui->sar_width = %d", __func__, val);

			read_bits(pstream, 16, &val);
			vui->sar_height = (unsigned short)val;
			LOGV("%s: vui->sar_height = %d", __func__, val);
		}
	}

	read_bits(pstream, 1, &val);
	vui->overscan_info_present_flag = (unsigned char)val;
	LOGV("%s: vui->overscan_info_present_flag = %d", __func__, val);

	if (val) {
		read_bits(pstream, 1, &val);
		vui->overscan_appropriate_flag = (unsigned char)val;
		LOGV("%s: vui->overscan_appropriate_flag = %d", __func__, val);
	}

	read_bits(pstream, 1, &val);
	vui->video_signal_type_present_flag = (unsigned char)val;
	LOGV("%s: vui->video_signal_type_present_flag = %d", __func__, val);

	if (val) {
		read_bits(pstream, 3, &val);
		vui->video_format = (unsigned char)val;
		LOGV("%s: vui->video_format = %d", __func__, val);

		read_bits(pstream, 1, &val);
		vui->video_full_range_flag = (unsigned char)val;
		LOGV("%s: vui->video_full_range_flag = %d", __func__, val);

		read_bits(pstream, 1, &val);
		vui->colour_description_present_flag = (unsigned char)val;
		LOGV("%s: vui->colour_description_present_flag = %d", __func__, val);

		if (val) {
			read_bits(pstream, 8, &val);
			vui->colour_primaries = (unsigned char)val;
			LOGV("%s: vui->colour_primaries = %d", __func__, val);

			read_bits(pstream, 8, &val);
			vui->transfer_characteristics = (unsigned char)val;
			LOGV("%s: vui->transfer_characteristics = %d", __func__, val);

			read_bits(pstream, 8, &val);
			vui->matrix_coefficients = (unsigned char)val;
			LOGV("%s: vui->matrix_coefficients = %d", __func__, val);
		}
	}

	read_bits(pstream, 1, &val);
	vui->chroma_loc_info_present_flag = (unsigned char)val;
	LOGV("%s: vui->chroma_loc_info_present_flag = %d", __func__, val);

	if (val) {
		read_ue_bits(pstream, &val);
		vui->chroma_sample_loc_type_top_field = val;
		LOGV("%s: vui->chroma_sample_loc_type_top_field = %d", __func__, val);

		read_ue_bits(pstream, &val);
		vui->chroma_sample_loc_type_bottom_field = val;
		LOGV("%s: vui->chroma_sample_loc_type_bottom_field = %d", __func__, val);
	}

	read_bits(pstream, 1, &val);
	vui->timing_info_present_flag = val;
	LOGV("%s: vui->timing_info_present_flag = %d", __func__, vui->timing_info_present_flag);

	if (val) {
		read_bits(pstream, 32, &val);
		vui->num_units_in_tick = val;
		LOGV("%s: vui->num_units_in_tick = %d", __func__, vui->num_units_in_tick);

		read_bits(pstream, 32, &val);
		vui->time_scale = val;
		LOGV("%s: vui->time_scale = %d", __func__, vui->time_scale);

		read_bits(pstream, 1, &val);
		vui->fixed_frame_rate_flag = val;
		LOGV("%s: vui->fixed_frame_rate_flag = %d", __func__, vui->fixed_frame_rate_flag);
	}

	/* TBD */

	return H264_STATUS_OK;
}

static int calculate_stride(unsigned int width,
                            unsigned int height,
                            unsigned int *stride)
{
    if ((width <= 0) || (width > 4096) || (height <= 0) || (height > 4096))
    {
        return -1;
    }

    if (512 >= width)
    {
        *stride = 512;
    }
    else if (1024 >= width)
    {
        *stride = 1024;
    }
    else if (1280 >= width)
    {
        *stride = 1280;
    }
    else if (2048 >= width)
    {
        *stride = 2048;
    }
    else if (4096 >= width)
    {
        *stride = 4096;
    }
    else
    {
        return -1;
    }
    return 0;
}

int nal_sps_parse(unsigned char *buffer, unsigned int len,
		  unsigned int *width, unsigned int *height,
		  unsigned int *stride, unsigned int *sliceheight)
{
	SPS sps;
	VUI vui;
	nal_stream nalst;
	unsigned int crop_left, crop_right, crop_top, crop_bottom;
	unsigned int val;
	int status = H264_SPS_ERROR;

	nalst.data = buffer;
	nalst.numbytes = len;
	nalst.bitbuf = 0;
	nalst.bitpos = 32;
	nalst.bytepos = 0;
	nalst.databitpos = 0;

	memset(&sps, 0, sizeof(sps));
	memset(&vui, 0, sizeof(vui));
	sps.vui = &vui;

	read_bits(&nalst, 8, &val);
	if ((val & 0x1F) != 7) {
		LOGE("%s: NAL type is not SPS. ERROR", __func__);
		goto init_error;
	}

	/* parse SPS */
	status = _sps_parse(&nalst, &sps);
	if (status) goto sps_error;

#if USE_VUI_PARSER
	if (sps.vui_parameters_present_flag) {
		/* parse VUI */
		status = _vui_parse(&nalst, &sps);
		if (status) goto vui_error;
	} else {
		LOGE("%s: VUI parameters present flag is OFF from SPS", __func__);
		goto vui_error;
	}
#endif

	/* calculate width, height, stride, sliceheight */
	*width = (sps.pic_width_in_mbs_minus1 + 1) * 16;
	*height = (sps.pic_height_in_map_units_minus1 + 1) * 16;
	LOGV("%s: width = %d", __func__, *width);
	LOGV("%s: height = %d", __func__, *height);

	if (sps.frame_cropping_flag) {
		crop_left = 2 * sps.frame_crop_left_offset;
		crop_right = *width - (2 * sps.frame_crop_right_offset + 1);

		if (sps.frame_mbs_only_flag) {
			crop_top = 2 * sps.frame_crop_top_offset;
			crop_bottom = *height - (2 * sps.frame_crop_bottom_offset + 1);
		} else {
			crop_top = 4 * sps.frame_crop_top_offset;
			crop_bottom = *height - (4 * sps.frame_crop_bottom_offset + 1);
		}
	} else {
		crop_bottom = *height - 1;
		crop_right = *width - 1;
		crop_top = crop_left = 0;
	}
	*width = crop_right - crop_left + 1;
	*height = crop_bottom - crop_top + 1;

#if 0
	/* NOTE: calculating method from psb_video library */
	calculate_stride(*width, *height, stride);
	*sliceheight = (*height + 0x1F) & ~0x1F;
#else
	*stride = *width;
	*sliceheight = *height;
#endif

	LOGV("%s: frame width = %d", __func__, *width);
	LOGV("%s: frame height = %d", __func__, *height);
	LOGV("%s: stride = %d", __func__, *stride);
	LOGV("%s: sliceheight = %d", __func__, *sliceheight);

	return H264_STATUS_OK;

 sps_error:
	*width = 0;
	*height = 0;
 vui_error:
	if (sps.seq_scaling_list_present_flag)
		free(sps.seq_scaling_list_present_flag);
	if (sps.offset_for_ref_frame)
		free(sps.offset_for_ref_frame);
 init_error:
	return status;
}
