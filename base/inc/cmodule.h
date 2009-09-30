/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __CMODULE_H
#define __CMODULE_H

#include <module.h>

class CModule;
class ComponentBase;

typedef OMX_ERRORTYPE (*cmodule_instantiate_t)(OMX_PTR *);
typedef OMX_ERRORTYPE (*cmodule_query_name_t)(OMX_STRING, OMX_U32);
typedef OMX_ERRORTYPE (*cmodule_query_roles_t)(OMX_U32 *, OMX_U8 **);

class CModule {
 public:
    CModule(const OMX_STRING lname);
    ~CModule();

    /*
     * library loading / unloading
     */
    OMX_ERRORTYPE Load(void);
    OMX_U32 Unload(void);

    /* end of library loading / unloading */

    /*
     * accessor
     */
    /* library name */
    const OMX_STRING GetLibraryName(void);

    /* component name and roles */
    const OMX_STRING GetComponentName(void);
    OMX_ERRORTYPE GetComponentRoles(OMX_U32 *nr_roles, OMX_U8 **roles);

    bool QueryHavingThisRole(const OMX_STRING role);

    /* end of accessor */

    /* library symbol method and helpers */
    /* call instantiate / query_name / query_roles */
    OMX_ERRORTYPE QueryComponentName(void);
    OMX_ERRORTYPE QueryComponentName(OMX_STRING cname, OMX_U32 len);
    OMX_ERRORTYPE QueryComponentRoles(void);
    OMX_ERRORTYPE QueryComponentRoles(OMX_U32 *nr_roles, OMX_U8 **roles);
    OMX_ERRORTYPE InstantiateComponent(ComponentBase **instance);

    /* end of library symbol method and helpers */

 private:
    /*
     * library symbol
     */
    cmodule_instantiate_t instantiate;
    cmodule_query_name_t query_name;
    cmodule_query_roles_t query_roles;

    /* end of library symbol */

    /* component name */
    char cname[OMX_MAX_STRINGNAME_SIZE];

    /* component roles */
    OMX_U8 **roles;
    OMX_U32 nr_roles;

    /* library name */
    char lname[OMX_MAX_STRINGNAME_SIZE];
    /* utils::module */
    struct module *module;
};

#endif /* __CMODULE_H */
