/*
 * Copyright (C) 2009 Wind River Systems.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <OMX_Core.h>

#include <cmodule.h>

#define LOG_TAG "cmodule"
#include <log.h>

CModule::CModule(const OMX_STRING lname)
{
    memset(lname, 0, OMX_MAX_STRINGNAME_SIZE);

    module = NULL;
    privdata = NULL;

    strncpy(this->lname, lname, OMX_MAX_STRINGNAME_SIZE);
    lname[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
}

CModule::~CModule()
{
    Unload();
}

OMX_ERRORTYPE CModule::Load()
{
    struct module *m;

    m = module_open(lname, MODULE_NOW);
    if (!m) {
        LOGE("module not founded (%s)\n", lname);
        return OMX_ErrorComponentNotFound;
    }
    LOGV("module founded (%s)\n", lname);

    if (!module) {
        module = m;

        LOGV("module %s successfully loaded\n", lname);
    }
    else {
        if (module != m) {
            LOGE("module fatal error, module != m (%p != %p)\n", module, m);
            module_close(m);
            return OMX_ErrorUndefined;
        }
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE CModule::Unload(void)
{
    OMX_U32 ref_count;

    if (!module)
        return OMX_ErrorNone;

    ref_count = module_close(module);
    if (!ref_count) {
        module = NULL;
        LOGV("module %s successfully unloaded\n", lname);
    }

    return OMX_ErrorNone;
}

const OMX_STRING CModule::GetLibraryName(void)
{
    return lname;
}

void CModule::SetPrivData(OMX_PTR privdata)
{
    this->privdata = privdata;
}

OMX_PTR CModule::GetPrivData(void)
{
    return privdata;
}
