/*
 * Copyright (C) Wind River Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <list.h>
#include <cmodule.h>
#include <componentbase.h>

#define LOG_TAG "wrs_omxcore"
#include <log.h>

static unsigned int g_initialized = 0;

static struct list *g_module_list = NULL;
static pthread_mutex_t g_module_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list *construct_components(const char *config_file_name)
{
    FILE *config_file;
    char library_name[OMX_MAX_STRINGNAME_SIZE];
    char config_file_path[256];
    struct list *head = NULL;

    strncpy(config_file_path, "/etc/", 256);
    strncat(config_file_path, config_file_name, 256);
    config_file = fopen(config_file_path, "r");
    if (!config_file) {
        strncpy(config_file_path, "./", 256);
        strncat(config_file_path, config_file_name, 256);
        config_file = fopen(config_file_path, "r");
        if (!config_file) {
            LOGE("not found file %s\n", config_file_name);
            return NULL;
        }
    }

    while (fscanf(config_file, "%s", library_name) > 0) {
        CModule *cmodule;
        struct list *entry;
        OMX_ERRORTYPE ret;

        library_name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
        cmodule = new CModule(&library_name[0]);
        if (!cmodule)
            continue;

        LOGV("found component library %s\n", library_name);

        ret = cmodule->Load();
        if (ret != OMX_ErrorNone)
            goto delete_cmodule;

        ret = cmodule->QueryComponentName();
        if (ret != OMX_ErrorNone)
            goto unload_cmodule;

        ret = cmodule->QueryComponentRoles();
        if (ret != OMX_ErrorNone)
            goto unload_cmodule;

        entry = list_alloc(cmodule);
        if (!entry)
            goto unload_cmodule;
        head = __list_add_tail(head, entry);

        cmodule->Unload();
        LOGV("module %s:%s added to component list\n",
             cmodule->GetLibraryName(), cmodule->GetComponentName());

        continue;

    unload_cmodule:
        cmodule->Unload();
    delete_cmodule:
        delete cmodule;
    }

    fclose(config_file);
    return head;
}

static struct list *destruct_components(struct list *head)
{
    struct list *entry, *next;

    list_foreach_safe(head, entry, next) {
        CModule *cmodule = static_cast<CModule *>(entry->data);

        head = __list_delete(head, entry);
        delete cmodule;
    }

    return head;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    int ret;

    pthread_mutex_lock(&g_module_lock);
    if (!g_initialized) {
        g_module_list = construct_components("wrs_omxil_components.cfg");
        if (!g_module_list) {
            pthread_mutex_unlock(&g_module_lock);
            return OMX_ErrorInsufficientResources;
        }

        g_initialized = 1;
    }
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    pthread_mutex_lock(&g_module_lock);
    destruct_components(g_module_list);
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN OMX_U32 nNameLength,
    OMX_IN OMX_U32 nIndex)
{
    CModule *cmodule;
    ComponentBase *cbase;
    struct list *entry;
    OMX_STRING cname;

    pthread_mutex_lock(&g_module_lock);
    entry = __list_entry(g_module_list, nIndex);
    if (!entry) {
        pthread_mutex_unlock(&g_module_lock);
        return OMX_ErrorNoMore;
    }
    pthread_mutex_unlock(&g_module_lock);

    cmodule = static_cast<CModule *>(entry->data);
    cname = cmodule->GetComponentName();

    strncpy(cComponentName, cname, nNameLength);
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_CALLBACKTYPE *pCallBacks)
{
    struct list *entry;
    OMX_ERRORTYPE ret;

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        OMX_STRING cname;

        cmodule = static_cast<CModule *>(entry->data);

        cname = cmodule->GetComponentName();
        if (!strcmp(cComponentName, cname)) {
            ComponentBase *cbase = NULL;
            
            ret = cmodule->Load();
            if (ret != OMX_ErrorNone)
                break;

            ret = cmodule->InstantiateComponent(&cbase);
            if (ret != OMX_ErrorNone) {
                cmodule->Unload();
                break;
            }
            cbase->SetCModule(cmodule);

            pthread_mutex_unlock(&g_module_lock);
            return cbase->GetHandle(pHandle, pAppData, pCallBacks);
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorInvalidComponent;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    ComponentBase *cbase;
    CModule *cmodule;
    OMX_ERRORTYPE ret;

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    ret = cbase->FreeHandle(hComponent);
    if (ret != OMX_ErrorNone) {
        LOGE("failed to cbase->FreeHandle() (ret = 0x%08x)\n", ret);
        return ret;
    }

    cmodule = cbase->GetCModule();
    if (!cmodule) {
        LOGE("fatal error, %s does not have cmodule\n", cbase->GetName());
        goto out;
    }
    cmodule->Unload();

out:
    delete cbase;
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_IN OMX_HANDLETYPE hOutput,
    OMX_IN OMX_U32 nPortOutput,
    OMX_IN OMX_HANDLETYPE hInput,
    OMX_IN OMX_U32 nPortInput)
{
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE   OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN OMX_STRING szURI)
{
    return OMX_ErrorNotImplemented;
}

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_IN OMX_STRING role,
    OMX_INOUT OMX_U32 *pNumComps,
    OMX_INOUT OMX_U8  **compNames)
{
    struct list *entry;
    OMX_U32 nr_comps = 0, copied_nr_comps = 0;

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        OMX_STRING cname;
        bool having_role;

        cmodule = static_cast<CModule *>(entry->data);

        having_role = cmodule->QueryHavingThisRole(role);
        if (having_role) {
            if (compNames && compNames[nr_comps]) {
                cname = cmodule->GetComponentName();
                strncpy((OMX_STRING)&compNames[nr_comps][0], cname,
                        OMX_MAX_STRINGNAME_SIZE);
                copied_nr_comps++;
            }
            nr_comps++;
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    if (!copied_nr_comps)
        *pNumComps = nr_comps;
    else {
        if (copied_nr_comps != *pNumComps)
            return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_IN OMX_STRING compName,
    OMX_INOUT OMX_U32 *pNumRoles,
    OMX_OUT OMX_U8 **roles)
{
    struct list *entry;

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        ComponentBase *cbase;
        OMX_STRING cname;

        cmodule = static_cast<CModule *>(entry->data);

        cname = cmodule->GetComponentName();
        if (!strcmp(compName, cname)) {
            pthread_mutex_unlock(&g_module_lock);
            return cmodule->GetComponentRoles(pNumRoles, roles);
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorInvalidComponent;
}
