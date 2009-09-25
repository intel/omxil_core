/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>

/*
 * constructor & destructor
 */
void PortBase::__PortBase(void)
{
    memset(&portparam, 0, sizeof(portparam));
    memset(&audioparam, 0, sizeof(audioparam));
}

PortBase::PortBase()
{
    __PortBase();
}

PortBase::~PortBase()
{
    /*
     * Todo
     */
}

/* end of constructor & destructor */

/*
 * component methods & helpers
 */
/* Get/SetParameter */
void PortBase::SetPortParam(
    const OMX_PARAM_PORTDEFINITIONTYPE *pComponentParameterStructure)
{
    memcpy(&portparam, pComponentParameterStructure, sizeof(portparam));
}

const OMX_PARAM_PORTDEFINITIONTYPE *PortBase::GetPortParam(void)
{
    return &portparam;
}

/* audio parameter */
void PortBase::SetAudioPortParam(
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *pComponentParameterStructure)
{
    memcpy(&audioparam, pComponentParameterStructure, sizeof(audioparam));
}

const OMX_AUDIO_PARAM_PORTFORMATTYPE *PortBase::GetAudioPortParam(void)
{
    return &audioparam;
}

/* end of component methods & helpers */
