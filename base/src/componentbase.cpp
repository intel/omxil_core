/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <componentbase.h>

#define LOG_TAG "componentbase"
#include <log.h>

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
    if (roles) {
        OMX_U32 i;

        for (i = 0; i < nr_roles; i++)
            free(roles[i]);

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

    this->roles = (OMX_U8 **)malloc(sizeof(OMX_STRING) * nr_roles);
    if (!this->roles)
        return OMX_ErrorInsufficientResources;

    for (i = 0; i < nr_roles; i++) {
        this->roles[i] = (OMX_U8 *)malloc(OMX_MAX_STRINGNAME_SIZE);
        if (!this->roles[i]) {
            int j;

            for (j = (int )i-1; j >= 0; j--)
                free(this->roles[j]);
            free(this->roles);

            return OMX_ErrorInsufficientResources;
        }

        strncpy((OMX_STRING)&this->roles[i][0],
                (const OMX_STRING)&roles[i][0],
                OMX_MAX_STRINGNAME_SIZE);
    }

    this->nr_roles = nr_roles;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE ComponentBase::GetRolesOfComponent(OMX_U32 *nr_roles,
                                                 OMX_U8 **roles)
{
    OMX_U32 i;
    OMX_U32 this_nr_roles = this->nr_roles;

    if (!roles) {
        *nr_roles = this_nr_roles;
        return OMX_ErrorNone;
    }

    if (!nr_roles || (*nr_roles != this_nr_roles))
        return OMX_ErrorBadParameter;

    for (i = 0; i < this_nr_roles; i++) {
        if (!roles[i])
            break;

        if (roles && roles[i])
            strncpy((OMX_STRING)&roles[i][0],
                    (const OMX_STRING)&this->roles[i][0],
                    OMX_MAX_STRINGNAME_SIZE);
    }

    if (i != this_nr_roles)
        return OMX_ErrorBadParameter;

    *nr_roles = this_nr_roles;
    return OMX_ErrorNone;
}

bool ComponentBase::QueryHavingThisRole(const OMX_STRING role)
{
    OMX_U32 i;

    if (!roles || !role)
        return false;

    for (i = 0; i < nr_roles; i++) {
        if (!strcmp((OMX_STRING)&roles[i][0], role))
            return true;
    }

    return false;
}

/* GetHandle & FreeHandle */
OMX_ERRORTYPE ComponentBase::GetHandle(OMX_HANDLETYPE *pHandle,
                                       OMX_PTR pAppData,
                                       OMX_CALLBACKTYPE *pCallBacks)
{
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

    /* virtual - see derived class */
    ret = InitComponent();
    if (ret != OMX_ErrorNone) {
        LOGE("failed to %s::InitComponent(), ret = 0x%08x\n",
             name, ret);
        goto free_handle;
    }

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
    *pHandle = (OMX_HANDLETYPE *)handle;

    return OMX_ErrorNone;

free_handle:
    free(this->handle);
    this->handle = NULL;

    return ret;
}

OMX_ERRORTYPE ComponentBase::FreeHandle(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE ret;

    if (hComponent != handle)
        return OMX_ErrorBadParameter;

    /* virtual - see derived class */
    ret = ExitComponent();
    if (ret != OMX_ErrorNone)
        return ret;

    free(handle);

    appdata = NULL;
    callbacks = NULL;

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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
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
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes,
    OMX_IN OMX_U8* pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::AllocateBuffer(
    OMX_IN OMX_HANDLETYPE hComponent,
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
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
    OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
    OMX_IN OMX_U32 nPortIndex,
    OMX_IN OMX_PTR pAppPrivate,
    OMX_IN OMX_U32 nSizeBytes)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::FreeBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_U32 nPortIndex,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
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
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
}

OMX_ERRORTYPE ComponentBase::FillThisBuffer(
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

    return cbase->CBaseFillThisBuffer(hComponent, pBuffer);
}

OMX_ERRORTYPE ComponentBase::CBaseFillThisBuffer(
    OMX_IN  OMX_HANDLETYPE hComponent,
    OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer)
{
    /*
     * Todo
     */

    return OMX_ErrorNotImplemented;
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

    *nsize = sizeof(size);
    nversion->nVersion = OMX_SPEC_VERSION;
}

OMX_BOOL ComponentBase::CheckTypeHeader(OMX_PTR type, OMX_U32 size)
{
    OMX_U32 *nsize;
    OMX_VERSIONTYPE *nversion;
    OMX_U8 mismatch = 0;

    if (!type)
        return OMX_FALSE;

    nsize = (OMX_U32 *)type;
    nversion = (OMX_VERSIONTYPE *)((OMX_U8 *)type + sizeof(OMX_U32));

    mismatch = (*nsize != sizeof(size)) ? 1 : 0;
    mismatch |= (nversion->nVersion != OMX_SPEC_VERSION) ? 1 : 0;

    if (mismatch)
        return OMX_TRUE;
    else
        return OMX_FALSE;
}
