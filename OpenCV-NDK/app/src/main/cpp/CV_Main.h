#ifndef OPENCV_NDK_CV_MAIN_H
#define OPENCV_NDK_CV_MAIN_H

// Android
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <jni.h>
// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp>
// OpenCV-NDK App
#include "Image_Reader.h"
#include "Util.h"
// C Libs
#include <unistd.h>
// STD Libs
#include <cstdlib>
#include <string>
#include <vector>

static void CameraDeviceOnDisconnected(void* context, ACameraDevice* device) {
  LOGI("Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}
static void CameraDeviceOnError(void* context, ACameraDevice* device,
                                int error) {
  LOGE("Error(code: %d) on Camera(id: %s).\n", error,
       ACameraDevice_getId(device));
}
static void CaptureSessionOnReady(void* context,
                                  ACameraCaptureSession* session) {
  LOGI("Session is ready.\n");
}
static void CaptureSessionOnActive(void* context,
                                   ACameraCaptureSession* session) {
  LOGI("Session is activated.\n");
}

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
  void SetAssetManager(AAssetManager* asset_manager) {
    m_aasset_manager = asset_manager;
  };

  void SetUpCamera();

  bool MatchCaptureSizeRequest(ImageFormat* resView);

  void CameraLoop();

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

  ACameraDevice* m_camera_device;
  ACaptureRequest* m_capture_request;
  ACameraOutputTarget* m_camera_output_target;
  ACaptureSessionOutput* m_session_output;
  ACaptureSessionOutputContainer* m_capture_session_output_container;
  ACameraCaptureSession* m_capture_session;

  ACameraDevice_StateCallbacks m_device_state_callbacks;
  ACameraCaptureSession_stateCallbacks m_capture_session_state_callbacks;

  ACameraManager* m_camera_manager;
  uint32_t m_camera_orientation;
  const char* m_selected_camera_ID = NULL;

  ImageFormat m_view{0, 0, 0};
  Image_Reader* m_image_reader;

  volatile bool m_camera_ready;
  volatile bool m_found_dim;

  // used to hold reference to assets in assets folder
  AAssetManager* m_aasset_manager;

  int32_t  backup_width = 480;
  int32_t  backup_height = 720;

  void* temp;
  cv::Mat tempMat;
  cv::Mat bufMat;
  cv::String face_cascade_name = "haarcascade_frontalface_alt.xml";
  cv::String eyes_cascade_name = "haarcascade_eye_tree_eyeglasses.xml";
  cv::CascadeClassifier face_cascade;
  cv::CascadeClassifier eyes_cascade;
};

#endif  // OPENCV_NDK_CV_MAIN_H
