#ifndef OMX_SYSDEPS_H_
#define OMX_SYSDEPS_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef ANDROID

#include <utils/Log.h>

#ifdef ANDROID_ALOG
#define omx_errorLog(...) ALOGE(__VA_ARGS__);
#define omx_infoLog(...) ALOGI(__VA_ARGS__);
#define omx_verboseLog(...) ALOGV(__VA_ARGS__);
#define omx_warnLog(...) ALOGV(__VA_ARGS__);
#define omx_debugLog(...) ALOGV(__VA_ARGS__);
#elif ANDROID_LOG
#define omx_errorLog(...) LOGE(__VA_ARGS__);
#define omx_infoLog(...) LOGI(__VA_ARGS__);
#define omx_verboseLog(...) LOGV(__VA_ARGS__);
#define omx_warnLog(...) LOGV(__VA_ARGS__);
#define omx_debugLog(...) LOGV(__VA_ARGS__);
#endif
#else

#define omx_errorLog(...) do { \
	fprintf(stderr, "omxil-core error: %s", __VA_ARGS__); \
}while (0)

#define omx_infoLog(...) do { \
	fprintf(stderr, "omxil-core info: %s", __VA_ARGS__); \
}while (0)

#define omx_verboseLog(...) do { \
	fprintf(stderr, "omxil-core verbose: %s", __VA_ARGS__); \
}while (0)

#define omx_warnLog(...) do { \
	fprintf(stderr, "omxil-core warning: %s", __VA_ARGS__); \
}while (0)

#define omx_debugLog(...) do { \
	fprintf(stderr, "omxil-core debug: %s", __VA_ARGS__); \
}while (0)

#endif //ANDROID
#endif//OMX_SYSDEPS_H_

