/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <portbase.h>
#include <componentbase.h>

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
    SetPortDefinition(portdefinition, false);
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
    const OMX_PARAM_PORTDEFINITIONTYPE *p, bool isclient)
{
    OMX_PARAM_PORTDEFINITIONTYPE temp;

    memcpy(&temp, &portdefinition, sizeof(temp));

    if (isclient) {
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

        if (!isclient) {
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

        if (!isclient)
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
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    pthread_mutex_lock(&hdrs_lock);

    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorNone;
    }

    buffer_hdr = (OMX_BUFFERHEADERTYPE *)calloc(1, sizeof(*buffer_hdr));
    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
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

    if (nr_buffer_hdrs >= portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
    }

    *ppBufferHdr = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::AllocateBuffer(OMX_BUFFERHEADERTYPE **ppBuffer,
                                       OMX_U32 nPortIndex,
                                       OMX_PTR pAppPrivate,
                                       OMX_U32 nSizeBytes)
{
    OMX_BUFFERHEADERTYPE *buffer_hdr;
    struct list *entry;

    pthread_mutex_lock(&hdrs_lock);
    if (portdefinition.bPopulated == OMX_TRUE) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorNone;
    }

    buffer_hdr = (OMX_BUFFERHEADERTYPE *)
        calloc(1, sizeof(*buffer_hdr) + nSizeBytes);
    if (!buffer_hdr) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorInsufficientResources;
    }

    entry = list_alloc(buffer_hdr);
    if (!entry) {
        free(buffer_hdr);
        pthread_mutex_unlock(&hdrs_lock);
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

    if (nr_buffer_hdrs == portdefinition.nBufferCountActual) {
        portdefinition.bPopulated = OMX_TRUE;
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
    }

    *ppBuffer = buffer_hdr;

    pthread_mutex_unlock(&hdrs_lock);
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PortBase::FreeBuffer(OMX_U32 nPortIndex,
                                   OMX_BUFFERHEADERTYPE *pBuffer)
{
    struct list *entry;
    OMX_ERRORTYPE ret;

    pthread_mutex_lock(&hdrs_lock);
    entry = list_find(buffer_hdrs, pBuffer);

    if (!entry) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorNone;
    }

    if (entry->data != pBuffer) {
        pthread_mutex_unlock(&hdrs_lock);
        return OMX_ErrorBadParameter;
    }

    ret = ComponentBase::CheckTypeHeader(pBuffer, sizeof(*pBuffer));
    if (ret != OMX_ErrorNone) {
        pthread_mutex_unlock(&hdrs_lock);
        return ret;
    }

    buffer_hdrs = __list_delete(buffer_hdrs, entry);
    nr_buffer_hdrs--;

    portdefinition.bPopulated = OMX_FALSE;
    if (!nr_buffer_hdrs) {
        buffer_hdrs_completion = true;
        pthread_cond_signal(&hdrs_wait);
    }

    free(pBuffer);

    pthread_mutex_unlock(&hdrs_lock);
    return OMX_ErrorNone;
}

void PortBase::WaitPortBufferCompletion(void)
{
    pthread_mutex_lock(&hdrs_lock);
    if (!buffer_hdrs_completion)
        pthread_cond_wait(&hdrs_wait, &hdrs_lock);
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

OMX_ERRORTYPE PortBase::RetainThisBuffer(OMX_BUFFERHEADERTYPE *pBuffer)
{
    int ret;

    pthread_mutex_lock(&bufferq_lock);
    ret = queue_push_head(&bufferq, pBuffer);
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

    if (!pBuffer)
        return OMX_ErrorBadParameter;

    if (direction == OMX_DirInput) {
        port_index = pBuffer->nInputPortIndex;
        bufferdone_callback = callbacks->EmptyBufferDone;
    }
    else if (direction == OMX_DirOutput) {
        port_index = pBuffer->nOutputPortIndex;
        bufferdone_callback = callbacks->FillBufferDone;
    }
    else
        return OMX_ErrorBadParameter;

    if (port_index != portdefinition.nPortIndex)
        return OMX_ErrorBadParameter;

    return bufferdone_callback(owner, appdata, pBuffer);
}

/* SendCommand:Flush/PortEnable/Disable */
/* must be held ComponentBase::ports_block */
OMX_ERRORTYPE PortBase::FlushPort(void)
{
    OMX_BUFFERHEADERTYPE *buffer;

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

    pthread_mutex_lock(&state_lock);
    enabled = (state == OMX_PortEnabled) ? true : false;
    pthread_mutex_unlock(&state_lock);

    return enabled;
}

OMX_DIRTYPE PortBase::GetPortDirection(void)
{
    return portdefinition.eDir;
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

OMX_ERRORTYPE PortBase::TransState(OMX_U8 state)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    pthread_mutex_lock(&state_lock);

    if (this->state == state) {
        ret = OMX_ErrorSameState;
        goto unlock;
    }

    if (state == OMX_PortEnabled) {
        WaitPortBufferCompletion();
        portdefinition.bEnabled = OMX_TRUE;
    }
    else if(state == OMX_PortDisabled) {
        FlushPort();
        WaitPortBufferCompletion();
        portdefinition.bEnabled = OMX_FALSE;
    }
    else {
        ret = OMX_ErrorBadParameter;
        goto unlock;
    }

    this->state = state;

unlock:
    pthread_mutex_unlock(&state_lock);
    return ret;
}

/* end of component methods & helpers */

/* end of PortBase */

PortAudio::PortAudio()
{
    memset(&audioparam, 0, sizeof(audioparam));
    ComponentBase::SetTypeHeader(&audioparam, sizeof(audioparam));
}

OMX_ERRORTYPE PortAudio::SetPortAudioParam(
    const OMX_AUDIO_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (audioparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        audioparam.nPortIndex = p->nPortIndex;

    audioparam.nIndex = p->nIndex;
    audioparam.eEncoding = p->eEncoding;

    return OMX_ErrorNone;
}

const OMX_AUDIO_PARAM_PORTFORMATTYPE *PortAudio::GetPortAudioParam(void)
{
    return &audioparam;
}

/* end of PortAudio */

PortVideo::PortVideo()
{
    memset(&videoparam, 0, sizeof(videoparam));
    ComponentBase::SetTypeHeader(&videoparam, sizeof(videoparam));
}

OMX_ERRORTYPE PortVideo::SetPortVideoParam(
    const OMX_VIDEO_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (videoparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        videoparam.nPortIndex = p->nPortIndex;

    videoparam.nIndex = p->nIndex;
    videoparam.eCompressionFormat = p->eCompressionFormat;
    videoparam.eColorFormat = p->eColorFormat;
    videoparam.xFramerate = p->xFramerate;

    return OMX_ErrorNone;
}

const OMX_VIDEO_PARAM_PORTFORMATTYPE *PortVideo::GetPortVideoParam(void)
{
    return &videoparam;
}

/* end of PortVideo */

PortImage::PortImage()
{
    memset(&imageparam, 0, sizeof(imageparam));
    ComponentBase::SetTypeHeader(&imageparam, sizeof(imageparam));
}

OMX_ERRORTYPE PortImage::SetPortImageParam(
    const OMX_IMAGE_PARAM_PORTFORMATTYPE *p, bool internal)
{
    if (!internal) {
        OMX_ERRORTYPE ret;

        ret = ComponentBase::CheckTypeHeader((void *)p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;
        if (imageparam.nPortIndex != p->nPortIndex)
            return OMX_ErrorBadPortIndex;
    }
    else
        imageparam.nPortIndex = p->nPortIndex;

    imageparam.nIndex = p->nIndex;
    imageparam.eCompressionFormat = p->eCompressionFormat;
    imageparam.eColorFormat = p->eColorFormat;

    return OMX_ErrorNone;
}

const OMX_IMAGE_PARAM_PORTFORMATTYPE *PortImage::GetPortImageParam(void)
{
    return &imageparam;
}

/* end of PortImage */

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
