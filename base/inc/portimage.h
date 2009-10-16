/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
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
