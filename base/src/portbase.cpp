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

/* Use/Allocate/FreeBuffer */
OMX_ERRORTYPE PortBase::UseBuffer(OMX_BUFFERHEADERTYPE **ppBufferHdr,
                                  OMX_U32 nPortIndex,
                                  OMX_PTR pAppPrivate,
                                  OMX_U32 nSizeBytes,
                                  OMX_U8 *pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE PortBase::AllocateBuffer(OMX_BUFFERHEADERTYPE **ppBuffer,
                                       OMX_U32 nPortIndex,
                                       OMX_PTR pAppPrivate,
                                       OMX_U32 nSizeBytes)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE PortBase::FreeBuffer(OMX_U32 nPortIndex,
                                   OMX_BUFFERHEADERTYPE *pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

/* end of component methods & helpers */

/* end of PortBase */
