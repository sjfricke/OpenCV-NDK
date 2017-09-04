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
#include <media/NdkImageReader.h>
#include <opencv2/core.hpp>

// A Data Structure to communicate resolution between camera and ImageReader
struct ImageFormat {
  int32_t width;
  int32_t height;
  int32_t format;  // ex) YUV_420
};

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
};

/**
 * A helper class to assist image size comparison, by comparing the absolute
 * size
 * regardless of the portrait or landscape mode.
 */
class DisplayDimension {
 public:
  DisplayDimension(int32_t w, int32_t h) : w_(w), h_(h), portrait_(false) {
    if (h > w) {
      // make it landscape
      w_ = h;
      h_ = w;
      portrait_ = true;
    }
  }
  DisplayDimension(const DisplayDimension& other) {
    w_ = other.w_;
    h_ = other.h_;
    portrait_ = other.portrait_;
  }

  DisplayDimension(void) {
    w_ = 0;
    h_ = 0;
    portrait_ = false;
  }
  DisplayDimension& operator=(const DisplayDimension& other) {
    w_ = other.w_;
    h_ = other.h_;
    portrait_ = other.portrait_;

    return (*this);
  }

  bool IsSameRatio(DisplayDimension& other) {
    return (w_ * other.h_ == h_ * other.w_);
  }
  bool operator>(DisplayDimension& other) {
    return (w_ >= other.w_ & h_ >= other.h_);
  }
  bool operator==(DisplayDimension& other) {
    return (w_ == other.w_ && h_ == other.h_ && portrait_ == other.portrait_);
  }
  DisplayDimension operator-(DisplayDimension& other) {
    DisplayDimension delta(w_ - other.w_, h_ - other.h_);
    return delta;
  }
  void Flip(void) { portrait_ = !portrait_; }
  bool IsPortrait(void) { return portrait_; }
  int32_t width(void) { return w_; }
  int32_t height(void) { return h_; }
  int32_t org_width(void) { return (portrait_ ? h_ : w_); }
  int32_t org_height(void) { return (portrait_ ? w_ : h_); }

 private:
  int32_t w_, h_;
  bool portrait_;
};

#endif  // OPENCV_NDK_IMAGE_READER_H
