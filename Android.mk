#
# Copyright (c) 2009-2010 Wind River Systems, Inc.
#
# The right to copy, distribute, modify, or otherwise make use
# of this software may be licensed only pursuant to the terms
# of an applicable Wind River license agreement.
#

ifeq ($(strip $(BOARD_USES_WRS_OMXIL_CORE)),true)

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

endif
