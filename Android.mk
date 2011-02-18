ifeq ($(strip $(BOARD_USES_WRS_OMXIL_CORE)),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := 10_wrs_omxil_core.cfg
LOCAL_SRC_FILES := 10_wrs_omxil_core.cfg
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

WRS_OMXIL_CORE_ROOT := $(LOCAL_PATH)

COMPONENT_SUPPORT_BUFFER_SHARING := false
COMPONENT_SUPPORT_OPENCORE := false

# core
-include $(WRS_OMXIL_CORE_ROOT)/core/src/Android.mk

# base class
-include $(WRS_OMXIL_CORE_ROOT)/base/src/Android.mk

# utility
-include $(WRS_OMXIL_CORE_ROOT)/utils/src/Android.mk

endif
