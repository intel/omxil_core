/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __WRS_OMXIL_AUDIO_PARSER
#define ___WRS_OMXIL_AUDIO_PARSER

#ifdef __cplusplus
extern "C" {
#endif

/*
 * MP3
 */

#define MP3_HEADER_VERSION_25           0x0
#define MP3_HEADER_VERSION_2            0x2
#define MP3_HEADER_VERSION_1            0x3

#define MP3_HEADER_LAYER_3              0x1
#define MP3_HEADER_LAYER_2              0x2
#define MP3_HEADER_LAYER_1              0x3

#define MP3_HEADER_CRC_PROTECTED        0x0
#define MP3_HEADER_NOT_PROTECTED        0x1

#define MP3_HEADER_STEREO               0x0
#define MP3_HEADER_JOINT_STEREO         0x1
#define MP3_HEADER_DUAL_CHANNEL         0x2
#define MP3_HEADER_SINGLE_CHANNEL       0x3

int mp3_header_parse(const unsigned char *buffer,
                     int *version, int *layer, int *crc, int *bitrate,
                     int *frequency, int *channel, int *mode_extension,
                     int *frame_length, int *frame_duration);

/* end of MP3 */

/*
 * MP4
 */

int audio_specific_config_parse(const unsigned char *buffer,
                                int *aot, int *frequency, int *channel);

int audio_specific_config_bitcoding(unsigned char *buffer,
                                    int aot, int frequency, int channel);

/* end of MP4 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ___WRS_OMXIL_AUDIO_PARSER */
