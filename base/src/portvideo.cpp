/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portvideo.h>

#define LOG_TAG "portvideo"
#include <log.h>

PortVideo::PortVideo()
{
    memset(&videoparam, 0, sizeof(videoparam));
    ComponentBase::SetTypeHeader(&videoparam, sizeof(videoparam));
}

OMX_ERRORTYPE PortVideo::SetPortVideoParam(
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (videoparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        videoparam.nPortIndex = p->nPortIndex;

    videoparam.nIndex = p->nIndex;
    videoparam.eCompressionFormat = p->eCompressionFormat;
    videoparam.eColorFormat = p->eColorFormat;
    videoparam.xFramerate = p->xFramerate;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_PORTFORMATTYPE *PortVideo::GetPortVideoParam(void)
{
    return &videoparam;
}

/* end of PortVideo */
