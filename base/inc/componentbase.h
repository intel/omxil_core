/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __COMPONENTBASE_H
#define __COMPONENTBASE_H

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <cmodule.h>
#include <portbase.h>

#include <queue.h>
#include <workqueue.h>

/* ProcessCmdWork */
struct cmd_s {
    OMX_COMMANDTYPE cmd;
    OMX_U32 param1;
    OMX_PTR cmddata;
};

class CmdHandlerInterface
{
public:
    virtual ~CmdHandlerInterface() {};
    virtual void CmdHandler(struct cmd_s *cmd) = 0;
};

class CmdProcessWork : public WorkableInterface
{
public:
    CmdProcessWork(CmdHandlerInterface *ci);
    ~CmdProcessWork();

    OMX_ERRORTYPE PushCmdQueue(struct cmd_s *cmd);

private:
    struct cmd_s *PopCmdQueue(void);

    virtual void Work(void); /* call ci->CmdHandler() */

    void ScheduleIfAvailable(void);

    WorkQueue *workq;

    struct queue q;
    pthread_mutex_t lock;

    CmdHandlerInterface *ci; /* to run ComponentBase::CmdHandler() */
};

class ComponentBase : public CmdHandlerInterface
{
public:
    /*
     * constructor & destructor
     */
    ComponentBase();
    ComponentBase(const OMX_STRING name);
    virtual ~ComponentBase();

    /*
     * accessor
     */
    /* name */
    void SetName(const OMX_STRING name);
    const OMX_STRING GetName(void);

    /* cmodule */
    void SetCModule(CModule *cmodule);
    CModule *GetCModule(void);

    /* end of accessor */

    /*
     * core methods & helpers
     */
    /* roles */
    OMX_ERRORTYPE SetRolesOfComponent(OMX_U32 nr_roles, const OMX_U8 **roles);
    OMX_ERRORTYPE GetRolesOfComponent(OMX_U32 *nr_roles, OMX_U8 **roles);
    bool QueryHavingThisRole(const OMX_STRING role);

    /* GetHandle & FreeHandle */
    OMX_ERRORTYPE GetHandle(OMX_HANDLETYPE* pHandle,
                            OMX_PTR pAppData,
                            OMX_CALLBACKTYPE *pCallBacks);
    OMX_ERRORTYPE FreeHandle(OMX_HANDLETYPE hComponent);

    /* end of core methods & helpers */

    /*
     * component methods & helpers
     */
    static OMX_ERRORTYPE GetComponentVersion(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID);
    OMX_ERRORTYPE CBaseGetComponentVersion(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STRING pComponentName,
        OMX_OUT OMX_VERSIONTYPE* pComponentVersion,
        OMX_OUT OMX_VERSIONTYPE* pSpecVersion,
        OMX_OUT OMX_UUIDTYPE* pComponentUUID);

    static OMX_ERRORTYPE SendCommand(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);
    OMX_ERRORTYPE CBaseSendCommand(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_COMMANDTYPE Cmd,
        OMX_IN  OMX_U32 nParam1,
        OMX_IN  OMX_PTR pCmdData);

    static OMX_ERRORTYPE GetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE CBaseGetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nParamIndex,
        OMX_INOUT OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE SetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);
    OMX_ERRORTYPE CBaseSetParameter(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentParameterStructure);

    static OMX_ERRORTYPE GetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE CBaseGetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_INOUT OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE SetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);
    OMX_ERRORTYPE CBaseSetConfig(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_INDEXTYPE nIndex,
        OMX_IN  OMX_PTR pComponentConfigStructure);

    static OMX_ERRORTYPE GetExtensionIndex(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);
    OMX_ERRORTYPE CBaseGetExtensionIndex(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_STRING cParameterName,
        OMX_OUT OMX_INDEXTYPE* pIndexType);

    static OMX_ERRORTYPE GetState(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState);
    OMX_ERRORTYPE CBaseGetState(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_STATETYPE* pState);

    static OMX_ERRORTYPE ComponentTunnelRequest(
        OMX_IN  OMX_HANDLETYPE hComp,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);
    OMX_ERRORTYPE CBaseComponentTunnelRequest(
        OMX_IN  OMX_HANDLETYPE hComp,
        OMX_IN  OMX_U32 nPort,
        OMX_IN  OMX_HANDLETYPE hTunneledComp,
        OMX_IN  OMX_U32 nTunneledPort,
        OMX_INOUT  OMX_TUNNELSETUPTYPE* pTunnelSetup);

    static OMX_ERRORTYPE UseBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);
    OMX_ERRORTYPE CBaseUseBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes,
        OMX_IN OMX_U8* pBuffer);

    static OMX_ERRORTYPE AllocateBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);
    OMX_ERRORTYPE CBaseAllocateBuffer(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBuffer,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN OMX_U32 nSizeBytes);

    static OMX_ERRORTYPE FreeBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseFreeBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_U32 nPortIndex,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE EmptyThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseEmptyThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE FillThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);
    OMX_ERRORTYPE CBaseFillThisBuffer(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE SetCallbacks(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);
    OMX_ERRORTYPE CBaseSetCallbacks(
        OMX_IN  OMX_HANDLETYPE hComponent,
        OMX_IN  OMX_CALLBACKTYPE* pCallbacks,
        OMX_IN  OMX_PTR pAppData);

    static OMX_ERRORTYPE ComponentDeInit(
        OMX_IN  OMX_HANDLETYPE hComponent);
    OMX_ERRORTYPE CBaseComponentDeInit(
        OMX_IN  OMX_HANDLETYPE hComponent);

    static OMX_ERRORTYPE UseEGLImage(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN void* eglImage);
    OMX_ERRORTYPE CBaseUseEGLImage(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_INOUT OMX_BUFFERHEADERTYPE** ppBufferHdr,
        OMX_IN OMX_U32 nPortIndex,
        OMX_IN OMX_PTR pAppPrivate,
        OMX_IN void* eglImage);

    static OMX_ERRORTYPE ComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);
    OMX_ERRORTYPE CBaseComponentRoleEnum(
        OMX_IN OMX_HANDLETYPE hComponent,
        OMX_OUT OMX_U8 *cRole,
        OMX_IN OMX_U32 nIndex);

    /* end of component methods & helpers */

protected:
    /*
     * omx header manipuation
     */
    void SetTypeHeader(OMX_PTR type, OMX_U32 size);
    OMX_BOOL CheckTypeHeader(OMX_PTR type, OMX_U32 size);

    /* end of omx header manipuation */

    /* ports */
    /*
     * allocated with derived port classes by derived component classes
     */
    PortBase **ports;
    OMX_U32 nr_ports;
    OMX_PORT_PARAM_TYPE portparam;

private:
    /* common routines for constructor */
    void __ComponentBase(void);

    /*
     * core methods & helpers
     */
    /* called in Get/FreeHandle() */
    virtual OMX_ERRORTYPE InitComponent(void) = 0;
    virtual OMX_ERRORTYPE ExitComponent(void) = 0;

    /* end of core methods & helpers */

    /*
     * component methods & helpers
     */
    /* SendCommand */
    /* implement CmdHandlerInterface */
    virtual void CmdHandler(struct cmd_s *cmd);

    /* Get/SetParameter */
    virtual OMX_ERRORTYPE
        ComponentGetParameter(OMX_INDEXTYPE nParamIndex,
                              OMX_PTR pComponentParameterStructure) = 0;
    virtual OMX_ERRORTYPE
        ComponentSetParameter(OMX_INDEXTYPE nIndex,
                              OMX_PTR pComponentParameterStructure) = 0;

    /* Get/SetConfig */
    virtual OMX_ERRORTYPE
        ComponentGetConfig(OMX_INDEXTYPE nIndex,
                           OMX_PTR pComponentConfigStructure) = 0;
    virtual OMX_ERRORTYPE
        ComponentSetConfig(OMX_INDEXTYPE nIndex,
                           OMX_PTR pComponentConfigStructure) = 0;

    /* end of component methods & helpers */

    /* process component's commands work */
    CmdProcessWork *cmdwork;

    /* roles */
    OMX_U8 **roles;
    OMX_U32 nr_roles;

    /* component module */
    CModule *cmodule;

    /* omx standard handle */
    /* allocated at GetHandle, freed at FreeHandle */
    OMX_COMPONENTTYPE *handle;

    /* omx standard callbacks */
    OMX_PTR appdata;
    OMX_CALLBACKTYPE *callbacks;

    /* component name */
    char name[OMX_MAX_STRINGNAME_SIZE];

    /* omx specification version */
    const static OMX_U8 OMX_SPEC_VERSION_MAJOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_MINOR = 1;
    const static OMX_U8 OMX_SPEC_VERSION_REVISION = 0;
    const static OMX_U8 OMX_SPEC_VERSION_STEP = 0;

    const static OMX_U32 OMX_SPEC_VERSION = 0
        | (OMX_SPEC_VERSION_MAJOR << 24)
        | (OMX_SPEC_VERSION_MINOR << 16)
        | (OMX_SPEC_VERSION_REVISION << 8)
        | (OMX_SPEC_VERSION_STEP << 0);
};
#endif
