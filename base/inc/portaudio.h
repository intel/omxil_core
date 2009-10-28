/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __PORTAUDIO_H
#define __PORTAUDIO_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortAudio : public PortBase
{
public:
    PortAudio();

    OMX_ERRORTYPE SetPortAudioParam(
        const OMX_AUDIO_PARAM_PORTFORMATTYPE *audioparam,
        bool overwrite_readonly);
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *GetPortAudioParam(void);

private:
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;
};

/* end of PortAudio */

class PortMp3 : public PortAudio
{
public:
    PortMp3();

    OMX_ERRORTYPE SetPortMp3Param(const OMX_AUDIO_PARAM_MP3TYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_MP3TYPE *GetPortMp3Param(void);

private:
    OMX_AUDIO_PARAM_MP3TYPE mp3param;
};

/* end of PortMp3 */

class PortAac : public PortAudio
{
public:
    PortAac();

    OMX_ERRORTYPE SetPortAacParam(const OMX_AUDIO_PARAM_AACPROFILETYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_AACPROFILETYPE *GetPortAacParam(void);

private:
    OMX_AUDIO_PARAM_AACPROFILETYPE aacparam;
};

/* end of PortAac */

class PortWma : public PortAudio
{
public:
    PortWma();

    OMX_ERRORTYPE SetPortWmaParam(const OMX_AUDIO_PARAM_WMATYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_WMATYPE *GetPortWmaParam(void);

private:
    OMX_AUDIO_PARAM_WMATYPE wmaparam;
};

/* end of PortWma */

class PortPcm : public PortAudio
{
public:
    PortPcm();

    OMX_ERRORTYPE SetPortPcmParam(const OMX_AUDIO_PARAM_PCMMODETYPE *p,
                                  bool overwrite_readonly);
    const OMX_AUDIO_PARAM_PCMMODETYPE*GetPortPcmParam(void);

private:
    OMX_AUDIO_PARAM_PCMMODETYPE pcmparam;
};

/* end of PortPcm */

#endif /* __PORTAUDIO_H */
