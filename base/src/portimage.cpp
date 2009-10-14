/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portimage.h>

#define LOG_TAG "portimage"
#include <log.h>

PortImage::PortImage()
{
    memset(&imageparam, 0, sizeof(imageparam));
    ComponentBase::SetTypeHeader(&imageparam, sizeof(imageparam));
}

OMX_ERRORTYPE PortImage::SetPortImageParam(
    const OMX_IMAGE_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (imageparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        imageparam.nPortIndex = p->nPortIndex;

    imageparam.nIndex = p->nIndex;
    imageparam.eCompressionFormat = p->eCompressionFormat;
    imageparam.eColorFormat = p->eColorFormat;

    return OMX_ErrorNone;
}

const OMX_IMAGE_PARAM_PORTFORMATTYPE *PortImage::GetPortImageParam(void)
{
    return &imageparam;
}

/* end of PortImage */
