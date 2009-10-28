/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __PORTIMAGE_H
#define __PORTIMAGE_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortImage : public PortBase
{
public:
    PortImage();

    OMX_ERRORTYPE SetPortImageParam(
        const OMX_IMAGE_PARAM_PORTFORMATTYPE *imageparam, bool internal);
    const OMX_IMAGE_PARAM_PORTFORMATTYPE *GetPortImageParam(void);

private:
    OMX_IMAGE_PARAM_PORTFORMATTYPE imageparam;
};

/* end of PortImage */

#endif /* __PORTIMAGE_H */
