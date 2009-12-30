/*
 * Copyright (c) 2009 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify, or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>
#include <componentbase.h>

#define LOG_NDEBUG 1

#define LOG_TAG "portbase"
#include <log.h>

/*
 * constructor & destructor
 */
void PortBase::__PortBase(void)
{
    buffer_hdrs = NULL;
    nr_buffer_hdrs = 0;
    buffer_hdrs_completion = false;

    pthread_mutex_init(&hdrs_lock, NULL);
    pthread_cond_init(&hdrs_wait, NULL);

    __queue_init(&bufferq);
    pthread_mutex_init(&bufferq_lock, NULL);

    __queue_init(&retainedbufferq);
    pthread_mutex_init(&retainedbufferq_lock, NULL);

    __queue_init(&markq);
    pthread_mutex_init(&markq_lock, NULL);

    state = OMX_PortEnabled;
    pthread_mutex_init(&state_lock, NULL);

    memset(&portdefinition, 0, sizeof(portdefinition));
    ComponentBase::SetTypeHeader(&portdefinition, sizeof(portdefinition));
    memset(definition_format_mimetype, 0, OMX_MAX_STRINGNAME_SIZE);
    portdefinition.format.audio.cMIMEType = &definition_format_mimetype[0];
    portdefinition.format.video.cMIMEType = &definition_format_mimetype[0];
    portdefinition.format.image.cMIMEType = &definition_format_mimetype[0];

    memset(&audioparam, 0, sizeof(audioparam));

    owner = NULL;
    appdata = NULL;
    callbacks = NULL;
}

PortBase::PortBase()
{
    __PortBase();
}

PortBase::PortBase(const OMX_PARAM_PORTDEFINITIONTYPE *portdefinition)
{
    __PortBase();
    SetPortDefinition(portdefinition, true);
}

PortBase::~PortBase()
{
    struct list *entry, *temp;

    /* should've been already freed at FreeBuffer() */
    list_foreach_safe(buffer_hdrs, entry, temp) {
        free(entry->data); /* OMX_BUFFERHEADERTYPE */
        __list_delete(buffer_hdrs, entry);
    }

    pthread_cond_destroy(&hdrs_wait);
    pthread_mutex_destroy(&hdrs_lock);

    /* should've been already freed at buffer processing */
    queue_free_all(&bufferq);
    pthread_mutex_destroy(&bufferq_lock);

    /* should've been already freed at buffer processing */
    queue_free_all(&retainedbufferq);
    pthread_mutex_destroy(&retainedbufferq_lock);

    /* should've been already empty in PushThisBuffer () */
    queue_free_all(&markq);
    pthread_mutex_destroy(&markq_lock);

    pthread_mutex_destroy(&state_lock);
}

/* end of constructor & destructor */

/*
 * accessor
 */
/* owner */
void PortBase::SetOwner(OMX_COMPONENTTYPE *handle)
{
    owner = handle;
}

OMX_COMPONENTTYPE *PortBase::GetOwner(void)
{
    return owner;
}

OMX_ERRORTYPE PortBase::SetCallbacks(OMX_HANDLETYPE hComponent,
                                     OMX_CALLBACKTYPE *pCallbacks,
                                     OMX_PTR pAppData)
{
    if (owner != hComponent)
        return OMX_ErrorBadParameter;

    appdata = pAppData;
    callbacks = pCallbacks;

    return OMX_ErrorNone;
}

/* end of accessor */

/*
 * component methods & helpers
 */
/* Get/SetParameter */
OMX_ERRORTYPE PortBase::SetPortDefinition(
    const OMX_PARAM_PORTDEFINITIONTYPE *p, bool overwrite_readonly)
{
    OMX_PARAM_PORTDEFINITIONTYPE temp;

    memcpy(&temp, &portdefinition, sizeof(temp));

    if (!overwrite_readonly) {
        if (temp.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadParameter;
        if (temp.eDir != p->eDir)
            return OMX_ErrorBadParameter;
        if (temp.eDomain != p->eDomain)
            return OMX_ErrorBadParameter;
        if (temp.nBufferCountActual != p->nBufferCountActual) {
            if (temp.nBufferCountMin > p->nBufferCountActual)
                return OMX_ErrorBadParameter;

            temp.nBufferCountActual = p->nBufferCountActual;
        }
    }
    else {
        temp.nPortIndex = p->nPortIndex;
        temp.eDir = p->eDir;
        temp.nBufferCountActual = p->nBufferCountActual;
        temp.nBufferCountMin = p->nBufferCountMin;
        temp.nBufferSize = p->nBufferSize;
        temp.bEnabled = p->bEnabled;
        temp.bPopulated = p->bPopulated;
        temp.eDomain = p->eDomain;
        temp.bBuffersContiguous = p->bBuffersContiguous;
        temp.nBufferAlignment = p->nBufferAlignment;
    }

    switch (p->eDomain) {
    case OMX_PortDomainAudio: {
        OMX_AUDIO_PORTDEFINITIONTYPE *format = &temp.format.audio;
        const OMX_AUDIO_PORTDEFINITIONTYPE *pformat = &p->format.audio;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
            mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->pNativeRender = pformat->pNativeRender;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eEncoding = pformat->eEncoding;

        break;
    }
    case OMX_PortDomainVideo: {
        OMX_VIDEO_PORTDEFINITIONTYPE *format = &temp.format.video;
        const OMX_VIDEO_PORTDEFINITIONTYPE *pformat = &p->format.video;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
            mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->pNativeRender = pformat->pNativeRender;
        format->nFrameWidth = pformat->nFrameWidth;
        format->nFrameHeight = pformat->nFrameHeight;
        format->nBitrate = pformat->nBitrate;
        format->xFramerate = pformat->xFramerate;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eCompressionFormat = pformat->eCompressionFormat;
        format->eColorFormat = pformat->eColorFormat;
        format->pNativeWindow = pformat->pNativeWindow;

        if (overwrite_readonly) {
            format->nStride = pformat->nStride;
            format->nSliceHeight = pformat->nSliceHeight;
        }

        break;
    }
    case OMX_PortDomainImage: {
        OMX_IMAGE_PORTDEFINITIONTYPE *format = &temp.format.image;
        const OMX_IMAGE_PORTDEFINITIONTYPE *pformat = &p->format.image;
        OMX_U32 mimetype_len = strlen(pformat->cMIMEType);

        mimetype_len = OMX_MAX_STRINGNAME_SIZE-1 > mimetype_len ?
            mimetype_len : OMX_MAX_STRINGNAME_SIZE-1;

        strncpy(format->cMIMEType, pformat->cMIMEType,
                mimetype_len+1);
        format->cMIMEType[mimetype_len+1] = '\0';
        format->nFrameWidth = pformat->nFrameWidth;
        format->nFrameHeight = pformat->nFrameHeight;
        format->nStride = pformat->nStride;
        format->bFlagErrorConcealment = pformat->bFlagErrorConcealment;
        format->eCompressionFormat = pformat->eCompressionFormat;
        format->eColorFormat = pformat->eColorFormat;
        format->pNativeWindow = pformat->pNativeWindow;

        if (overwrite_readonly)
            format->nSliceHeight = pformat->nSliceHeight;

        break;
    }
    case OMX_PortDomainOther: {
        OMX_OTHER_PORTDEFINITIONTYPE *format = &temp.format.other;
        const OMX_OTHER_PORTDEFINITIONTYPE *pformat = &p->format.other;

        format->eFormat = pformat->eFormat;
        break;
    }
    default:
        LOGE("cannot find 0x%08x port domain\n", p->eDomain);
        return OMX_ErrorBadParameter;
    }

    memcpy(&portdefinition, &temp, sizeof(temp));
    return OMX_ErrorNone;
}

const OMX_PARAM_PORTDEFINITIONTYPE *PortBase::GetPortDefinition(void)
{
    return &portdefinition;
}

/* Use/Allocate/FreeBuffer */
OMX_ERRORTYPE PortBase::UseBuffer(OMX_BUFFERHEADERTYPE **ppBufferHdr,
                                  OMX_U32 nPortIndex,
                                  OMX_PTR pAppPrivate,
                                  OMX_U32 nSizeBytes,
                                  OMX_U8 *pBuffer)
{
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    LOGV("%s(): enter, nPortIndex=%lu, nSizeBytes=%lu\n", __func__,
         nPortIndex, nSizeBytes);

    pthread_mutex_lock(&hdrs_lock);

    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGV("%s(): exit, already populated (ret = 0x%08x)\n", __func__,
             OMX_ErrorNone);
        return OMX_ErrorNone;
    }

    buffer_hdr = (OMX_BUFFERHEADERTYPE *)calloc(1, sizeof(*buffer_hdr));
    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit (ret = 0x%08x)\n",
             __func__, OMX_ErrorInsufficientResources);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit (ret = 0x%08x)\n",
             __func__, OMX_ErrorInsufficientResources);
        return OMX_ErrorInsufficientResources;
    }

    ComponentBase::SetTypeHeader(buffer_hdr, sizeof(*buffer_hdr));
    buffer_hdr->pBuffer = pBuffer;
    buffer_hdr->nAllocLen = nSizeBytes;
    buffer_hdr->pAppPrivate = pAppPrivate;
    if (portdefinition.eDir == OMX_DirInput) {
        buffer_hdr->nInputPortIndex = nPortIndex;
        buffer_hdr->nOutputPortIndex = 0x7fffffff;
        buffer_hdr->pInputPortPrivate = this;
        buffer_hdr->pOutputPortPrivate = NULL;
    }
    else {
        buffer_hdr->nOutputPortIndex = nPortIndex;
        buffer_hdr->nInputPortIndex = 0x7fffffff;
        buffer_hdr->pOutputPortPrivate = this;
        buffer_hdr->pInputPortPrivate = NULL;
    }

    buffer_hdrs = __list_add_tail(buffer_hdrs, entry);
    nr_buffer_hdrs++;

    LOGV("%s(): a buffer allocated (%lu/%lu)\n", __func__,
         nr_buffer_hdrs, portdefinition.nBufferCountActual);

    if (nr_buffer_hdrs >= portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): allocate all buffers, nBufferCountActual (%lu)\n",
             __func__, portdefinition.nBufferCountActual);
    }

    *ppBufferHdr = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);
    LOGV("%s(): exit (ret = 0x%08x)\n", __func__, OMX_ErrorNone);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::AllocateBuffer(OMX_BUFFERHEADERTYPE **ppBuffer,
                                       OMX_U32 nPortIndex,
                                       OMX_PTR pAppPrivate,
                                       OMX_U32 nSizeBytes)
{
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    LOGV("%s(): enter, nPortIndex=%lu, nSizeBytes=%lu\n", __func__,
         nPortIndex, nSizeBytes);

    pthread_mutex_lock(&hdrs_lock);
    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGV("%s(): exit, already populated (ret = 0x%08x)\n", __func__,
             OMX_ErrorNone);
        return OMX_ErrorNone;
    }

    buffer_hdr = (OMX_BUFFERHEADERTYPE *)
        calloc(1, sizeof(*buffer_hdr) + nSizeBytes);
    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit (ret = 0x%08x)\n",
             __func__, OMX_ErrorInsufficientResources);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit (ret = 0x%08x)\n",
             __func__, OMX_ErrorInsufficientResources);
        return OMX_ErrorInsufficientResources;
    }

    ComponentBase::SetTypeHeader(buffer_hdr, sizeof(*buffer_hdr));
    buffer_hdr->pBuffer = (OMX_U8 *)buffer_hdr + sizeof(*buffer_hdr);
    buffer_hdr->nAllocLen = nSizeBytes;
    buffer_hdr->pAppPrivate = pAppPrivate;
    if (portdefinition.eDir == OMX_DirInput) {
        buffer_hdr->nInputPortIndex = nPortIndex;
        buffer_hdr->nOutputPortIndex = (OMX_U32)-1;
        buffer_hdr->pInputPortPrivate = this;
        buffer_hdr->pOutputPortPrivate = NULL;
    }
    else {
        buffer_hdr->nOutputPortIndex = nPortIndex;
        buffer_hdr->nInputPortIndex = (OMX_U32)-1;
        buffer_hdr->pOutputPortPrivate = this;
        buffer_hdr->pInputPortPrivate = NULL;
    }

    buffer_hdrs = __list_add_tail(buffer_hdrs, entry);
    nr_buffer_hdrs++;

    LOGV("%s(): a buffer allocated (%lu/%lu)\n", __func__,
         nr_buffer_hdrs, portdefinition.nBufferCountActual);

    if (nr_buffer_hdrs == portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): allocate all buffers, nBufferCountActual (%lu)\n",
             __func__, portdefinition.nBufferCountActual);
    }

    *ppBuffer = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);
    LOGV("%s(): exit (ret = 0x%08x)\n", __func__, OMX_ErrorNone);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::FreeBuffer(OMX_U32 nPortIndex,
                                   OMX_BUFFERHEADERTYPE *pBuffer)
{
    struct list *entry;
    OMX_ERRORTYPE ret;

    LOGV("%s(): enter, nPortIndex=%lu\n", __func__, nPortIndex);

    pthread_mutex_lock(&hdrs_lock);
    entry = list_find(buffer_hdrs, pBuffer);

    if (!entry) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGV("%s(): exit (ret = 0x%08x)\n", __func__, OMX_ErrorNone);
        return OMX_ErrorNone;
    }

    if (entry->data != pBuffer) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit, (entry->data != pBuffer) (ret = 0x%08x)\n", __func__,
             OMX_ErrorBadParameter);
        return OMX_ErrorBadParameter;
    }

    ret = ComponentBase::CheckTypeHeader(pBuffer, sizeof(*pBuffer));
    if (ret != OMX_ErrorNone) {
        pthread_mutex_unlock(&hdrs_lock);
        LOGE("%s(): exit (ret = 0x%08x)\n",
             __func__, OMX_ErrorInsufficientResources);
        return ret;
    }

    buffer_hdrs = __list_delete(buffer_hdrs, entry);
    nr_buffer_hdrs--;

    LOGV("%s(): free a buffer (%lu/%lu)\n", __func__,
         nr_buffer_hdrs, portdefinition.nBufferCountActual);

    portdefinition.bPopulated = OMX_FALSE;
    if (!nr_buffer_hdrs) {
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
        LOGV("%s(): free all allocated buffers\n", __func__);
    }

    free(pBuffer);

    pthread_mutex_unlock(&hdrs_lock);
    LOGV("%s(): exit (ret = 0x%08x)\n", __func__, OMX_ErrorNone);
    return OMX_ErrorNone;
}

void PortBase::WaitPortBufferCompletion(void)
{
    pthread_mutex_lock(&hdrs_lock);
    if (!buffer_hdrs_completion) {
        LOGV("%s(): wait for buffer header completion\n", __func__);
        pthread_cond_wait(&hdrs_wait, &hdrs_lock);
        LOGV("%s(): wokeup (buffer header completion)\n", __func__);
    }
    buffer_hdrs_completion = !buffer_hdrs_completion;
    pthread_mutex_unlock(&hdrs_lock);
}

/* Empty/FillThisBuffer */
OMX_ERRORTYPE PortBase::PushThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer)
{
    int ret;

    pthread_mutex_lock(&bufferq_lock);
    ret = queue_push_tail(&bufferq, pBuffer);
    pthread_mutex_unlock(&bufferq_lock);

    if (ret)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}

OMX_BUFFERHEADERTYPE *PortBase::PopBuffer(void)
{
    OMX_BUFFERHEADERTYPE *buffer;

    pthread_mutex_lock(&bufferq_lock);
    buffer = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&bufferq);
    pthread_mutex_unlock(&bufferq_lock);

    return buffer;
}

OMX_U32 PortBase::BufferQueueLength(void)
{
    OMX_U32 length;

    pthread_mutex_lock(&bufferq_lock);
    length = queue_length(&bufferq);
    pthread_mutex_unlock(&bufferq_lock);

    return length;
}

OMX_ERRORTYPE PortBase::ReturnThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer)
{
    OMX_DIRTYPE direction = portdefinition.eDir;
    OMX_U32 port_index;
    OMX_ERRORTYPE (*bufferdone_callback)(OMX_HANDLETYPE,
                                         OMX_PTR,
                                         OMX_BUFFERHEADERTYPE *);
    OMX_ERRORTYPE ret;

    LOGV("%s(): enter\n", __func__);

    if (!pBuffer) {
        LOGE("%s(): exit, invalid buffer pointer (ret = 0x%08x)\n", __func__,
             OMX_ErrorBadParameter);
        return OMX_ErrorBadParameter;
    }

    if (direction == OMX_DirInput) {
        port_index = pBuffer->nInputPortIndex;
        bufferdone_callback = callbacks->EmptyBufferDone;
    }
    else if (direction == OMX_DirOutput) {
        port_index = pBuffer->nOutputPortIndex;
        bufferdone_callback = callbacks->FillBufferDone;
    }
    else {
        LOGE("%s(): exit, invalid direction (%d) (ret = 0x%08x)\n", __func__,
             direction, OMX_ErrorBadParameter);
        return OMX_ErrorBadParameter;
    }

    LOGV("%s(): direction=%d, port_index=%lu\n",
         __func__, direction, port_index);

    if (port_index != portdefinition.nPortIndex) {
        LOGE("%s(): exit, not matched port index (%lu/%lu) (ret = 0x%08x)\n",
             __func__, port_index, portdefinition.nPortIndex,
             OMX_ErrorBadParameter);
        return OMX_ErrorBadParameter;
    }

    if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS) {
        callbacks->EventHandler(owner, appdata,
                                OMX_EventBufferFlag,
                                port_index, pBuffer->nFlags, NULL);
    }

    if (pBuffer->hMarkTargetComponent == owner) {
        callbacks->EventHandler(owner, appdata, OMX_EventMark,
                                0, 0, pBuffer->pMarkData);
        pBuffer->hMarkTargetComponent = NULL;
        pBuffer->pMarkData = NULL;
    }

    ret = bufferdone_callback(owner, appdata, pBuffer);
    LOGV("%s(): exit, after calling bufferdone callback (ret = 0x%08x)\n",
         __func__, ret);
    return ret;
}

/* retain buffer */
OMX_ERRORTYPE PortBase::RetainThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer,
                                         bool accumulate)
{
    int ret;

    /* push at tail of retainedbufferq */
    if (accumulate == true) {
        /* do not accumulate a buffer set EOS flag */
        if (pBuffer->nFlags & OMX_BUFFERFLAG_EOS)
            return OMX_ErrorBadParameter;

        pthread_mutex_lock(&retainedbufferq_lock);
        if ((OMX_U32)queue_length(&retainedbufferq) <
            portdefinition.nBufferCountActual)
            ret = queue_push_tail(&retainedbufferq, pBuffer);
        else {
            ret = OMX_ErrorInsufficientResources;
            LOGE("%s(): retained bufferq length (%d) exceeds port's "
                 "nBufferCountActual (%lu)", __func__,
                 queue_length(&retainedbufferq),
                 portdefinition.nBufferCountActual);
        }
        pthread_mutex_unlock(&retainedbufferq_lock);
    }
    /*
     * just push at head of bufferq to get this buffer again in
     * ComponentBase::ProcessorProcess()
     */
    else {
        pthread_mutex_lock(&bufferq_lock);
        ret = queue_push_head(&bufferq, pBuffer);
        pthread_mutex_unlock(&bufferq_lock);
    }

    if (ret)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}

void PortBase::ReturnAllRetainedBuffers(void)
{
    OMX_BUFFERHEADERTYPE *buffer;
    OMX_ERRORTYPE ret;
    int i;

    pthread_mutex_lock(&retainedbufferq_lock);

    do {
        buffer = (OMX_BUFFERHEADERTYPE *)queue_pop_head(&retainedbufferq);

        if (buffer) {
            LOGV("%s(): returns a retained buffer (%lu / %d)\n", __func__,
                 i++, queue_length(&retainedbufferq));

            ret = ReturnThisBuffer(buffer);
            if (ret != OMX_ErrorNone)
                LOGE("%s(): ReturnThisBuffer failed (ret 0x%x08x)\n", __func__,
                     ret);
        }
    } while (buffer);

    LOGV("%s(): returned all retained buffers\n", __func__);
    pthread_mutex_unlock(&retainedbufferq_lock);
}

/* SendCommand:Flush/PortEnable/Disable */
/* must be held ComponentBase::ports_block */
OMX_ERRORTYPE PortBase::FlushPort(void)
{
    OMX_BUFFERHEADERTYPE *buffer;

    ReturnAllRetainedBuffers();

    while ((buffer = PopBuffer()))
        ReturnThisBuffer(buffer);

    return OMX_ErrorNone;
}

OMX_STATETYPE PortBase::GetOwnerState(void)
{
    OMX_STATETYPE state = OMX_StateInvalid;

    if (owner) {
        ComponentBase *cbase;
        cbase = static_cast<ComponentBase *>(owner->pComponentPrivate);
        if (!cbase)
            return state;

        cbase->CBaseGetState((void *)owner, &state);
    }

    return state;
}

bool PortBase::IsEnabled(void)
{
    bool enabled;
    bool unlock = true;

    if (pthread_mutex_trylock(&state_lock))
        unlock = false;

    enabled = (state == OMX_PortEnabled) ? true : false;

    if (unlock)
        pthread_mutex_unlock(&state_lock);

    return enabled;
}

OMX_DIRTYPE PortBase::GetPortDirection(void)
{
    return portdefinition.eDir;
}

OMX_U32 PortBase::GetPortBufferCount(void)
{
    return nr_buffer_hdrs;
}

OMX_ERRORTYPE PortBase::PushMark(OMX_MARKTYPE *mark)
{
    int ret;

    pthread_mutex_lock(&markq_lock);
    ret = queue_push_tail(&markq, mark);
    pthread_mutex_unlock(&markq_lock);

    if (ret)
        return OMX_ErrorInsufficientResources;

    return OMX_ErrorNone;
}

OMX_MARKTYPE *PortBase::PopMark(void)
{
    OMX_MARKTYPE *mark;

    pthread_mutex_lock(&markq_lock);
    mark = (OMX_MARKTYPE *)queue_pop_head(&markq);
    pthread_mutex_unlock(&markq_lock);

    return mark;
}

OMX_ERRORTYPE PortBase::TransState(OMX_U8 transition)
{
    OMX_U8 current;
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOGV("%s(): enter, transition=%d\n", __func__, transition);

    pthread_mutex_lock(&state_lock);

    current = state;

    if (current == transition) {
        ret = OMX_ErrorSameState;
        LOGE("%s(): exit, same state (%d) (ret = 0x%08x)\n",
             __func__, current, OMX_ErrorSameState);
        goto unlock;
    }

    if (transition == OMX_PortEnabled) {
        WaitPortBufferCompletion();
        portdefinition.bEnabled = OMX_TRUE;
    }
    else if(transition == OMX_PortDisabled) {
        FlushPort();
        WaitPortBufferCompletion();
        portdefinition.bEnabled = OMX_FALSE;
    }
    else {
        ret = OMX_ErrorBadParameter;
        LOGE("%s(): exit, invalid transition (%d) (ret = 0x%08x)\n",
             __func__, transition, OMX_ErrorSameState);
        goto unlock;
    }

    state = transition;
    LOGV("%s(): transition from %d to %d completed\n", __func__,
         current, transition);

unlock:
    pthread_mutex_unlock(&state_lock);
    return ret;
}

OMX_ERRORTYPE PortBase::ReportPortSettingsChanged(void)
{
    return callbacks->EventHandler(owner, appdata,
                                   OMX_EventPortSettingsChanged,
                                   portdefinition.nPortIndex, 0, NULL);
}

/* end of component methods & helpers */

/* end of PortBase */
