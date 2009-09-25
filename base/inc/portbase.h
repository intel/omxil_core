/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __PORTBASE_H
#define __PORTBASE_H

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <list.h>

class PortBase
{
public:
    /*
     * constructor & destructor
     */
    PortBase();
    PortBase(const OMX_PARAM_PORTDEFINITIONTYPE *param);
    virtual ~PortBase();

    /* end of constructor & destructor */

    /*
     * component methods & helpers
     */
    /* Get/SetParameter */
    void SetPortParam(
        const OMX_PARAM_PORTDEFINITIONTYPE *pComponentParameterStructure);
    const OMX_PARAM_PORTDEFINITIONTYPE *GetPortParam(void);
    /* audio parameter */
    void SetAudioPortParam(
        const OMX_AUDIO_PARAM_PORTFORMATTYPE *pComponentParameterStructure);
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *GetAudioPortParam(void);

    /* Use/Allocate/FreeBuffer */
    OMX_ERRORTYPE UseBuffer(OMX_BUFFERHEADERTYPE **ppBufferHdr,
                            OMX_U32 nPortIndex,
                            OMX_PTR pAppPrivate,
                            OMX_U32 nSizeBytes,
                            OMX_U8 *pBuffer);
    OMX_ERRORTYPE AllocateBuffer(OMX_BUFFERHEADERTYPE **ppBuffer,
                                 OMX_U32 nPortIndex,
                                 OMX_PTR pAppPrivate,
                                 OMX_U32 nSizeBytes);
    OMX_ERRORTYPE FreeBuffer(OMX_U32 nPortIndex,
                             OMX_BUFFERHEADERTYPE *pBuffer);

    /* end of component methods & helpers */

private:
    /* common routines for constructor */
    void __PortBase(void);

    /* buffer headers */
    struct list *buffer_hdrs;
    OMX_U32 nr_buffer_hdrs;
    bool buffer_hdrs_completion; /* Use/Allocate/FreeBuffer completion flag */
    pthread_mutex_t hdrs_lock;
    pthread_cond_t hdrs_wait;

    /* parameter */
    OMX_PARAM_PORTDEFINITIONTYPE portparam;
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;
};

#endif /* __PORTBASE_H */
