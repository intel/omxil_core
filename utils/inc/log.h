/*
 * Copyright (C) 2009 Wind River Systems
 *      Author: Ho-Eun Ryu <ho-eun.ryu@windriver.com>
 */

#ifndef __LOG_H
#define __LOG_H

#ifdef ANDROID
 #include <cutils/log.h>
#else
 #include <stdio.h>
 #define LOG(_p, ...) \
      fprintf(stderr, _p "/" LOG_TAG ": " __VA_ARGS__)
 #define LOGV(...)   LOG("V", __VA_ARGS__)
 #define LOGD(...)   LOG("D", __VA_ARGS__)
 #define LOGI(...)   LOG("I", __VA_ARGS__)
 #define LOGW(...)   LOG("W", __VA_ARGS__)
 #define LOGE(...)   LOG("E", __VA_ARGS__)
#endif

#endif /* __LOG_H */
