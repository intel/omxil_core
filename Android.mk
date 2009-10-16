LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

WRS_OMXIL_CORE_ROOT := $(LOCAL_PATH)

$(call add-prebuilt-files, ETC, 10_wrs_omxil_core.cfg)

# core
-include $(WRS_OMXIL_CORE_ROOT)/core/src/Android.mk

# base class
-include $(WRS_OMXIL_CORE_ROOT)/base/src/Android.mk

# utility
-include $(WRS_OMXIL_CORE_ROOT)/utils/src/Android.mk
