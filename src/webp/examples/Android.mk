LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    example_util.c \

LOCAL_CFLAGS := $(WEBP_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src

LOCAL_MODULE := example_util

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

# Note: to enable jpeg/png encoding the sources from AOSP can be used with
# minor modification to their Android.mk files.
LOCAL_SRC_FILES := \
    cwebp.c \
    jpegdec.c \
    metadata.c \
    pngdec.c \
    tiffdec.c \
    webpdec.c \

LOCAL_CFLAGS := $(WEBP_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src
LOCAL_STATIC_LIBRARIES := example_util webp

LOCAL_MODULE := cwebp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    dwebp.c \

LOCAL_CFLAGS := $(WEBP_CFLAGS)
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../src
LOCAL_STATIC_LIBRARIES := example_util webp

LOCAL_MODULE := dwebp

include $(BUILD_EXECUTABLE)
