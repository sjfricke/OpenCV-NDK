#ifndef OPENCV_NDK_CV_MAIN_H
#define OPENCV_NDK_CV_MAIN_H

// Android
#include <android/log.h>
#include <android/native_window.h>
#include <android/asset_manager.h>
#include <jni.h>
// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
//C Libs
#include <unistd.h>
//STD Libs
#include <string>
#include <vector>
#include <cstdlib>


// used to get logcat outputs which can be regex filtered by the LOG_TAG we give
// So in Logcat you can filter this example by putting OpenCV-NDK
#define LOG_TAG "OpenCV-NDK-Native"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)


class CV_Main {
    public:
    CV_Main();
    ~CV_Main();
    CV_Main(const CV_Main& other) = delete;
    CV_Main& operator=(const CV_Main& other) = delete;

    // Lets us know when app has started passing in VM info
    void OnCreate(JNIEnv* env, jobject caller_activity);

    // Disconnect from service
    void OnPause();

    // Cleanup
    void OnDestroy();

    // Cache the Java VM used from the Java layer.
    void SetJavaVM(JavaVM* pjava_vm) { java_vm = pjava_vm; }

    // sets Surface buffer reference pointer
    void SetNativeWindow(ANativeWindow* native_indow);

    // sets Surface buffer reference pointer
    void SetAssetManager(AAssetManager* asset_manager) { m_aasset_manager = asset_manager; };

    void RunCV();
private:

    // Cached Java VM, caller activity object
    JavaVM* java_vm;
    jobject calling_activity_obj;
    jmethodID on_callback;

    // holds native window to write buffer too
    ANativeWindow* m_native_window;

    // buffer to hold native window when writing to it
    ANativeWindow_Buffer m_native_buffer;

    // the buffer we will write to send to native window
    int32_t* m_frame_buffer;
    int32_t m_frame_height;
    int32_t m_frame_width;
    int32_t m_frame_stride;

    // used to hold reference to assets in assets folder
    AAssetManager* m_aasset_manager;

};

#endif //OPENCV_NDK_CV_MAIN_H
