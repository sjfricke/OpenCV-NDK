#include "CV_Main.h"

CV_Main::CV_Main()
    : m_camera_ready(false)
    , m_found_dim(false)
    , m_image_reader(nullptr)
{
};

CV_Main::~CV_Main()
{
    // clean up VM and callback handles
    JNIEnv* env;
    java_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    env->DeleteGlobalRef(calling_activity_obj);
    calling_activity_obj = nullptr;

    //ACameraCaptureSession_stopRepeating(m_capture_session);

    if (m_capture_request != NULL) {
        ACaptureRequest_free(m_capture_request);
        m_capture_request = NULL;
    }

    if (m_camera_output_target != NULL) {
        ACameraOutputTarget_free(m_camera_output_target);
        m_camera_output_target = NULL;
    }

    if (m_camera_device != NULL) {
        ACameraDevice_close(m_camera_device);
        m_camera_device = NULL;
    }

    ACaptureSessionOutputContainer_remove(m_capture_session_output_container, m_session_output);
    if (m_session_output != NULL) {
        ACaptureSessionOutput_free(m_session_output);
        m_session_output = NULL;
    }

    if (m_capture_session_output_container != NULL) {
        ACaptureSessionOutputContainer_free(m_capture_session_output_container);
        m_capture_session_output_container = NULL;
    }

    ACameraManager_delete(m_camera_manager);

    // make sure we don't leak native windows
    if (m_native_window != NULL) {
        ANativeWindow_release(m_native_window);
        m_native_window = NULL;
    }

    if (m_image_reader != NULL) {
        delete(m_image_reader);
        m_image_reader = NULL;
    }
}

void CV_Main::OnCreate(JNIEnv* env, jobject caller_activity)
{
    // Need to create an instance of the Java activity
    calling_activity_obj = env->NewGlobalRef(caller_activity);

    // Need to enter package and class to find Java class
    jclass handler_class = env->GetObjectClass(caller_activity);

    // Create function pointeACameraManager_getCameraCharacteristicsr to use for on_loaded callbacks
    //on_callback = env->GetMethodID(handler_class, "JAVA_FUNCTION", "()V");
}

void CV_Main::OnPause()
{
}

void CV_Main::OnDestroy()
{
}

void CV_Main::SetNativeWindow(ANativeWindow* native_window)
{
    // Save native window
    m_native_window = native_window;
}

void CV_Main::SetUpCamera()
{
    ACameraIdList *cameraIdList = NULL;
    ACameraMetadata *cameraMetadata = NULL;

    camera_status_t cameraStatus = ACAMERA_OK;
    m_camera_manager = ACameraManager_create();

    cameraStatus = ACameraManager_getCameraIdList(m_camera_manager, &cameraIdList);
    ASSERT(cameraStatus == ACAMERA_OK, "Failed to get camera id list (reason: %d)", cameraStatus);
    ASSERT(cameraIdList->numCameras > 0, "No camera device detected");

    m_selected_camera_ID = cameraIdList->cameraIds[0];

    cameraStatus = ACameraManager_getCameraCharacteristics(m_camera_manager, m_selected_camera_ID, &cameraMetadata);
    ASSERT(cameraStatus == ACAMERA_OK, "Failed to get camera meta data of ID: %s", m_selected_camera_ID);

    m_device_state_callbacks.onDisconnected = CameraDeviceOnDisconnected;
    m_device_state_callbacks.onError = CameraDeviceOnError;

    cameraStatus = ACameraManager_openCamera(
            m_camera_manager, m_selected_camera_ID, &m_device_state_callbacks, &m_camera_device);
    ASSERT(cameraStatus == ACAMERA_OK, "Failed to open camera device (id: %s)", m_selected_camera_ID);

    LOGI("TEST W: %d --- H: %d", ANativeWindow_getWidth(m_native_window), ANativeWindow_getHeight(m_native_window));

//    m_view.height = 1920;
//    m_view.width = 1080;
//    m_view.format = AIMAGE_FORMAT_YUV_420_888;
    m_camera_orientation = 90;
    MatchCaptureSizeRequest(&m_view);

    ASSERT(m_view.width && m_view.height, "Could not find supportable resolution");

      // Here we set the buffer to use RGBA_8888 as default might be; RGB_565
    ANativeWindow_setBuffersGeometry(m_native_window, m_view.height, m_view.width, WINDOW_FORMAT_RGBA_8888);

    m_image_reader = new Image_Reader(&m_view, AIMAGE_FORMAT_YUV_420_888);
    m_image_reader->SetPresentRotation(m_camera_orientation);

    ANativeWindow* image_reader_window = m_image_reader->GetNativeWindow();

    ACaptureSessionOutputContainer_create(&m_capture_session_output_container);
    ANativeWindow_acquire(image_reader_window);
    ACaptureSessionOutput_create(image_reader_window, &m_session_output);
    ACaptureSessionOutputContainer_add(m_capture_session_output_container, m_session_output);
    ACameraOutputTarget_create(image_reader_window, &m_camera_output_target);

    // TEMPLATE_RECORD because rather have post-processing quality for more accureate CV algo
    // Frame rate should be good since all image buffers are being done from native side
    cameraStatus = ACameraDevice_createCaptureRequest(m_camera_device, TEMPLATE_RECORD, &m_capture_request);
    ASSERT(cameraStatus == ACAMERA_OK, "Failed to create preview capture request (id: %s)", m_selected_camera_ID);

    ACaptureRequest_addTarget(m_capture_request, m_camera_output_target);


    m_capture_session_state_callbacks.onReady = CaptureSessionOnReady;
    m_capture_session_state_callbacks.onActive = CaptureSessionOnActive;
    ACameraDevice_createCaptureSession(m_camera_device, m_capture_session_output_container,
                                       &m_capture_session_state_callbacks, &m_capture_session);

    m_camera_ready = true;

    ACameraCaptureSession_setRepeatingRequest(m_capture_session, NULL, 1, &m_capture_request, NULL);
}

bool CV_Main::MatchCaptureSizeRequest(ImageFormat* resView) {
    DisplayDimension disp(ANativeWindow_getWidth(m_native_window),
                          ANativeWindow_getHeight(m_native_window));
    if (m_camera_orientation == 90 || m_camera_orientation == 270) {
        disp.Flip();
    }

    ACameraMetadata* metadata;
    ACameraManager_getCameraCharacteristics(m_camera_manager, m_selected_camera_ID, &metadata);
    ACameraMetadata_const_entry entry;
    ACameraMetadata_getConstEntry(metadata, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);
    // format of the data: format, width, height, input?, type int32
    bool foundIt = false;
    DisplayDimension foundRes(4000, 4000);

    for (int i = 0; i < entry.count; ++i) {
        int32_t input = entry.data.i32[i * 4 + 3];
        int32_t format = entry.data.i32[i * 4 + 0];
        if (input) continue;

        if (format == AIMAGE_FORMAT_YUV_420_888) {
            DisplayDimension res(entry.data.i32[i * 4 + 1],
                                 entry.data.i32[i * 4 + 2]);
            if (!disp.IsSameRatio(res)) continue;
            if (format == AIMAGE_FORMAT_YUV_420_888 && foundRes > res) {
                foundIt = true;
                foundRes = res;
            }
        }
    }

    if (foundIt) {
        resView->width = foundRes.org_width();
        resView->height = foundRes.org_height();
    } else {
        LOGE("Did not find any compatible camera resolution, taking 640x480");
        if (disp.IsPortrait()) {
            resView->width = 1080;
            resView->height = 1920;
        } else {
            resView->width = 1920;
            resView->height = 1080;
        }
    }
    resView->format = AIMAGE_FORMAT_YUV_420_888;
    m_found_dim = true;
    return foundIt;
}

void CV_Main::CameraLoop()
{
    while (1) {
        if (!m_camera_ready || !m_image_reader || !m_found_dim) continue;
        AImage *image = m_image_reader->GetNextImage();
        if (!image) {
            continue;
        }

        ANativeWindow_acquire(m_native_window);
        ANativeWindow_Buffer buf;
        if (ANativeWindow_lock(m_native_window, &buf, nullptr) < 0) {
            m_image_reader->DeleteImage(image);
            continue;
        }

        m_image_reader->DisplayImage(&buf, image);
        ANativeWindow_unlockAndPost(m_native_window);
        ANativeWindow_release(m_native_window);
    }

}

void CV_Main::RunCV()
{
    int w = 400;

    cv::Mat atom_image = cv::Mat::zeros( w, w, CV_8UC3 );
    cv::Point center = cv::Point( w/2, w/2);

    cv::circle( atom_image,
                center,
                w/32,
                cv::Scalar( 0, 0, 255 ),
                cv::FILLED,
                cv::LINE_8 );
}