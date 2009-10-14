/*
 * Copyright (C) 2009 Wind River Systems.
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

#endif /* __PORTVIDEO_H */
