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

#if 1
static struct list *construct_components(const char *config_file_name)
{
    FILE *config_file;
    char library_name[OMX_MAX_STRINGNAME_SIZE];
    char config_file_path[256];
    struct list *head = NULL;
    OMX_ERRORTYPE load_ret;

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

        library_name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
        cmodule = new CModule(&library_name[0]);
        if (!cmodule) {
            list_free_all(head);
            fclose(config_file);
            return NULL;
        }
        LOGV("found component library %s\n", library_name);

        load_ret = cmodule->Load();
        if (load_ret == OMX_ErrorNone) {
            struct list *entry;

            entry = list_alloc(cmodule);
            if (entry) {
                entry->data = cmodule;
                head = __list_add_tail(head, entry);
                LOGV("module %s added to component list\n",
                     cmodule->GetName());
            }
            else {
                cmodule->Unload();
                delete cmodule;
            }
        }
        else
            delete cmodule;
    }

    fclose(config_file);
    return head;
}
#else
static const OMX_STRING g_components_list[] = {
    "libomx_mrst_mp3decoder.so",
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static struct list *construct_components(const char *conf_file)
{
    struct list *head = NULL;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(g_components_list); i++) {
        CModule *cmodule;
        OMX_ERRORTYPE load_ret;

        cmodule = new CModule(&g_components_list[i][0]);
        if (!cmodule) {
            list_free_all(head);
            return NULL;
        }

        load_ret = cmodule->Load();
        if (load_ret == OMX_ErrorNone) {
            struct list *entry;

            entry = list_alloc(cmodule);
            if (entry) {
                entry->data = cmodule;
                head = __list_add_tail(head, entry);
                LOGV("module %s added to component list\n",
                     &g_components_list[i][0]);
            }
            else {
                cmodule->Unload();
                delete cmodule;
            }
        }
        else {
            delete cmodule;
        }
    }

    return head;
}
#endif

static void destruct_components(struct list *head)
{
    struct list *entry, *next;

    list_foreach_safe(head, entry, next) {
        CModule *cmodule = static_cast<CModule *>(entry->data);
        OMX_ERRORTYPE unload_ret;

        head = __list_delete(head, entry);

        unload_ret = cmodule->Unload();
        delete cmodule;
    }

    g_module_list = head;
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
    cbase = static_cast<ComponentBase *>(cmodule->GetPrivData());

    cname = cbase->GetName();
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

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        ComponentBase *cbase;
        OMX_STRING name;

        cmodule = static_cast<CModule *>(entry->data);
        cbase = static_cast<ComponentBase *>(cmodule->GetPrivData());

        name = cbase->GetName();
        if (!strcmp(cComponentName, name)) {
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

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase)
        return OMX_ErrorBadParameter;

    return cbase->FreeHandle(hComponent);
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
        ComponentBase *cbase;
        OMX_STRING cname;
        bool having_role;

        cmodule = static_cast<CModule *>(entry->data);
        cbase = static_cast<ComponentBase *>(cmodule->GetPrivData());

        having_role = cbase->QueryHavingThisRole(role);
        if (having_role) {
            if (compNames && compNames[nr_comps]) {
                cname = cbase->GetName();
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
        cbase = static_cast<ComponentBase *>(cmodule->GetPrivData());

        cname = cbase->GetName();
        if (!strcmp(compName, cname)) {
            pthread_mutex_unlock(&g_module_lock);
            return cbase->GetRolesOfComponent(pNumRoles, roles);
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorInvalidComponent;
}
