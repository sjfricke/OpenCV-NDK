//
// Created by fricke on 8/20/17.
//

#ifndef OPENCV_NDK_UTIL_H
#define OPENCV_NDK_UTIL_H

#include <android/log.h>

// used to get logcat outputs which can be regex filtered by the LOG_TAG we give
// So in Logcat you can filter this example by putting OpenCV-NDK
#define LOG_TAG "OpenCV-NDK-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define ASSERT(cond, fmt, ...)                                \
  if (!(cond)) {                                              \
    __android_log_assert(#cond, LOG_TAG, fmt, ##__VA_ARGS__); \
  }

#endif  // OPENCV_NDK_UTIL_H
