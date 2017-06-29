LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := cabc_test.c
LOCAL_MODULE := cabc_test
LOCAL_C_INCLUDES := \
	external/libdrm \
	external/libdrm/include/drm

LOCAL_CFLAGS := -O2 -g -W -Wall
LOCAL_SYSTEM_SHARED_LIBRARIES := libc libm
LOCAL_SHARED_LIBRARIES := libdrm

include $(BUILD_EXECUTABLE)

