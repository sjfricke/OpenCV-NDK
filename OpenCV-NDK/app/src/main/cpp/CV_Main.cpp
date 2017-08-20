#include "CV_Main.h"

CV_Main::CV_Main()
{
};

CV_Main::~CV_Main()
{
    // clean up VM and callback handles
    JNIEnv* env;
    java_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    env->DeleteGlobalRef(calling_activity_obj);
    calling_activity_obj = nullptr;

    // make sure we don't leak native windows
    if (m_native_window != NULL) {
        ANativeWindow_release(m_native_window);
        m_native_window = NULL;
    }

    if (m_frame_buffer != NULL) {
        free(m_frame_buffer);
        m_frame_buffer = NULL;
    }
}

void CV_Main::OnCreate(JNIEnv* env, jobject caller_activity)
{
    // Need to create an instance of the Java activity
    calling_activity_obj = env->NewGlobalRef(caller_activity);

    // Need to enter package and class to find Java class
    jclass handler_class = env->GetObjectClass(caller_activity);

    // Create function pointer to use for on_loaded callbacks
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

    // Here we set the buffer to use RGBA_8888 as default might be; RGB_565
    if (ANativeWindow_lock(m_native_window, &m_native_buffer, NULL) == 0) {
        ANativeWindow_setBuffersGeometry(m_native_window, m_native_buffer.width, m_native_buffer.height, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);

        // while we have the buffer size this is a good time to allocate memory to copy to it later
        m_frame_buffer = (int32_t *)malloc(sizeof(int32_t) * 4 * m_native_buffer.width * m_native_buffer.height);
        if (m_frame_buffer == NULL) { LOGE("Failed to allocate frame_buffer"); exit(1); }

        // save buffer info while initializing as well
        m_frame_height = m_native_buffer.height;
        m_frame_width = m_native_buffer.width;
        m_frame_stride = m_native_buffer.stride;

        ANativeWindow_unlockAndPost(m_native_window);
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