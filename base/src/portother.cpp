/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>
#include <portother.h>

#define LOG_TAG "portother"
#include <log.h>

PortOther::PortOther()
{
    memset(&otherparam, 0, sizeof(otherparam));
    ComponentBase::SetTypeHeader(&otherparam, sizeof(otherparam));
}

OMX_ERRORTYPE PortOther::SetPortOtherParam(
    const OMX_OTHER_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;
        
        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (otherparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        otherparam.nPortIndex = p->nPortIndex;

    otherparam.nIndex = p->nIndex;
    otherparam.eFormat = p->eFormat;

    return OMX_ErrorNone;
}

const OMX_OTHER_PARAM_PORTFORMATTYPE *PortOther::GetPortOtherParam(void)
{
    return &otherparam;
}

/* end of PortOther */
