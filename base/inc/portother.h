/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
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
