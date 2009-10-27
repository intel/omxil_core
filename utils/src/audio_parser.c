/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#include <endian.h>

#define LOG_NDEBUG 1

#define LOG_TAG "audio_parser"
#include <log.h>

/*
 * MP3
 */

struct mp3_frame_header_s {
    union {
#if (__BYTE_ORDER == __LITTLE_ENDIAN)
        struct {
            unsigned int
                emphasis : 2,
                original : 1,
                copyright : 1,
                mode_extension : 2,
                channel_mode : 2,
                private_bit : 1,
                padding_bit : 1,
                samplingrate_index : 2,
                bitrate_index : 4,
                protected : 1,
                layer_index : 2,
                version_index : 2,
                sync : 11;
        };
        struct {
            unsigned char h0, h1, h2, h3;
        };
#elif (__BYTE_ORDER == __BIG_ENDIAN)
        struct {
            unsigned int
                sync : 11,
                version_index : 2,
                layer_index : 2,
                protected : 1,
                bitrate_index : 4,
                samplingrate_index : 2,
                padding_bit : 1,
                private_bit : 1,
                channel_mode : 2,
                mode_extension : 2,
                copyright : 1,
                original : 1,
                emphasis : 2;
        };
        struct {
            unsigned char h3, h2, h1, h0;
        };
#endif
    };
} __attribute__ ((packed));

/* index : sampling rate index */
static const int sampling_rate_table_version_1[4] = {
    [0] = 44100,
    [1] = 48000,
    [2] = 32000,
    [3] = 0,
};

/* index : sampling rate index */
static const int sampling_rate_table_version_2[4] = {
    [0] = 22050,
    [1] = 24000,
    [2] = 16000,
    [3] = 0,
};

/* index : sampling rate index */
static const int sampling_rate_table_version_25[4] = {
    [0] = 11025,
    [1] = 12000,
    [2] = 8000,
    [3] = 0,
};

/* index : version index */
static const int *sampling_rate_table[4] = {
    [0] = &sampling_rate_table_version_25[0],
    [1] = NULL,
    [2] = &sampling_rate_table_version_2[0],
    [3] = &sampling_rate_table_version_1[0],
};

/* index : bitrate index */
static const int bitrate_table_version_1_layer_1[16] = {
    [0] = 0,
    [1] = 32,
    [2] = 64,
    [3] = 96,
    [4] = 128,
    [5] = 160,
    [6] = 192,
    [7] = 224,
    [8] = 256,
    [9] = 288,
    [10] = 320,
    [11] = 352,
    [12] = 384,
    [13] = 416,
    [14] = 448,
    [15] = 0,
};

/* index : bitrate index */
static const int bitrate_table_version_1_layer_2[16] = {
    [0] = 0,
    [1] = 32,
    [2] = 48,
    [3] = 56,
    [4] = 64,
    [5] = 80,
    [6] = 96,
    [7] = 112,
    [8] = 128,
    [9] = 160,
    [10] = 192,
    [11] = 224,
    [12] = 256,
    [13] = 320,
    [14] = 384,
    [15] = 0,
};

/* index : bitrate index */
static const int bitrate_table_version_1_layer_3[16] = {
    [0] = 0,
    [1] = 32,
    [2] = 40,
    [3] = 48,
    [4] = 56,
    [5] = 64,
    [6] = 80,
    [7] = 96,
    [8] = 112,
    [9] = 128,
    [10] = 160,
    [11] = 192,
    [12] = 224,
    [13] = 256,
    [14] = 320,
    [15] = 0,
};

/* index : bitrate index */
static const int bitrate_table_version_2_25_layer_1[16] = {
    [0] = 0,
    [1] = 32,
    [2] = 64,
    [3] = 96,
    [4] = 128,
    [5] = 160,
    [6] = 192,
    [7] = 224,
    [8] = 256,
    [9] = 288,
    [10] = 320,
    [11] = 352,
    [12] = 384,
    [13] = 416,
    [14] = 448,
    [15] = 0,
};

/* index : bitrate index */
static const int bitrate_table_version_2_25_layer_2_3[16] = {
    [0] = 0,
    [1] = 8,
    [2] = 16,
    [3] = 24,
    [4] = 32,
    [5] = 40,
    [6] = 48,
    [7] = 56,
    [8] = 64,
    [9] = 80,
    [10] = 96,
    [11] = 112,
    [12] = 128,
    [13] = 144,
    [14] = 160,
    [15] = 0,
};

/* index : layer index */
static const int *bitrate_table_version_1[4] = {
    [0] = NULL,
    [1] = &bitrate_table_version_1_layer_3[0],
    [2] = &bitrate_table_version_1_layer_2[0],
    [3] = &bitrate_table_version_1_layer_1[0]
};

/* index : layer index */
static const int *bitrate_table_version_2_25[4] = {
    [0] = NULL,
    [1] = &bitrate_table_version_2_25_layer_2_3[0],
    [2] = &bitrate_table_version_2_25_layer_2_3[0],
    [3] = &bitrate_table_version_2_25_layer_1[0],
};

/* index : version index */
static const int **bitrate_table[4] = {
    [0] = &bitrate_table_version_2_25[0],
    [1] = NULL,
    [2] = &bitrate_table_version_2_25[0],
    [3] = &bitrate_table_version_1[0],
};

/* index : version index */
static const char *version_string[4] = {
    "MPEG Version 2.5", "reserved", "MPEG Version 2", "MPEG Version 1"
};

/* index : layer index */
static const char *layer_string[4] = {
    "reserved", "Layer III", "Layer II", "Layer I"
};

/* index : crc index */
static const char *protection_string[2] = {
    "Protected by CRC", "Not protected"
};

/* index : padding bit */
static const char *padding_string[2] = {
    "frame is not padded", "frame is padded with one extra slot"
};

/* index : channel mode */
static const char *channel_mode_string[4] = {
    "Stereo", "Joint Stereo (Stereo)", "Dual Channel (2 mono channels)",
    "Single Channel (Mono)"
};

/* index : layer index, mode extension */
static const char *mode_extention_string[4][4] = {
    [0] = {NULL, NULL, NULL, NULL},
    [1] = {"intensity:off, MS stereo:off", "intensity:on, MS stereo:off",
           "intensity:off, MS stereo:on", "intensity:on, MS stereo:on"},
    [2] = {"bands 4 to 31", "bands 8 to 31",
           "bands 12 to 31", "bands 16 to 31"},
    [3] = { "bands 4 to 31", "bands 8 to 31",
            "bands 12 to 31", "bands 16 to 31"},
};

/* index : copyright bit */
static const char *copyright_string[2] = {
    "Audio is not copyrighted", "Audio is copyrighted"
};

/* index : original bit */
static const char *original_string[2] = {
    "Copy of original media", "Original media"
};

/* index : emphasis */
static const char *emphasis_string[4] = {
    "none", "50/15 ms", "reserved", "CCIT J.17"
};

int mp3_header_parse(const unsigned char *buffer,
                     int *version, int *layer, int *crc, int *bitrate,
                     int *frequency, int *channel, int *mode_extension)
{
    const unsigned char *p = buffer;
    struct mp3_frame_header_s header;
    unsigned int version_index, layer_index, bitrate_index, samplingrate_index;
    const int *psampling_rate_table;
    const int *pbitrate_table, **ppbitrate_table;

    if (!p || !(p + 1) || !(p + 2) || !(p + 3))
        return -1;

    if (!version || !layer || !crc || !bitrate || !frequency ||
        !channel || !mode_extension)
        return -1;

    header.h0 = *(p + 3);
    header.h1 = *(p + 2);
    header.h2 = *(p + 1);
    header.h3 = *(p + 0);

    if (header.sync != 0x7ff)
        return -1;

    version_index = header.version_index;
    layer_index = header.layer_index;
    bitrate_index = header.bitrate_index;
    samplingrate_index = header.samplingrate_index;

    if ((version_index > 0x3) || (version_index == 0x1))
        return -1;

    if (layer_index > 0x3 || !layer_index)
        return -1;

    if (bitrate_index > 0xe)
        return -1;

    if (samplingrate_index > 0x2)
        return -1;

    psampling_rate_table = sampling_rate_table[version_index];

    ppbitrate_table = bitrate_table[version_index];
    pbitrate_table = ppbitrate_table[layer_index];

    *version = version_index;
    *layer = layer_index;
    *crc = header.protected;
    *bitrate = pbitrate_table[bitrate_index];
    *frequency = psampling_rate_table[samplingrate_index];
    *channel = header.channel_mode;
    *mode_extension = header.mode_extension;

    LOGV("mp3 frame header\n");
    LOGV("  sync: 0x%x\n", header.sync);
    LOGV("  version: 0x%x, %s\n", header.version_index,
         version_string[header.version_index]);
    LOGV("  layer: 0x%x, %s\n", header.layer_index,
         layer_string[header.layer_index]);
    LOGV("  protection: 0x%x, %s\n", header.protected,
         protection_string[header.protected]);
    LOGV("  bitrate: 0x%x, %u\n", header.bitrate_index, *bitrate);
    LOGV("  sampling rate: 0x%x, %u\n", header.samplingrate_index, *frequency);
    LOGV("  padding bit: 0x%x, %s\n", header.padding_bit,
         padding_string[header.padding_bit]);
    LOGV("  private bit: 0x%x\n", header.private_bit);
    LOGV("  channel mode: 0x%x, %s\n", header.channel_mode,
         channel_mode_string[header.channel_mode]);
    LOGV("  mode extension: 0x%x, %s\n", header.mode_extension,
         mode_extention_string[header.layer_index][header.mode_extension]);
    LOGV("  copyright: 0x%x, %s\n", header.copyright,
         copyright_string[header.copyright]);
    LOGV("  original: 0x%x, %s\n", header.original,
         original_string[header.original]);
    LOGV("  emphasis: 0x%x, %s\n", header.emphasis,
         emphasis_string[header.emphasis]);

    return 0;
}

/* end of MP3 */
