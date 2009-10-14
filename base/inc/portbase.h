/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __PORTBASE_H
#define __PORTBASE_H

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <list.h>
#include <queue.h>

class PortBase
{
public:
    /*
     * constructor & destructor
     */
    PortBase();
    PortBase(const OMX_PARAM_PORTDEFINITIONTYPE *portdefinition);
    virtual ~PortBase();

    /* end of constructor & destructor */

    /*
     * accessor
     */
    /* owner */
    void SetOwner(OMX_COMPONENTTYPE *handle);
    OMX_COMPONENTTYPE *GetOwner(void);

    /* for ReturnThisBuffer() */
    OMX_ERRORTYPE SetCallbacks(OMX_HANDLETYPE hComponent,
                               OMX_CALLBACKTYPE *pCallbacks,
                               OMX_PTR pAppData);
    /* end of accessor */

    /*
     * component methods & helpers
     */
    /* Get/SetParameter */
    OMX_ERRORTYPE SetPortDefinition(const OMX_PARAM_PORTDEFINITIONTYPE *p,
                                    bool isclient);
    const OMX_PARAM_PORTDEFINITIONTYPE *GetPortDefinition(void);
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

    /*
     * called in ComponentBase::TransStateToLoaded(OMX_StateIdle) or
     * in ComponentBase::TransStateToIdle(OMX_StateLoaded)
     * wokeup by Use/Allocate/FreeBuffer
     */
    void WaitPortBufferCompletion(void);

    /* Empty/FillThisBuffer */
    OMX_ERRORTYPE PushThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer);
    /* push buffer at head */
    OMX_ERRORTYPE RetainThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer);
    OMX_BUFFERHEADERTYPE *PopBuffer(void);
    OMX_U32 BufferQueueLength(void);

    /* Empty/FillBufferDone */
    OMX_ERRORTYPE ReturnThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer);

    /* flush all buffers not under processing */
    OMX_ERRORTYPE FlushPort(void);

    bool IsEnabled(void);

    OMX_DIRTYPE GetPortDirection(void);

    OMX_ERRORTYPE PushMark(OMX_MARKTYPE *mark);
    OMX_MARKTYPE *PopMark(void);

    /* SendCommand(OMX_CommandPortDisable/Enable) */
    OMX_ERRORTYPE TransState(OMX_U8 state);

    /* end of component methods & helpers */

    /* TransState, state */
    static const OMX_U8 OMX_PortEnabled = 0;
    static const OMX_U8 OMX_PortDisabled = 1;

private:
    /* common routines for constructor */
    void __PortBase(void);

    /*
     * component methods & helpers
     */
    OMX_STATETYPE GetOwnerState(void);

    /* end of component methods & helpers */

    /* buffer headers */
    struct list *buffer_hdrs;
    OMX_U32 nr_buffer_hdrs;
    bool buffer_hdrs_completion; /* Use/Allocate/FreeBuffer completion flag */
    pthread_mutex_t hdrs_lock;
    pthread_cond_t hdrs_wait;

    struct queue bufferq;
    pthread_mutex_t bufferq_lock;

    struct queue markq;
    pthread_mutex_t markq_lock;

    /* state */
    OMX_U8 state;
    pthread_mutex_t state_lock;

    /* parameter */
    OMX_PARAM_PORTDEFINITIONTYPE portdefinition;
    /* room for portdefinition.format.*.cMIMEType */
    char definition_format_mimetype[OMX_MAX_STRINGNAME_SIZE];

    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;

    /* owner handle */
    OMX_COMPONENTTYPE *owner;

    /* omx standard callbacks */
    OMX_PTR appdata;
    OMX_CALLBACKTYPE *callbacks;
};

/* end of PortBase */

class PortAudio : public PortBase
{
public:
    PortAudio();

    OMX_ERRORTYPE SetPortAudioParam(
        const OMX_AUDIO_PARAM_PORTFORMATTYPE *audioparam, bool internal);
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *GetPortAudioParam(void);

private:
    OMX_AUDIO_PARAM_PORTFORMATTYPE audioparam;
};

/* end of PortAudio */

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

#endif /* __PORTBASE_H */
