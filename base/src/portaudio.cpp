/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portaudio.h>

#define LOG_TAG "portaudio"
#include <log.h>

PortAudio::PortAudio()
{
    memset(&audioparam, 0, sizeof(audioparam));
    ComponentBase::SetTypeHeader(&audioparam, sizeof(audioparam));
}

OMX_ERRORTYPE PortAudio::SetPortAudioParam(
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *p, bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (audioparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        audioparam.nPortIndex = p->nPortIndex;

    audioparam.nIndex = p->nIndex;
    audioparam.eEncoding = p->eEncoding;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_PORTFORMATTYPE *PortAudio::GetPortAudioParam(void)
{
    return &audioparam;
}

/* end of PortAudio */

PortMp3::PortMp3()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingMP3;
    SetPortAudioParam(&audioparam, false);

    memset(&mp3param, 0, sizeof(mp3param));
    ComponentBase::SetTypeHeader(&mp3param, sizeof(mp3param));
}

OMX_ERRORTYPE PortMp3::SetPortMp3Param(const OMX_AUDIO_PARAM_MP3TYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (mp3param.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        mp3param.nPortIndex = p->nPortIndex;
    }

    mp3param.nChannels = p->nChannels;
    mp3param.nBitRate = p->nBitRate;
    mp3param.nSampleRate = p->nSampleRate;
    mp3param.nAudioBandWidth = p->nAudioBandWidth;
    mp3param.eChannelMode = p->eChannelMode;
    mp3param.eFormat = p->eFormat;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_MP3TYPE *PortMp3::GetPortMp3Param(void)
{
    return &mp3param;
}

/* end of PortMp3 */

PortPcm::PortPcm()
{
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
    audioparam.eEncoding = OMX_AUDIO_CodingPCM;
    SetPortAudioParam(&audioparam, false);

    memset(&pcmparam, 0, sizeof(pcmparam));
    ComponentBase::SetTypeHeader(&pcmparam, sizeof(pcmparam));
}

OMX_ERRORTYPE PortPcm::SetPortPcmParam(const OMX_AUDIO_PARAM_PCMMODETYPE *p,
                                       bool overwrite_readonly)
{
    if (!overwrite_readonly) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (pcmparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else {
        OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

        memcpy(&audioparam, GetPortAudioParam(), sizeof(audioparam));
        audioparam.nPortIndex = p->nPortIndex;
        SetPortAudioParam(&audioparam, true);

        pcmparam.nPortIndex = p->nPortIndex;
    }

    pcmparam.nChannels = p->nChannels;
    pcmparam.eNumData = p->eNumData;
    pcmparam.eEndian = p->eEndian;
    pcmparam.bInterleaved = p->bInterleaved;
    pcmparam.nBitPerSample = p->nBitPerSample;
    pcmparam.nSamplingRate = p->nSamplingRate;
    pcmparam.ePCMMode = p->ePCMMode;
    memcpy(&pcmparam.eChannelMapping[0], &p->eChannelMapping[0],
           sizeof(OMX_U32) * p->nChannels);

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_PCMMODETYPE *PortPcm::GetPortPcmParam(void)
{
    return &pcmparam;
}

/* end of PortPcm */
