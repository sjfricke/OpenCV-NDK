#include "CV_Main.h"

CV_Main::CV_Main()
    : m_camera_ready(false), m_image_reader(nullptr), m_native_camera(nullptr){
  temp = malloc(1080 * 1920 * 4);
  ASSERT(temp != nullptr, "Failed to allocate temp");

//  AAssetDir* assetDir = AAssetManager_openDir(m_aasset_manager, "");
//  const char* filename = (const char*)NULL;
//  while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
//    AAsset* asset = AAssetManager_open(m_aasset_manager, filename, AASSET_MODE_STREAMING);
//    char buf[BUFSIZ];
//    int nb_read = 0;
//    FILE* out = fopen(filename, "w");
//    while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
//      fwrite(buf, nb_read, 1, out);
//    fclose(out);
//    AAsset_close(asset);
//  }
//  AAssetDir_close(assetDir);

  if( !face_cascade.load( face_cascade_name ) ){ LOGE("--(!)Error loading face cascade\n"); };
  if( !eyes_cascade.load( eyes_cascade_name ) ){ LOGE("--(!)Error loading eyes cascade\n"); };
};

CV_Main::~CV_Main() {
  // clean up VM and callback handles
  JNIEnv* env;
  java_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
  env->DeleteGlobalRef(calling_activity_obj);
  calling_activity_obj = nullptr;

  // ACameraCaptureSession_stopRepeating(m_capture_session);

  if (m_native_camera != nullptr) {
    delete m_native_camera;
    m_native_camera = nullptr;
  }

   // make sure we don't leak native windows
  if (m_native_window != nullptr) {
    ANativeWindow_release(m_native_window);
    m_native_window = nullptr;
  }

  if (m_image_reader != nullptr) {
    delete (m_image_reader);
    m_image_reader = nullptr;
  }

  if (temp != nullptr) {
    free(temp);
  }
}

void CV_Main::OnCreate(JNIEnv* env, jobject caller_activity) {
  // Need to create an instance of the Java activity
  calling_activity_obj = env->NewGlobalRef(caller_activity);

  // Need to enter package and class to find Java class
  jclass handler_class = env->GetObjectClass(caller_activity);

  // Create function pointeACameraManager_getCameraCharacteristicsr to use for
  // on_loaded callbacks
  // on_callback = env->GetMethodID(handler_class, "JAVA_FUNCTION", "()V");
}

void CV_Main::OnPause() {}

void CV_Main::OnDestroy() {}

void CV_Main::SetNativeWindow(ANativeWindow* native_window) {
  // Save native window
  m_native_window = native_window;
}

void CV_Main::SetUpCamera() {

  m_native_camera = new Native_Camera(m_selected_camera_type);

  LOGI("TEST W: %d --- H: %d", ANativeWindow_getWidth(m_native_window),
       ANativeWindow_getHeight(m_native_window));

  //    m_view.height = 1920;
  //    m_view.width = 1080;
  //    m_view.format = AIMAGE_FORMAT_YUV_420_888;
  m_native_camera->MatchCaptureSizeRequest(&m_view,
                                           ANativeWindow_getWidth(m_native_window),
                                           ANativeWindow_getHeight(m_native_window));

  ASSERT(m_view.width && m_view.height,
         "Could not find supportable resolution");

  // Here we set the buffer to use RGBA_8888 as default might be; RGB_565
  ANativeWindow_setBuffersGeometry(m_native_window, m_view.height, m_view.width,
                                   WINDOW_FORMAT_RGBX_8888);

  m_image_reader = new Image_Reader(&m_view, AIMAGE_FORMAT_YUV_420_888);
  m_image_reader->SetPresentRotation(m_native_camera->GetOrientation());

  ANativeWindow* image_reader_window = m_image_reader->GetNativeWindow();

  m_camera_ready = m_native_camera->CreateCaptureSession(image_reader_window);
}

void CV_Main::CameraLoop() {
  bool test = false;

  cv::HOGDescriptor hog;
  hog.setSVMDetector(cv::HOGDescriptor::getDefaultPeopleDetector());

  while (1) {
    if (!m_camera_ready || !m_image_reader) continue;
    //         AImage *image = m_image_reader->GetNextImage();
    m_image = m_image_reader->GetLatestImage();
    if (!m_image) {
      continue;
    }

    ANativeWindow_acquire(m_native_window);
    ANativeWindow_Buffer buf;
    if (ANativeWindow_lock(m_native_window, &buf, nullptr) < 0) {
      m_image_reader->DeleteImage(m_image);
      continue;
    }

    if (false == test) { test = true;
      LOGI("========= H-W-S-F: %d, %d, %d, %d", buf.height, buf.width, buf.stride, buf.format);
    }

    m_image_reader->DisplayImage(&buf, m_image, temp);
   // memcpy(buf.bits, temp, buf.height * buf.stride * 4);
//
//    tempMat = cv::Mat(buf.height, buf.stride, CV_8UC3, temp);
//  cv::Mat outputMat = cv::Mat(buf->height, buf->width, CV_8UC4, buf->bits);
    tempMat = cv::Mat(buf.height, buf.stride, CV_8UC4, buf.bits);

//    start_t = clock();
   DetectAndDisplay(&tempMat);
   // DetectAndDraw(hog, tempMat);

//    end_t = clock();
//    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
//    LOGI("TIME BEFORE LOOP: %f\n", total_t  )


    ANativeWindow_unlockAndPost(m_native_window);
    ANativeWindow_release(m_native_window);
  }
}

void CV_Main::DetectAndDisplay( cv::Mat* frame ) {

  std::vector<cv::Rect> faces;
  cv::Mat frame_gray;

  cv::cvtColor( *frame, frame_gray, CV_RGBA2GRAY );

 // equalizeHist( frame_gray, frame_gray );

  start_t = clock();
 //-- Detect faces
  face_cascade.detectMultiScale( frame_gray, faces, 1.15, 2, 0|CV_HAAR_SCALE_IMAGE, cv::Size(70, 70) );
  end_t = clock();
  total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
  LOGI("TIME detectMultiScale: %f\n", total_t  );


  for( size_t i = 0; i < faces.size(); i++ )
  {
    cv::Point center( faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5 );
    ellipse( *frame, center, cv::Size( faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, cv::Scalar( 255, 0, 255 ), 4, 8, 0 );

    cv::Mat faceROI = frame_gray( faces[i] );
    std::vector<cv::Rect> eyes;

    //-- In each face, detect eyes
    eyes_cascade.detectMultiScale( faceROI, eyes, 1.2, 2, 0 |CV_HAAR_SCALE_IMAGE, cv::Size(45, 45) );

    for( size_t j = 0; j < eyes.size(); j++ )
    {
      cv::Point center( faces[i].x + eyes[j].x + eyes[j].width*0.5, faces[i].y + eyes[j].y + eyes[j].height*0.5 );
      int radius = cvRound( (eyes[j].width + eyes[j].height)*0.25 );
      circle( *frame, center, radius, cv::Scalar( 255, 0, 0 ), 4, 8, 0 );
    }
  }
}

void CV_Main::DetectAndDraw(const cv::HOGDescriptor &hog, cv::Mat &img) {
  std::vector<cv::Rect> found, found_filtered;
  // Run the detector with default parameters. to get a higher hit-rate
  // (and more false alarms, respectively), decrease the hitThreshold and
  // groupThreshold (set groupThreshold to 0 to turn off the grouping completely).

  cv::Mat frame_gray;
  cv::cvtColor( img, frame_gray, CV_RGBA2GRAY );

  start_t = clock();
  hog.detectMultiScale(frame_gray, found, 4, cv::Size(4,4), cv::Size(0,0), 2.00, 2);
  end_t = clock();
  total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
  LOGI("TIME detectMultiScale: %f\n", total_t  );

  for(size_t i = 0; i < found.size(); i++ )
  {
    cv::Rect r = found[i];

    size_t j;
    // Do not add small detections inside a bigger detection.
    for ( j = 0; j < found.size(); j++ )
      if ( j != i && (r & found[j]) == r )
        break;

    if ( j == found.size() )
      found_filtered.push_back(r);
  }

  for (size_t i = 0; i < found_filtered.size(); i++)
  {
    cv::Rect r = found_filtered[i];

    // The HOG detector returns slightly larger rectangles than the real objects,
    // so we slightly shrink the rectangles to get a nicer output.
    r.x += cvRound(r.width*0.1);
    r.width = cvRound(r.width*0.8);
    r.y += cvRound(r.height*0.07);
    r.height = cvRound(r.height*0.8);
    rectangle(img, r.tl(), r.br(), cv::Scalar(0,255,0), 3);
  }
}

void CV_Main::RunCV() {
  jumpingJackPost(40,23);
}

void CV_Main::FlipCamera() {

  if (m_native_camera == nullptr) {
    LOGE("Can't flip camera without camera instance");
    return; // need to setup camera
  } else if (m_native_camera->GetCameraCount() < 2) {
    LOGE("Only one camera is available"); // TODO - remove button if this is true
    return; // need a second camera to flip with
  }

  if (m_selected_camera_type == FRONT_CAMERA) {
    m_selected_camera_type = BACK_CAMERA;
  } else {
    m_selected_camera_type = FRONT_CAMERA;
  }

  // reset info
  m_camera_ready =  false;
  m_image_reader->DeleteImage(m_image);
  if (m_image_reader != nullptr) {
    delete (m_image_reader);
  }
  delete m_native_camera;

  SetUpCamera();
}