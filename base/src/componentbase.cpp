/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>

#include <queue.h>
#include <workqueue.h>

#define LOG_TAG "componentbase"
#include <log.h>

/*
 * CmdProcessWork
 */
CmdProcessWork::CmdProcessWork(CmdHandlerInterface *ci)
{
    this->ci = ci;

    workq = new WorkQueue;

    __queue_init(&q);
    pthread_mutex_init(&lock, NULL);

    workq->StartWork();
}

CmdProcessWork::~CmdProcessWork()
{
    workq->StopWork();
    delete workq;

    pthread_mutex_lock(&lock);
    queue_free_all(&q);
    pthread_mutex_unlock(&lock);

    pthread_mutex_destroy(&lock);
}

OMX_ERRORTYPE CmdProcessWork::PushCmdQueue(struct cmd_s *cmd)
{
    int ret;

    pthread_mutex_lock(&lock);
    ret = queue_push_tail(&q, cmd);
    if (ret) {
        pthread_mutex_unlock(&lock);
        return OMX_ErrorInsufficientResources;
    }

    workq->ScheduleWork(this);
    pthread_mutex_unlock(&lock);

    return OMX_ErrorNone;
}

struct cmd_s *CmdProcessWork::PopCmdQueue(void)
{
    struct cmd_s *cmd;

    pthread_mutex_lock(&lock);
    cmd = (struct cmd_s *)queue_pop_head(&q);
    pthread_mutex_unlock(&lock);

    return cmd;
}

void CmdProcessWork::ScheduleIfAvailable(void)
{
    bool avail;

    pthread_mutex_lock(&lock);
    avail = queue_length(&q) ? true : false;
    pthread_mutex_unlock(&lock);

    if (avail)
        workq->ScheduleWork(this);
}

void CmdProcessWork::Work(void)
{
    struct cmd_s *cmd;

    cmd = PopCmdQueue();
    if (cmd) {
        ci->CmdHandler(cmd);
        free(cmd);
    }
    ScheduleIfAvailable();
}

/* end of CmdProcessWork */

/*
 * ComponentBase
 */
/*
 * constructor & destructor
 */
void ComponentBase::__ComponentBase(void)
{
    memset(name, 0, OMX_MAX_STRINGNAME_SIZE);
    cmodule = NULL;
    handle = NULL;

    roles = NULL;
    nr_roles = 0;

    working_role = NULL;

    ports = NULL;
    nr_ports = 0;
    memset(&portparam, 0, sizeof(portparam));

    state = OMX_StateUnloaded;

    cmdwork = new CmdProcessWork(this);

    executing = false;
    pthread_mutex_init(&executing_lock, NULL);
    pthread_cond_init(&executing_wait, NULL);

    bufferwork = new WorkQueue();

    pthread_mutex_init(&ports_block, NULL);
}

ComponentBase::ComponentBase()
{
    __ComponentBase();
}

ComponentBase::ComponentBase(const OMX_STRING name)
{
    __ComponentBase();
    SetName(name);
}

ComponentBase::~ComponentBase()
{
    delete cmdwork;

    delete bufferwork;
    pthread_mutex_destroy(&executing_lock);
    pthread_cond_destroy(&executing_wait);

    pthread_mutex_destroy(&ports_block);

    if (roles) {
        if (roles[0])
            free(roles[0]);
        free(roles);
    }
}

/* end of constructor & destructor */

/*
 * accessor
 */
/* name */
void ComponentBase::SetName(const OMX_STRING name)
{
    strncpy(this->name, name, OMX_MAX_STRINGNAME_SIZE);
    this->name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
}

const OMX_STRING ComponentBase::GetName(void)
{
    return name;
}

/* component module */
void ComponentBase::SetCModule(CModule *cmodule)
{
    this->cmodule = cmodule;
}

CModule *ComponentBase::GetCModule(void)
{
    return cmodule;
}

/* end of accessor */

/*
 * core methods & helpers
 */
/* roles */
OMX_ERRORTYPE ComponentBase::SetRolesOfComponent(OMX_U32 nr_roles,
                                                 const OMX_U8 **roles)
{
    OMX_U32 i;

    if (!roles || !nr_roles)
        return OMX_ErrorBadParameter;

    if (this->roles) {
        free(this->roles[0]);
        free(this->roles);
        this->roles = NULL;
    }

    this->roles = (OMX_U8 **)malloc(sizeof(OMX_STRING) * nr_roles);
    if (!this->roles)
        return OMX_ErrorInsufficientResources;

    this->roles[0] = (OMX_U8 *)malloc(OMX_MAX_STRINGNAME_SIZE * nr_roles);
    if (!this->roles[0]) {
        free(this->roles);
        this->roles = NULL;
        return OMX_ErrorInsufficientResources;
    }

    for (i = 0; i < nr_roles; i++) {
        if (i < nr_roles-1)
            this->roles[i+1] = this->roles[i] + OMX_MAX_STRINGNAME_SIZE;

        strncpy((OMX_STRING)&this->roles[i][0],
                (const OMX_STRING)&roles[i][0], OMX_MAX_STRINGNAME_SIZE);
    }

    this->nr_roles = nr_roles;
    return OMX_ErrorNone;
}

/* GetHandle & FreeHandle */
OMX_ERRORTYPE ComponentBase::GetHandle(OMX_HANDLETYPE *pHandle,
                                       OMX_PTR pAppData,
                                       OMX_CALLBACKTYPE *pCallBacks)
{
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (handle)
        return OMX_ErrorUndefined;

    handle = (OMX_COMPONENTTYPE *)calloc(1, sizeof(*handle));
    if (!handle)
        return OMX_ErrorInsufficientResources;

    /* handle initialization */
    SetTypeHeader(handle, sizeof(*handle));
    handle->pComponentPrivate = static_cast<OMX_PTR>(this);
    handle->pApplicationPrivate = pAppData;

    /* connect handle's functions */
    handle->GetComponentVersion = GetComponentVersion;
    handle->SendCommand = SendCommand;
    handle->GetParameter = GetParameter;
    handle->SetParameter = SetParameter;
    handle->GetConfig = GetConfig;
    handle->SetConfig = SetConfig;
    handle->GetExtensionIndex = GetExtensionIndex;
    handle->GetState = GetState;
    handle->ComponentTunnelRequest = ComponentTunnelRequest;
    handle->UseBuffer = UseBuffer;
    handle->AllocateBuffer = AllocateBuffer;
    handle->FreeBuffer = FreeBuffer;
    handle->EmptyThisBuffer = EmptyThisBuffer;
    handle->FillThisBuffer = FillThisBuffer;
    handle->SetCallbacks = SetCallbacks;
    handle->ComponentDeInit = ComponentDeInit;
    handle->UseEGLImage = UseEGLImage;
    handle->ComponentRoleEnum = ComponentRoleEnum;

    appdata = pAppData;
    callbacks = pCallBacks;

    if (nr_roles == 1) {
        SetWorkingRole((OMX_STRING)&roles[0][0]);
        ret = ApplyWorkingRole();
        if (ret != OMX_ErrorNone) {
            SetWorkingRole(NULL);
            goto free_handle;
        }
    }

    *pHandle = (OMX_HANDLETYPE *)handle;
    state = OMX_StateLoaded;
    return OMX_ErrorNone;

free_handle:
    free(handle);

    appdata = NULL;
    callbacks = NULL;
    *pHandle = NULL;

    return ret;
}

OMX_ERRORTYPE ComponentBase::FreeHandle(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    FreePorts();

    free(handle);

    appdata = NULL;
    callbacks = NULL;

    state = OMX_StateUnloaded;
    return OMX_ErrorNone;
}

/* end of core methods & helpers */

/*
 * component methods & helpers
 */
OMX_ERRORTYPE ComponentBase::GetComponentVersion(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
    OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetComponentVersion(hComponent,
                                           pComponentName,
                                           pComponentVersion,
                                           pSpecVersion,
                                           pComponentUUID);
}

OMX_ERRORTYPE ComponentBase::CBaseGetComponentVersion(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STRING pComponentName,
    OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
    OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
    OMX_OUT OMX_UUIDTYPE* pComponentUUID)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::SendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSendCommand(hComponent, Cmd, nParam1, pCmdData);
}

OMX_ERRORTYPE ComponentBase::CBaseSendCommand(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_COMMANDTYPE Cmd,
    OMX_IN  OMX_U32 nParam1,
    OMX_IN  OMX_PTR pCmdData)
{
    struct cmd_s *cmd;

    if (hComponent != handle)
        return OMX_ErrorInvalidComponent;

    /* basic error check */
    switch (Cmd) {
    case OMX_CommandStateSet:
        /*
         * Todo
         */
        break;
    case OMX_CommandFlush: {
        OMX_U32 port_index = nParam1;

        if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
            return OMX_ErrorBadPortIndex;
        break;
    }
    case OMX_CommandPortDisable:
    case OMX_CommandPortEnable: {
        OMX_U32 port_index = nParam1;

        if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
            return OMX_ErrorBadPortIndex;
        break;
    }
    case OMX_CommandMarkBuffer: {
        OMX_MARKTYPE *mark = (OMX_MARKTYPE *)pCmdData;
        OMX_MARKTYPE *copiedmark;
        OMX_U32 port_index = nParam1;

        if (port_index > nr_ports-1)
            return OMX_ErrorBadPortIndex;

        if (!mark || !mark->hMarkTargetComponent)
            return OMX_ErrorBadParameter;

        copiedmark = (OMX_MARKTYPE *)malloc(sizeof(*copiedmark));
        if (!copiedmark)
            return OMX_ErrorInsufficientResources;

        copiedmark->hMarkTargetComponent = mark->hMarkTargetComponent;
        copiedmark->pMarkData = mark->pMarkData;
        pCmdData = (OMX_PTR)copiedmark;
        break;
    }
    default:
        LOGE("command %d not supported\n", Cmd);
        return OMX_ErrorUnsupportedIndex;
    }

    cmd = (struct cmd_s *)malloc(sizeof(*cmd));
    if (!cmd)
        return OMX_ErrorInsufficientResources;

    cmd->cmd = Cmd;
    cmd->param1 = nParam1;
    cmd->cmddata = pCmdData;

    return cmdwork->PushCmdQueue(cmd);
}

OMX_ERRORTYPE ComponentBase::GetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetParameter(hComponent, nParamIndex,
                                    pComponentParameterStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseGetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nParamIndex,
    OMX_INOUT OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nParamIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit: {
        OMX_PORT_PARAM_TYPE *p =
            (OMX_PORT_PARAM_TYPE *)pComponentParameterStructure;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        memcpy(p, &portparam, sizeof(*p));
        break;
    }
    case OMX_IndexParamPortDefinition: {
        OMX_PARAM_PORTDEFINITIONTYPE *p =
            (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32 index = p->nPortIndex;
        PortBase *port = NULL;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        if (index < nr_ports)
            port = ports[index];

        if (!port)
            return OMX_ErrorBadPortIndex;

        memcpy(p, port->GetPortDefinition(), sizeof(*p));
        break;
    }
    case OMX_IndexParamCompBufferSupplier:
        /*
         * Todo
         */

        ret = OMX_ErrorUnsupportedIndex;
        break;
    default:
        ret = ComponentGetParameter(nParamIndex, pComponentParameterStructure);
    } /* switch */

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetParameter(hComponent, nIndex,
                                    pComponentParameterStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseSetParameter(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentParameterStructure)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    case OMX_IndexParamAudioInit:
    case OMX_IndexParamVideoInit:
    case OMX_IndexParamImageInit:
    case OMX_IndexParamOtherInit:
        /* preventing clients from setting OMX_PORT_PARAM_TYPE */
        ret = OMX_ErrorUnsupportedIndex;
        break;
    case OMX_IndexParamPortDefinition: {
        OMX_PARAM_PORTDEFINITIONTYPE *p =
            (OMX_PARAM_PORTDEFINITIONTYPE *)pComponentParameterStructure;
        OMX_U32 index = p->nPortIndex;
        PortBase *port = NULL;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        if (index < nr_ports)
            port = ports[index];

        if (!port)
            return OMX_ErrorBadPortIndex;

        if (port->IsEnabled()) {
            if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
                return OMX_ErrorIncorrectStateOperation;
        }

        port->SetPortDefinition(p, false);
        break;
    }
    case OMX_IndexParamCompBufferSupplier:
        /*
         * Todo
         */

        ret = OMX_ErrorUnsupportedIndex;
        break;
    case OMX_IndexParamStandardComponentRole: {
        OMX_PARAM_COMPONENTROLETYPE *p =
            (OMX_PARAM_COMPONENTROLETYPE *)pComponentParameterStructure;

        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;

        ret = CheckTypeHeader(p, sizeof(*p));
        if (ret != OMX_ErrorNone)
            return ret;

        ret = SetWorkingRole((OMX_STRING)p->cRole);
        if (ret != OMX_ErrorNone)
            return ret;

        if (ports)
            FreePorts();

        ret = ApplyWorkingRole();
        if (ret != OMX_ErrorNone) {
            SetWorkingRole(NULL);
            return ret;
        }
        break;
    }
    default:
        ret = ComponentSetParameter(nIndex, pComponentParameterStructure);
    } /* switch */

    return ret;
}

OMX_ERRORTYPE ComponentBase::GetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetConfig(hComponent, nIndex,
                                 pComponentConfigStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseGetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_INOUT OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    default:
        ret = ComponentGetConfig(nIndex, pComponentConfigStructure);
    }

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetConfig(hComponent, nIndex,
                                 pComponentConfigStructure);
}

OMX_ERRORTYPE ComponentBase::CBaseSetConfig(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_INDEXTYPE nIndex,
    OMX_IN  OMX_PTR pComponentConfigStructure)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    switch (nIndex) {
    default:
        ret = ComponentSetConfig(nIndex, pComponentConfigStructure);
    }

    return ret;
}

OMX_ERRORTYPE ComponentBase::GetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetExtensionIndex(hComponent, cParameterName,
                                         pIndexType);
}

OMX_ERRORTYPE ComponentBase::CBaseGetExtensionIndex(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_STRING cParameterName,
    OMX_OUT OMX_INDEXTYPE* pIndexType)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::GetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseGetState(hComponent, pState);
}

OMX_ERRORTYPE ComponentBase::CBaseGetState(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_STATETYPE* pState)
{
    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    *pState = state;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComponent,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseComponentTunnelRequest(hComponent, nPort,
                                              hTunneledComponent,
                                              nTunneledPort, pTunnelSetup);
}

OMX_ERRORTYPE ComponentBase::CBaseComponentTunnelRequest(
    OMX_IN  OMX_HANDLETYPE hComp,
    OMX_IN  OMX_U32 nPort,
    OMX_IN  OMX_HANDLETYPE hTunneledComp,
    OMX_IN  OMX_U32 nTunneledPort,
    OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::UseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseUseBuffer(hComponent, ppBufferHdr, nPortIndex,
                                 pAppPrivate, nSizeBytes, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseUseBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8 *pBuffer)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!ppBufferHdr)
        return OMX_ErrorBadParameter;
    *ppBufferHdr = NULL;

    if (!pBuffer)
        return OMX_ErrorBadParameter;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;
    }

    return port->UseBuffer(ppBufferHdr, nPortIndex, pAppPrivate, nSizeBytes,
                           pBuffer);
}

OMX_ERRORTYPE ComponentBase::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseAllocateBuffer(hComponent, ppBuffer, nPortIndex,
                                      pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ComponentBase::CBaseAllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE **ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!ppBuffer)
        return OMX_ErrorBadParameter;
    *ppBuffer = NULL;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateLoaded && state != OMX_StateWaitForResources)
            return OMX_ErrorIncorrectStateOperation;
    }

    return port->AllocateBuffer(ppBuffer, nPortIndex, pAppPrivate, nSizeBytes);
}

OMX_ERRORTYPE ComponentBase::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseFreeBuffer(hComponent, nPortIndex, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseFreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    if (!pBuffer)
        return OMX_ErrorBadParameter;

    if (ports)
        if (nPortIndex < nr_ports)
            port = ports[nPortIndex];

    if (!port)
        return OMX_ErrorBadParameter;

    return port->FreeBuffer(nPortIndex, pBuffer);
}

OMX_ERRORTYPE ComponentBase::EmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseEmptyThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseEmptyThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_U32 port_index;
    OMX_ERRORTYPE ret;

    if ((hComponent != handle) || !pBuffer)
        return OMX_ErrorBadParameter;

    ret = CheckTypeHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone)
        return ret;

    port_index = pBuffer->nInputPortIndex;
    if (port_index == (OMX_U32)-1)
        return OMX_ErrorBadParameter;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port)
        return OMX_ErrorBadParameter;

    if (pBuffer->pInputPortPrivate != port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateIdle && state != OMX_StateExecuting &&
            state != OMX_StatePause)
            return OMX_ErrorIncorrectStateOperation;
    }

    if (!pBuffer->hMarkTargetComponent) {
        OMX_MARKTYPE *mark;

        mark = port->PopMark();
        if (mark) {
            pBuffer->hMarkTargetComponent = mark->hMarkTargetComponent;
            pBuffer->pMarkData = mark->pMarkData;
            free(mark);
        }
    }

    ret = port->PushThisBuffer(pBuffer);
    if (ret == OMX_ErrorNone)
        bufferwork->ScheduleWork(this);

    return ret;
}

OMX_ERRORTYPE ComponentBase::FillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseFillThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseFillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE *pBuffer)
{
    PortBase *port = NULL;
    OMX_U32 port_index;
    OMX_ERRORTYPE ret;

    if ((hComponent != handle) || !pBuffer)
        return OMX_ErrorBadParameter;

    ret = CheckTypeHeader(pBuffer, sizeof(OMX_BUFFERHEADERTYPE));
    if (ret != OMX_ErrorNone)
        return ret;

    port_index = pBuffer->nOutputPortIndex;
    if (port_index == (OMX_U32)-1)
        return OMX_ErrorBadParameter;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port)
        return OMX_ErrorBadParameter;

    if (pBuffer->pOutputPortPrivate != port)
        return OMX_ErrorBadParameter;

    if (port->IsEnabled()) {
        if (state != OMX_StateIdle && state != OMX_StateExecuting &&
            state != OMX_StatePause)
            return OMX_ErrorIncorrectStateOperation;
    }

    ret = port->PushThisBuffer(pBuffer);
    if (ret == OMX_ErrorNone)
        bufferwork->ScheduleWork(this);

    return ret;
}

OMX_ERRORTYPE ComponentBase::SetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseSetCallbacks(hComponent, pCallbacks, pAppData);
}

OMX_ERRORTYPE ComponentBase::CBaseSetCallbacks(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_CALLBACKTYPE *pCallbacks,
    OMX_IN  OMX_PTR pAppData)
{
    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    appdata = pAppData;
    callbacks = pCallbacks;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ComponentDeInit(
    OMX_IN  OMX_HANDLETYPE hComponent)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseComponentDeInit(hComponent);
}

OMX_ERRORTYPE ComponentBase::CBaseComponentDeInit(
    OMX_IN  OMX_HANDLETYPE hComponent)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::UseEGLImage(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN void* eglImage)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseUseEGLImage(hComponent, ppBufferHdr, nPortIndex,
                                   pAppPrivate, eglImage);
}

OMX_ERRORTYPE ComponentBase::CBaseUseEGLImage(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN void* eglImage)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::ComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    ComponentBase *cbase;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->CBaseComponentRoleEnum(hComponent, cRole, nIndex);
}

OMX_ERRORTYPE ComponentBase::CBaseComponentRoleEnum(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_OUT OMX_U8 *cRole,
    OMX_IN OMX_U32 nIndex)
{
    if (hComponent != (OMX_HANDLETYPE *)this->handle)
        return OMX_ErrorBadParameter;

    if (nIndex > nr_roles)
        return OMX_ErrorBadParameter;

    strncpy((char *)cRole, (const char *)roles[nIndex],
            OMX_MAX_STRINGNAME_SIZE);
    return OMX_ErrorNone;
}

/* implement CmdHandlerInterface */
void ComponentBase::CmdHandler(struct cmd_s *cmd)
{
    switch (cmd->cmd) {
    case OMX_CommandStateSet: {
        OMX_STATETYPE transition = (OMX_STATETYPE)cmd->param1;

        TransState(transition);
        break;
    }
    case OMX_CommandFlush: {
        OMX_U32 port_index = cmd->param1;

        FlushPort(port_index, 1);
        break;
    }
    case OMX_CommandPortDisable: {
        OMX_U32 port_index = cmd->param1;

        TransStatePort(port_index, PortBase::OMX_PortDisabled);
        break;
    }
    case OMX_CommandPortEnable: {
        OMX_U32 port_index = cmd->param1;

        TransStatePort(port_index, PortBase::OMX_PortEnabled);
        break;
    }
    case OMX_CommandMarkBuffer: {
        OMX_U32 port_index = (OMX_U32)cmd->param1;
        OMX_MARKTYPE *mark = (OMX_MARKTYPE *)cmd->cmddata;

        PushThisMark(port_index, mark);
        break;
    }
    default:
        LOGE("receive command 0x%08x that cannot be handled\n", cmd->cmd);
        break;
    } /* switch */
}

/*
 * SendCommand:OMX_CommandStateSet
 * called in CmdHandler or called in other parts of component for reporting
 * internal error (OMX_StateInvalid).
 */
/*
 * Todo
 *   Resource Management (OMX_StateWaitForResources)
 *   for now, we never notify OMX_ErrorInsufficientResources,
 *   so IL client doesn't try to set component' state OMX_StateWaitForResources
 */
static const char *state_name[OMX_StateWaitForResources + 1] = {
    "OMX_StateInvalid",
    "OMX_StateLoaded",
    "OMX_StateIdle",
    "OMX_StateExecuting",
    "OMX_StatePause",
    "OMX_StateWaitForResources",
};

static inline const char *GetStateName(OMX_STATETYPE state)
{
    if (state > OMX_StateWaitForResources)
        return "UnKnown";

    return state_name[state];
}

void ComponentBase::TransState(OMX_STATETYPE transition)
{
    OMX_STATETYPE current = this->state;
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_ERRORTYPE ret;

    LOGD("current state = %s, transition state = %s\n",
         GetStateName(current), GetStateName(transition));

    /* same state */
    if (current == transition) {
        ret = OMX_ErrorSameState;
        goto notify_event;
    }

    /* invalid state */
    if (current == OMX_StateInvalid) {
        ret = OMX_ErrorInvalidState;
        goto notify_event;
    }

    if (transition == OMX_StateLoaded)
        ret = TransStateToLoaded(current);
    else if (transition == OMX_StateIdle)
        ret = TransStateToIdle(current);
    else if (transition == OMX_StateExecuting)
        ret = TransStateToExecuting(current);
    else if (transition == OMX_StatePause)
        ret = TransStateToPause(current);
    else if (transition == OMX_StateInvalid)
        ret = TransStateToInvalid(current);
    else if (transition == OMX_StateWaitForResources)
        ret = TransStateToWaitForResources(current);
    else
        ret = OMX_ErrorIncorrectStateTransition;

notify_event:
    if (ret == OMX_ErrorNone) {
        event = OMX_EventCmdComplete;
        data1 = OMX_CommandStateSet;
        data2 = transition;

        state = transition;
        LOGD("transition from %s to %s completed\n",
             GetStateName(current), GetStateName(transition));
    }
    else {
        event = OMX_EventError;
        data1 = ret;
        data2 = 0;

        if (transition == OMX_StateInvalid || ret == OMX_ErrorInvalidState) {
            state = OMX_StateInvalid;
            LOGD("failed transition from %s to %s, current state is %s\n",
                 GetStateName(current), GetStateName(transition),
                 GetStateName(state));
        }
    }

    callbacks->EventHandler(handle, appdata, event, data1, data2, NULL);

    /* WaitForResources workaround */
    if (ret == OMX_ErrorNone && transition == OMX_StateWaitForResources)
        callbacks->EventHandler(handle, appdata,
                                OMX_EventResourcesAcquired, 0, 0, NULL);
}

inline OMX_ERRORTYPE ComponentBase::TransStateToLoaded(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        OMX_U32 i;

        for (i = 0; i < nr_ports; i++)
            ports[i]->WaitPortBufferCompletion();

        ret = ProcessorDeinit();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorDeinit() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else if (current == OMX_StateWaitForResources) {
        LOGE("state transition's requested from WaitForResources to "
             "Loaded\n");

        /*
         * from WaitForResources to Loaded considered from Loaded to Loaded.
         * do nothing
         */

        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToIdle(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateLoaded) {
        OMX_U32 i;

        ret = ProcessorInit();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorInit() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }

        for (i = 0; i < nr_ports; i++) {
            if (ports[i]->IsEnabled())
                ports[i]->WaitPortBufferCompletion();
        }
    }
    else if ((current == OMX_StatePause) || (current == OMX_StateExecuting)) {
        FlushPort(OMX_ALL, 0);

        bufferwork->CancelScheduledWork(this);

        if (current == OMX_StatePause) {
            pthread_mutex_lock(&executing_lock);
            executing = true;
            pthread_cond_signal(&executing_wait);
            pthread_mutex_unlock(&executing_lock);
        }

        bufferwork->StopWork();

        ret = ProcessorStop();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorStop() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else if (current == OMX_StateWaitForResources) {
        LOGE("state transition's requested from WaitForResources to Idle\n");

        /* same as Loaded to Idle BUT DO NOTHING for now */

        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE
ComponentBase::TransStateToExecuting(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        pthread_mutex_lock(&executing_lock);
        executing = true;
        pthread_mutex_unlock(&executing_lock);

        bufferwork->StartWork();

        ret = ProcessorStart();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorStart() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else if (current == OMX_StatePause) {
        pthread_mutex_lock(&executing_lock);
        executing = true;
        pthread_cond_signal(&executing_wait);
        pthread_mutex_unlock(&executing_lock);

        ret = ProcessorResume();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorStart() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToPause(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateIdle) {
        /* turn off executing flag */
        pthread_mutex_lock(&executing_lock);
        executing = false;
        pthread_mutex_unlock(&executing_lock);

        bufferwork->StartWork();

        ret = ProcessorStart();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorPause() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else if (current == OMX_StateExecuting) {
        pthread_mutex_lock(&executing_lock);
        executing = false;
        pthread_mutex_unlock(&executing_lock);

        ret = ProcessorPause();
        if (ret != OMX_ErrorNone) {
            LOGE("failed to ProcessorPause() (ret = 0x%08x)\n", ret);
            ret = OMX_ErrorInvalidState;
            goto out;
        }
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

out:
    return ret;
}

inline OMX_ERRORTYPE ComponentBase::TransStateToInvalid(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret = OMX_ErrorInvalidState;

    /*
     * Todo
     *   graceful escape
     */

    return ret;
}

inline OMX_ERRORTYPE
ComponentBase::TransStateToWaitForResources(OMX_STATETYPE current)
{
    OMX_ERRORTYPE ret;

    if (current == OMX_StateLoaded) {
        LOGE("state transition's requested from Loaded to WaitForResources\n");
        ret = OMX_ErrorNone;
    }
    else
        ret = OMX_ErrorIncorrectStateTransition;

    return ret;
}

/* mark buffer */
void ComponentBase::PushThisMark(OMX_U32 port_index, OMX_MARKTYPE *mark)
{
    PortBase *port = NULL;
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_ERRORTYPE ret;

    if (ports)
        if (port_index < nr_ports)
            port = ports[port_index];

    if (!port) {
        ret = OMX_ErrorBadPortIndex;
        goto notify_event;
    }

    ret = port->PushMark(mark);
    if (ret != OMX_ErrorNone) {
        /* don't report OMX_ErrorInsufficientResources */
        ret = OMX_ErrorUndefined;
        goto notify_event;
    }

notify_event:
    if (ret == OMX_ErrorNone) {
        event = OMX_EventCmdComplete;
        data1 = OMX_CommandMarkBuffer;
        data2 = port_index;
    }
    else {
        event = OMX_EventError;
        data1 = ret;
        data2 = 0;
    }

    callbacks->EventHandler(handle, appdata, event, data1, data2, NULL);
}

void ComponentBase::FlushPort(OMX_U32 port_index, bool notify)
{
    OMX_U32 i, from_index, to_index;

    if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
        return;

    if (port_index == OMX_ALL) {
        from_index = 0;
        to_index = nr_ports - 1;
    }
    else {
        from_index = port_index;
        to_index = port_index;
    }

    pthread_mutex_lock(&ports_block);
    for (i = from_index; i <= to_index; i++) {
        ports[i]->FlushPort();
        if (notify)
            callbacks->EventHandler(handle, appdata, OMX_EventCmdComplete,
                                    OMX_CommandFlush, i, NULL);
    }
    pthread_mutex_unlock(&ports_block);
}

void ComponentBase::TransStatePort(OMX_U32 port_index, OMX_U8 state)
{
    OMX_EVENTTYPE event;
    OMX_U32 data1, data2;
    OMX_U32 i, from_index, to_index;
    OMX_ERRORTYPE ret;

    if ((port_index != OMX_ALL) && (port_index > nr_ports-1))
        return;

    if (port_index == OMX_ALL) {
        from_index = 0;
        to_index = nr_ports - 1;
    }
    else {
        from_index = port_index;
        to_index = port_index;
    }

    pthread_mutex_lock(&ports_block);
    for (i = from_index; i <= to_index; i++) {
        ret = ports[i]->TransState(state);
        if (ret == OMX_ErrorNone) {
            event = OMX_EventCmdComplete;
            if (state == PortBase::OMX_PortEnabled)
                data1 = OMX_CommandPortEnable;
            else
                data1 = OMX_CommandPortDisable;
            data2 = i;
        }
        else {
            event = OMX_EventError;
            data1 = ret;
            data2 = 0;
        }
        callbacks->EventHandler(handle, appdata, OMX_EventCmdComplete,
                                OMX_CommandPortDisable, data2, NULL);
    }
    pthread_mutex_unlock(&ports_block);
}

/* set working role */
OMX_ERRORTYPE ComponentBase::SetWorkingRole(const OMX_STRING role)
{
    OMX_U32 i;

    if (state != OMX_StateUnloaded && state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    if (!role) {
        working_role = NULL;
        return OMX_ErrorNone;
    }

    for (i = 0; i < nr_roles; i++) {
        if (!strcmp((char *)&roles[i][0], role)) {
            working_role = (OMX_STRING)&roles[i][0];
            return OMX_ErrorNone;
        }
    }

    LOGE("cannot find %s role in %s\n", role, name);
    return OMX_ErrorBadParameter;
}

/* apply a working role for a component having multiple roles */
OMX_ERRORTYPE ComponentBase::ApplyWorkingRole(void)
{
    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (state != OMX_StateUnloaded && state != OMX_StateLoaded)
        return OMX_ErrorIncorrectStateOperation;

    if (!working_role)
        return OMX_ErrorBadParameter;

    if (!callbacks || !appdata)
        return OMX_ErrorBadParameter;

    ret = AllocatePorts();
    if (ret != OMX_ErrorNone) {
        LOGE("failed to AllocatePorts() (ret = 0x%08x)\n", ret);
        return ret;
    }

    /* now we can access ports */
    for (i = 0; i < nr_ports; i++) {
        ports[i]->SetOwner(handle);
        ports[i]->SetCallbacks(handle, callbacks, appdata);
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::AllocatePorts(void)
{
    OMX_ERRORTYPE ret;

    if (ports)
        return OMX_ErrorBadParameter;

    ret = ComponentAllocatePorts();
    if (ret != OMX_ErrorNone) {
        LOGE("failed to %s::ComponentAllocatePorts(), ret = 0x%08x\n",
             name, ret);
        return ret;
    }

    return OMX_ErrorNone;
}

/* called int FreeHandle() */
OMX_ERRORTYPE ComponentBase::FreePorts(void)
{
    if (ports) {
        OMX_U32 i, this_nr_ports = this->nr_ports;

        for (i = 0; i < this_nr_ports; i++) {
            if (ports[i]) {
                OMX_MARKTYPE *mark;
                /* it should be empty before this */
                while ((mark = ports[i]->PopMark()))
                    free(mark);

                delete ports[i];
                ports[i] = NULL;
            }
        }
        delete []ports;
        ports = NULL;
    }

    return OMX_ErrorNone;
}

/* buffer processing */
/* implement WorkableInterface */
void ComponentBase::Work(void)
{
    OMX_BUFFERHEADERTYPE *buffers[nr_ports];
    bool retain[nr_ports];
    OMX_U32 i;
    bool avail = false;

    pthread_mutex_lock(&executing_lock);
    if (!executing)
        pthread_cond_wait(&executing_wait, &executing_lock);
    pthread_mutex_unlock(&executing_lock);

    pthread_mutex_lock(&ports_block);

    avail = IsAllBufferAvailable();
    if (avail) {
        for (i = 0; i < nr_ports; i++) {
            buffers[i] = ports[i]->PopBuffer();
            retain[i] = false;
        }

        ProcessorProcess(buffers, &retain[0], nr_ports);
        PostProcessBuffer(buffers, &retain[0], nr_ports);

        for (i = 0; i < nr_ports; i++) {
            if (retain[i])
                ports[i]->RetainThisBuffer(buffers[i]);
            else
                ports[i]->ReturnThisBuffer(buffers[i]);
        }
    }
    ScheduleIfAllBufferAvailable();
    pthread_mutex_unlock(&ports_block);
}

bool ComponentBase::IsAllBufferAvailable(void)
{
    OMX_U32 i;
    OMX_U32 nr_avail = 0;

    for (i = 0; i < nr_ports; i++) {
        OMX_U32 length = 0;

        if (ports[i]->IsEnabled())
            length = ports[i]->BufferQueueLength();

        if (length)
            nr_avail++;
    }

    if (nr_avail == nr_ports)
        return true;
    else
        return false;
}

void ComponentBase::ScheduleIfAllBufferAvailable(void)
{
    bool avail;

    avail = IsAllBufferAvailable();
    if (avail)
        bufferwork->ScheduleWork(this);
}

void ComponentBase::PostProcessBuffer(OMX_BUFFERHEADERTYPE **buffers,
                                      bool *retain,
                                      OMX_U32 nr_buffers)
{
    OMX_U32 i;

    for (i = 0; i < nr_ports; i++) {
        OMX_MARKTYPE *mark;

        if (ports[i]->GetPortDirection() == OMX_DirInput) {
            bool is_sink_component = true;
            OMX_U32 j;

            if (buffers[i]->hMarkTargetComponent) {
                if (buffers[i]->hMarkTargetComponent == handle) {
                    callbacks->EventHandler(handle, appdata, OMX_EventMark,
                                            0, 0, buffers[i]->pMarkData);
                    buffers[i]->hMarkTargetComponent = NULL;
                    buffers[i]->pMarkData = NULL;
                }
            }

            for (j = 0; j < nr_ports; j++) {
                if (j == i)
                    continue;

                if (ports[j]->GetPortDirection() == OMX_DirOutput) {
                    if (buffers[i]->nFlags & OMX_BUFFERFLAG_EOS) {
                        buffers[j]->nFlags |= OMX_BUFFERFLAG_EOS;
                        buffers[i]->nFlags &= ~OMX_BUFFERFLAG_EOS;
                        retain[i] = false;
                        retain[j] = false;
                    }

                    if (!buffers[j]->hMarkTargetComponent) {
                        mark = ports[j]->PopMark();
                        if (mark) {
                            buffers[j]->hMarkTargetComponent =
                                mark->hMarkTargetComponent;
                            buffers[j]->pMarkData = mark->pMarkData;
                            free(mark);
                            mark = NULL;
                        }

                        if (buffers[i]->hMarkTargetComponent) {
                            if (buffers[j]->hMarkTargetComponent) {
                                mark = (OMX_MARKTYPE *)
                                    malloc(sizeof(*mark));
                                if (mark) {
                                    mark->hMarkTargetComponent =
                                        buffers[i]->hMarkTargetComponent;
                                    mark->pMarkData = buffers[i]->pMarkData;
                                    ports[j]->PushMark(mark);
                                    mark = NULL;
                                    buffers[i]->hMarkTargetComponent = NULL;
                                    buffers[i]->pMarkData = NULL;
                                }
                            }
                            else {
                                buffers[j]->hMarkTargetComponent =
                                    buffers[i]->hMarkTargetComponent;
                                buffers[j]->pMarkData = buffers[i]->pMarkData;
                                buffers[i]->hMarkTargetComponent = NULL;
                                buffers[i]->pMarkData = NULL;
                            }
                        }
                    }
                    else {
                        if (buffers[i]->hMarkTargetComponent) {
                            mark = (OMX_MARKTYPE *)malloc(sizeof(*mark));
                            if (mark) {
                                mark->hMarkTargetComponent =
                                    buffers[i]->hMarkTargetComponent;
                                mark->pMarkData = buffers[i]->pMarkData;
                                ports[j]->PushMark(mark);
                                mark = NULL;
                                buffers[i]->hMarkTargetComponent = NULL;
                                buffers[i]->pMarkData = NULL;
                            }
                        }
                    }
                    is_sink_component = false;
                }
            }

            if (is_sink_component) {
                if (buffers[i]->nFlags & OMX_BUFFERFLAG_EOS) {
                    callbacks->EventHandler(handle, appdata,
                                            OMX_EventBufferFlag,
                                            i, buffers[i]->nFlags, NULL);
                    retain[i] = false;
                }
            }
        }
        else if (ports[i]->GetPortDirection() == OMX_DirOutput) {
            bool is_source_component = true;
            OMX_U32 j;

            if (buffers[i]->nFlags & OMX_BUFFERFLAG_EOS) {
                callbacks->EventHandler(handle, appdata,
                                        OMX_EventBufferFlag,
                                        i, buffers[i]->nFlags, NULL);
                retain[i] = false;
            }

            for (j = 0; j < nr_ports; j++) {
                if (j == i)
                    continue;

                if (ports[j]->GetPortDirection() == OMX_DirInput)
                    is_source_component = false;
            }

            if (is_source_component) {
                if (!retain[i]) {
                    mark = ports[i]->PopMark();
                    if (mark) {
                        buffers[i]->hMarkTargetComponent =
                            mark->hMarkTargetComponent;
                        buffers[i]->pMarkData = mark->pMarkData;
                        free(mark);
                        mark = NULL;

                        if (buffers[i]->hMarkTargetComponent == handle) {
                            callbacks->EventHandler(handle, appdata,
                                                    OMX_EventMark, 0, 0,
                                                    buffers[i]->pMarkData);
                            buffers[i]->hMarkTargetComponent = NULL;
                            buffers[i]->pMarkData = NULL;
                        }
                    }
                }
            }
        }
        else {
            LOGE("%s(): fatal error unknown port direction (0x%08x)\n",
                 __func__, ports[i]->GetPortDirection());
        }
    }
}

/* processor default callbacks */
OMX_ERRORTYPE ComponentBase::ProcessorInit(void)
{
    return OMX_ErrorNone;
}
OMX_ERRORTYPE ComponentBase::ProcessorDeinit(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorStart(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorStop(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorPause(void)
{
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::ProcessorResume(void)
{
    return OMX_ErrorNone;
}

/* end of processor callbacks */

/* helper for derived class */
const OMX_STRING ComponentBase::GetWorkingRole(void)
{
    return &working_role[0];
}

const OMX_COMPONENTTYPE *ComponentBase::GetComponentHandle(void)
{
    return handle;
}

void ComponentBase::DumpBuffer(const OMX_BUFFERHEADERTYPE *bufferheader)
{
    OMX_U8 *pbuffer = bufferheader->pBuffer, *p;
    OMX_U32 offset = bufferheader->nOffset;
    OMX_U32 alloc_len = bufferheader->nAllocLen;
    OMX_U32 filled_len =  bufferheader->nFilledLen;
    OMX_U32 left = filled_len, oneline;
    OMX_U32 index = 0, i;
    /* 0x%04lx:  %02x %02x .. (n = 16)\n\0 */
    char prbuffer[8 + 3 * 0x10 + 2], *pp;
    OMX_U32 prbuffer_len;

    LOGD("Componant %s DumpBuffer\n", name);
    LOGD("%s port index = %lu",
         (bufferheader->nInputPortIndex != 0x7fffffff) ? "input" : "output",
         (bufferheader->nInputPortIndex != 0x7fffffff) ?
         bufferheader->nInputPortIndex : bufferheader->nOutputPortIndex);
    LOGD("nAllocLen = %lu, nOffset = %lu, nFilledLen = %lu\n",
         alloc_len, offset, filled_len);
    LOGD("nTimeStamp = %lld, nTickCount = %lu",
         bufferheader->nTimeStamp,
         bufferheader->nTickCount);
    LOGD("nFlags = 0x%08lx\n", bufferheader->nFlags);

    if (!pbuffer || !alloc_len || !filled_len)
        return;

    if (offset + filled_len > alloc_len)
        return;

    p = pbuffer + offset;
    while (left) {
        oneline = left > 0x10 ? 0x10 : left; /* 16 items per 1 line */
        pp += sprintf(pp, "0x%04lx: ", index);
        for (i = 0; i < oneline; i++)
            pp += sprintf(pp, " %02x", *(p + i));
        pp += sprintf(pp, "\n");
        *pp = '\0';

        index += 0x10;
        p += oneline;
        left -= oneline;

        pp = &prbuffer[0];
        LOGD("%s", pp);
    }
}

/* end of component methods & helpers */

/*
 * omx header manipuation
 */
void ComponentBase::SetTypeHeader(OMX_PTR type, OMX_U32 size)
{
    OMX_U32 *nsize;
    OMX_VERSIONTYPE *nversion;

    if (!type)
        return;

    nsize = (OMX_U32 *)type;
    nversion = (OMX_VERSIONTYPE *)((OMX_U8 *)type + sizeof(OMX_U32));

    *nsize = size;
    nversion->nVersion = OMX_SPEC_VERSION;
}

OMX_ERRORTYPE ComponentBase::CheckTypeHeader(const OMX_PTR type, OMX_U32 size)
{
    OMX_U32 *nsize;
    OMX_VERSIONTYPE *nversion;

    if (!type)
        return OMX_ErrorBadParameter;

    nsize = (OMX_U32 *)type;
    nversion = (OMX_VERSIONTYPE *)((OMX_U8 *)type + sizeof(OMX_U32));

    if (*nsize != size)
        return OMX_ErrorBadParameter;

    if (nversion->nVersion != OMX_SPEC_VERSION)
        return OMX_ErrorVersionMismatch;

    return OMX_ErrorNone;
}

/*
 * query_roles helper
 */
OMX_ERRORTYPE ComponentBase::QueryRolesHelper(
    OMX_U32 nr_comp_roles,
    const OMX_U8 **comp_roles,
    OMX_U32 *nr_roles, OMX_U8 **roles)
{
    OMX_U32 i;

    if (!roles) {
        *nr_roles = nr_comp_roles;
        return OMX_ErrorNone;
    }

    if (!nr_roles || (*nr_roles != nr_comp_roles) || !roles)
        return OMX_ErrorBadParameter;

    for (i = 0; i < nr_comp_roles; i++) {
        if (!roles[i])
            break;

        strncpy((OMX_STRING)&roles[i][0],
                (const OMX_STRING)&comp_roles[i][0], OMX_MAX_STRINGNAME_SIZE);
    }

    if (i != nr_comp_roles)
        return OMX_ErrorBadParameter;

    *nr_roles = nr_comp_roles;
    return OMX_ErrorNone;
}

/* end of ComponentBase */
