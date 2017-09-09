#ifndef OPENCV_NDK_NATIVE_CAMERA_H
#define OPENCV_NDK_NATIVE_CAMERA_H

#include "Util.h"
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraManager.h>
#include <media/NdkImageReader.h>
#include <android/native_window.h>

enum camera_type { BACK_CAMERA, FRONT_CAMERA };

// TODO - reasonable actions with callbacks
// Camera Callbacks
static void CameraDeviceOnDisconnected(void* context, ACameraDevice* device) {
  LOGI("Camera(id: %s) is diconnected.\n", ACameraDevice_getId(device));
}
static void CameraDeviceOnError(void* context, ACameraDevice* device,
                                int error) {
  LOGE("Error(code: %d) on Camera(id: %s).\n", error,
       ACameraDevice_getId(device));
}
// Capture Callbacks
static void CaptureSessionOnReady(void* context,
                                  ACameraCaptureSession* session) {
  LOGI("Session is ready.\n");
}
static void CaptureSessionOnActive(void* context,
                                   ACameraCaptureSession* session) {
  LOGI("Session is activated.\n");
}

// This class was created since if you want to alternate cameras
// you need to do a proper cleanup and setup and creating a
// separate class was the best OOP move
class Native_Camera {
 public:
  explicit Native_Camera(camera_type type);

  ~Native_Camera();

  bool MatchCaptureSizeRequest(ImageFormat* resView, int32_t width, int32_t height);

  bool CreateCaptureSession(ANativeWindow* window);

  int32_t GetCameraCount() { return m_camera_id_list->numCameras; }
  uint32_t GetOrientation() { return m_camera_orientation; };

 private:

  // Camera variables
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
  ACameraIdList* m_camera_id_list = NULL;
  const char* m_selected_camera_id = NULL;
  bool m_camera_ready;

  int32_t  backup_width = 480;
  int32_t  backup_height = 720;
};

#endif //OPENCV_NDK_NATIVE_CAMERA_H
