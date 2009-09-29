/*
 * Copyright (C) 2009 Wind River Systems.
 */

#ifndef __CMODULE_H
#define __CMODULE_H

#include <module.h>

class CModule;

typedef OMX_ERRORTYPE (*cmodule_init_t)(CModule *);
typedef OMX_ERRORTYPE (*cmodule_exit_t)(CModule *);

class CModule {
 public:
    CModule(const OMX_STRING name);
    ~CModule();

    OMX_ERRORTYPE Load(void);
    OMX_ERRORTYPE Unload(void);

    void SetName(const OMX_STRING name);
    const OMX_STRING GetName(void);
    void SetPrivData(OMX_PTR privdata);
    OMX_PTR GetPrivData(void);

 private:
    cmodule_init_t init;
    cmodule_exit_t exit;

    char name[OMX_MAX_STRINGNAME_SIZE];
    struct module *module;

    OMX_PTR privdata;
};

#endif /* __CMODULE_H */
