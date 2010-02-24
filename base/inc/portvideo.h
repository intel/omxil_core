/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __PORTVIDEO_H
#define __PORTVIDEO_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortVideo : public PortBase
{
public:
    PortVideo();

    OMX_ERRORTYPE SetPortVideoParam(
        const OMX_VIDEO_PARAM_PORTFORMATTYPE *videoparam, bool internal);
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *GetPortVideoParam(void);

private:
    OMX_VIDEO_PARAM_PORTFORMATTYPE videoparam;
};

/* end of PortVideo */

class PortAvc : public PortVideo
{
public:
    PortAvc();

    OMX_ERRORTYPE SetPortAvcParam(const OMX_VIDEO_PARAM_AVCTYPE *p,
                                  bool overwrite_readonly);
    const OMX_VIDEO_PARAM_AVCTYPE *GetPortAvcParam(void);

private:
    OMX_VIDEO_PARAM_AVCTYPE avcparam;
};

/* end of PortAvc */
class PortMpeg4 : public PortVideo
{
public:
    PortMpeg4();

    OMX_ERRORTYPE SetPortMpeg4Param(const OMX_VIDEO_PARAM_MPEG4TYPE *p,
                                  bool overwrite_readonly);
    const OMX_VIDEO_PARAM_MPEG4TYPE *GetPortMpeg4Param(void);

private:
    OMX_VIDEO_PARAM_MPEG4TYPE mpeg4param;
};

/* end of PortMpeg4 */

#endif /* __PORTVIDEO_H */
