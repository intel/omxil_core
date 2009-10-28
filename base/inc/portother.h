/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#ifndef __PORTOTHER_H
#define __PORTOTHER_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

class PortOther : public PortBase
{
public:
    PortOther();

    OMX_ERRORTYPE SetPortOtherParam(
        const OMX_OTHER_PARAM_PORTFORMATTYPE *otherparam, bool internal);
    const OMX_OTHER_PARAM_PORTFORMATTYPE *GetPortOtherParam(void);

private:
    OMX_OTHER_PARAM_PORTFORMATTYPE otherparam;
};

/* end of PortOther */

#endif /* __PORTOTHER_H */
