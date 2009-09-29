/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __CMODULE_H
#define __CMODULE_H

#include <module.h>

class CModule;

typedef OMX_ERRORTYPE (*cmodule_instantiate_t)(OMX_PTR *);
typedef OMX_ERRORTYPE (*cmodule_query_name_t)(OMX_STRING *);
typedef OMX_ERRORTYPE (*cmodule_query_roles_t)(OMX_U32 *, OMX_U8 **);

class CModule {
 public:
    CModule(const OMX_STRING lname);
    ~CModule();

    /*
     * library loading / unloading
     */
    OMX_ERRORTYPE Load(void);
    OMX_ERRORTYPE Unload(void);

    /* end of library loading / unloading */

    /*
     * accessor
     */
    /* library name */
    const OMX_STRING GetLibraryName(void);

    /* end of accessor */

 private:
    /*
     * library symbol
     */
    cmodule_instantiate_t instantiate;
    cmodule_query_name_t query_name;
    cmodule_query_roles_t query_roles;

    /* end of library symbol */

    /* library name */
    char lname[OMX_MAX_STRINGNAME_SIZE];
    /* utils::module */
    struct module *module;
};

#endif /* __CMODULE_H */
