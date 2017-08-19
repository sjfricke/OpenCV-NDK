LOCAL_PATH := $(call my-dir)
PROJECT_ROOT:= $(call my-dir)/../../../../..

include $(CLEAR_VARS)
LOCAL_MODULE    := libopencv_ndk
LOCAL_CFLAGS    := -Werror -std=c++11
LOCAL_SRC_FILES := native-lib.cpp
LOCAL_LDLIBS    := -llog
include $(BUILD_SHARED_LIBRARY)
