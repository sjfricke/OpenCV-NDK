/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OPENCV_NDK_IMAGE_READER_H
#define OPENCV_NDK_IMAGE_READER_H
#include "Util.h"
#include <media/NdkImageReader.h>
#include <opencv2/core.hpp>

class Image_Reader {
 public:
  explicit Image_Reader(ImageFormat* res, enum AIMAGE_FORMATS format);

  ~Image_Reader();

  /**
   * Report cached ANativeWindow, which was used to create camera's capture
   * session output.
   */
  ANativeWindow* GetNativeWindow(void);

  /**
   * Retrieve Image on the top of Reader's queue
   */
  AImage* GetNextImage(void);

  /**
  * Retrieve Image on the bottom of Reader's queue
  */
  AImage* GetLatestImage(void);

  int32_t GetMaxImage(void);

  /**
   * Delete Image
   * @param image {@link AImage} instance to be deleted
   */
  void DeleteImage(AImage* image);

  /**
   * AImageReader callback handler. Called by AImageReader when a frame is
   * captured
   * (Internal function, not to be called by clients)
   */
  void ImageCallback(AImageReader* reader);

  /**
   * DisplayImage()
   *   Present camera image to the given display buffer. Avaliable image is
   * converted
   *   to display buffer format. Supported display format:
   *      WINDOW_FORMAT_RGBX_8888
   *      WINDOW_FORMAT_RGBA_8888
   *   @param buf {@link ANativeWindow_Buffer} for image to display to.
   *   @param image a {@link AImage} instance, source of image conversion.
   *            it will be deleted via {@link AImage_delete}
   *   @return true on success, false on failure
   */
  bool DisplayImage(ANativeWindow_Buffer* buf, AImage* image);

  /**
   * Configure the rotation angle necessary to apply to
   * Camera image when presenting: all rotations should be accumulated:
   *    CameraSensorOrientation + Android Device Native Orientation +
   *    Human Rotation (rotated degree related to Phone native orientation
   */
  void SetPresentRotation(int32_t angle);

 private:
  int32_t presentRotation_;
  AImageReader* reader_;

  void PresentImage(ANativeWindow_Buffer* buf, AImage* image);
  void PresentImage90(ANativeWindow_Buffer* buf, AImage* image);
  void PresentImage180(ANativeWindow_Buffer* buf, AImage* image);
  void PresentImage270(ANativeWindow_Buffer* buf, AImage* image);

  int32_t imageHeight_;
  int32_t imageWidth_;

  uint8_t* imageBuffer_;

  int32_t yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  int32_t uvPixelStride;
};

#endif  // OPENCV_NDK_IMAGE_READER_H
