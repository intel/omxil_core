/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#ifndef __MODULE_H
#define __MODULE_H

#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

struct module;

typedef int (*module_init_t)(struct module *module);
typedef void (*module_exit_t)(struct module *module);

struct module {
    char *name;

    int ref_count;
    void *handle;

    module_init_t init;
    module_exit_t exit;

    void *priv;
    struct module *next;
};

#define MODULE_LAZY RTLD_LAZY
#define MODULE_NOW RTLD_NOW
#define MODUEL_GLOBAL RTLD_GLOBAL

struct module *module_open(const char *path, int flag);
int module_close(struct module *module);
void *module_symbol(struct module *module, const char *string);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __MODULE_H */
