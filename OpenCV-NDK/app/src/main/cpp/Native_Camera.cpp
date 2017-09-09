#include "Native_Camera.h"

Native_Camera::Native_Camera(camera_type type) {

  ACameraMetadata* cameraMetadata = nullptr;
  camera_status_t cameraStatus = ACAMERA_OK;

  m_camera_manager = ACameraManager_create();

  cameraStatus =
      ACameraManager_getCameraIdList(m_camera_manager, &m_camera_id_list);
  ASSERT(cameraStatus == ACAMERA_OK,
         "Failed to get camera id list (reason: %d)", cameraStatus);
  ASSERT(m_camera_id_list->numCameras > 0, "No camera device detected");

  // ASSUMPTION: Back camera is index[0] and front is index[1]
  if (type == BACK_CAMERA) {
    m_selected_camera_id = m_camera_id_list->cameraIds[0];
  } else {
    ASSERT(m_camera_id_list->numCameras > 1, "No dual camera setup");
    m_selected_camera_id= m_camera_id_list->cameraIds[1];
  }

  cameraStatus = ACameraManager_getCameraCharacteristics(
      m_camera_manager, m_selected_camera_id, &cameraMetadata);
  ASSERT(cameraStatus == ACAMERA_OK, "Failed to get camera meta data of ID: %s",
         m_selected_camera_id);

  m_device_state_callbacks.onDisconnected = CameraDeviceOnDisconnected;
  m_device_state_callbacks.onError = CameraDeviceOnError;

  cameraStatus =
      ACameraManager_openCamera(m_camera_manager, m_selected_camera_id,
                                &m_device_state_callbacks, &m_camera_device);
  ASSERT(cameraStatus == ACAMERA_OK, "Failed to open camera device (id: %s)",
         m_selected_camera_id);

  m_camera_orientation = 90;

  // TEMPLATE_RECORD because rather have post-processing quality for more
  // accureate CV algo
  // Frame rate should be good since all image buffers are being done from
  // native side
  cameraStatus = ACameraDevice_createCaptureRequest(
      m_camera_device, TEMPLATE_RECORD, &m_capture_request);
  ASSERT(cameraStatus == ACAMERA_OK,
         "Failed to create preview capture request (id: %s)",
         m_selected_camera_id);

  m_camera_ready = true;
}

Native_Camera::~Native_Camera() {
  if (m_capture_request != nullptr) {
    ACaptureRequest_free(m_capture_request);
    m_capture_request = nullptr;
  }

  if (m_camera_output_target != nullptr) {
    ACameraOutputTarget_free(m_camera_output_target);
    m_camera_output_target = nullptr;
  }

  if (m_camera_device != nullptr) {
    ACameraDevice_close(m_camera_device);
    m_camera_device = nullptr;
  }

  ACaptureSessionOutputContainer_remove(m_capture_session_output_container,
                                        m_session_output);
  if (m_session_output != nullptr) {
    ACaptureSessionOutput_free(m_session_output);
    m_session_output = nullptr;
  }

  if (m_capture_session_output_container != nullptr) {
    ACaptureSessionOutputContainer_free(m_capture_session_output_container);
    m_capture_session_output_container = nullptr;
  }

  ACameraManager_delete(m_camera_manager);
}

bool Native_Camera::MatchCaptureSizeRequest(ImageFormat* resView, int32_t width, int32_t height) {
  Display_Dimension disp(width, height);
  if (m_camera_orientation == 90 || m_camera_orientation == 270) {
    disp.Flip();
  }

  ACameraMetadata* metadata;
  ACameraManager_getCameraCharacteristics(m_camera_manager,
                                          m_selected_camera_id, &metadata);
  ACameraMetadata_const_entry entry;
  ACameraMetadata_getConstEntry(
      metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
  // format of the data: format, width, height, input?, type int32
  bool foundIt = false;
  Display_Dimension foundRes(640, 480);

  for (int i = 0; i < entry.count; ++i) {
    int32_t input = entry.data.i32[i * 4 + 3];
    int32_t format = entry.data.i32[i * 4 + 0];
    if (input) continue;

    if (format == AIMAGE_FORMAT_YUV_420_888) {
      Display_Dimension res(entry.data.i32[i * 4 + 1],
                            entry.data.i32[i * 4 + 2]);

      LOGE("W: %d === H: %d", entry.data.i32[i * 4 + 1],
           entry.data.i32[i * 4 + 2]);
      if (!disp.IsSameRatio(res)) continue;

      LOGE("W: SAME RATIO %d === H: %d", entry.data.i32[i * 4 + 1],
           entry.data.i32[i * 4 + 2]);
      // if (format == AIMAGE_FORMAT_YUV_420_888 && res > foundRes) {
      if (format == AIMAGE_FORMAT_YUV_420_888 &&
          entry.data.i32[i * 4 + 1] == 1280 &&
          entry.data.i32[i * 4 + 2] == 720) {
        foundIt = true;
        foundRes = res;
      }
    }
  }

  if (foundIt) {
    resView->width = foundRes.org_width();
    resView->height = foundRes.org_height();

    LOGE("Selected: W: %d - H: %d", resView->width, resView->height);
  } else {
    LOGE("Did not find any compatible camera resolution");
    if (disp.IsPortrait()) {
      resView->width = backup_width;
      resView->height = backup_height;
    } else {
      resView->width = backup_height ;
      resView->height = backup_width;
    }
  }
  resView->format = AIMAGE_FORMAT_YUV_420_888;
  return foundIt;
}

bool Native_Camera::CreateCaptureSession(ANativeWindow* window) {

  camera_status_t cameraStatus = ACAMERA_OK;

  ACaptureSessionOutputContainer_create(&m_capture_session_output_container);
  ANativeWindow_acquire(window);
  ACaptureSessionOutput_create(window, &m_session_output);
  ACaptureSessionOutputContainer_add(m_capture_session_output_container,
                                     m_session_output);
  ACameraOutputTarget_create(window, &m_camera_output_target);

  // TEMPLATE_RECORD because rather have post-processing quality for more
  // accureate CV algo
  // Frame rate should be good since all image buffers are being done from
  // native side
  cameraStatus = ACameraDevice_createCaptureRequest(m_camera_device,
                                                    TEMPLATE_RECORD, &m_capture_request);
  ASSERT(cameraStatus == ACAMERA_OK,
         "Failed to create preview capture request (id: %s)",
         m_selected_camera_id);

  ACaptureRequest_addTarget(m_capture_request, m_camera_output_target);

  m_capture_session_state_callbacks.onReady = CaptureSessionOnReady;
  m_capture_session_state_callbacks.onActive = CaptureSessionOnActive;
  ACameraDevice_createCaptureSession(
      m_camera_device, m_capture_session_output_container,
      &m_capture_session_state_callbacks, &m_capture_session);

  ACameraCaptureSession_setRepeatingRequest(m_capture_session, nullptr, 1,
                                            &m_capture_request, nullptr);

  return true;
}