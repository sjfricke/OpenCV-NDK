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
#include "Image_Reader.h"
#include <string>
#include <opencv2/imgproc.hpp>
#include "Util.h"

/**
 * MAX_BUF_COUNT:
 *   Max buffers in this ImageReader.
 */
#define MAX_BUF_COUNT 2

/**
 * ImageReader listener: called by AImageReader for every frame captured
 * We pass the event to ImageReader class, so it could do some housekeeping
 * about
 * the loaded queue. For example, we could keep a counter to track how many
 * buffers are full and idle in the queue. If camera almost has no buffer to
 * capture
 * we could release ( skip ) some frames by AImageReader_getNextImage() and
 * AImageReader_delete().
 */
void OnImageCallback(void *ctx, AImageReader *reader) {
  reinterpret_cast<Image_Reader *>(ctx)->ImageCallback(reader);
}

/**
 * Constructor
 */
Image_Reader::Image_Reader(ImageFormat *res, enum AIMAGE_FORMATS format)
    : reader_(nullptr),
      presentRotation_(0),
      imageHeight_(res->height),
      imageWidth_(res->width) {
  media_status_t status = AImageReader_new(res->width, res->height, format,
                                           MAX_BUF_COUNT, &reader_);
  ASSERT(reader_ && status == AMEDIA_OK, "Failed to create AImageReader");

  AImageReader_ImageListener listener{
      .context = this, .onImageAvailable = OnImageCallback,
  };
  AImageReader_setImageListener(reader_, &listener);

  // assuming 4 bit per pixel max
  LOGI("Image Buffer Size: %d", res->width * res->height * 4);
  imageBuffer_ = (uint8_t*)malloc(res->width * res->height * 4);
  ASSERT(imageBuffer_ != nullptr, "Failed to allocate imageBuffer_");


//  temp = malloc(1080 * 1920 * 4);
//  ASSERT(temp != nullptr, "Failed to allocate temp");
}

Image_Reader::~Image_Reader() {
  ASSERT(reader_, "NULL Pointer to %s", __FUNCTION__);
  AImageReader_delete(reader_);

  if (imageBuffer_ != nullptr) {
    free(imageBuffer_);
  }

//  if (temp != nullptr) {
//    free(temp);
//  }
}

void Image_Reader::ImageCallback(AImageReader *reader) {
  int32_t format;
  media_status_t status = AImageReader_getFormat(reader, &format);
  ASSERT(status == AMEDIA_OK, "Failed to get the media format");
  if (format == AIMAGE_FORMAT_JPEG) {
    // Create a thread and write out the jpeg files
    AImage *image = nullptr;
    media_status_t status = AImageReader_acquireNextImage(reader, &image);
    ASSERT(status == AMEDIA_OK && image, "Image is not available");

    int planeCount;
    status = AImage_getNumberOfPlanes(image, &planeCount);
    ASSERT(status == AMEDIA_OK && planeCount == 1,
           "Error: getNumberOfPlanes() planceCount = %d", planeCount);
    uint8_t *data = nullptr;
    int len = 0;
    AImage_getPlaneData(image, 0, &data, &len);

    AImage_delete(image);
  }
}

ANativeWindow *Image_Reader::GetNativeWindow(void) {
  if (!reader_) return nullptr;
  ANativeWindow *nativeWindow;
  media_status_t status = AImageReader_getWindow(reader_, &nativeWindow);
  ASSERT(status == AMEDIA_OK, "Could not get ANativeWindow");

  return nativeWindow;
}

/**
 * GetNextImage()
 *   Retrieve the next image in Image_Reader's bufferQueue, NOT the last image
 * so
 * no image is
 *   skipped
 */
AImage *Image_Reader::GetNextImage(void) {
  AImage *image;
  media_status_t status = AImageReader_acquireNextImage(reader_, &image);
  if (status != AMEDIA_OK) {
    return nullptr;
  }
  return image;
}

/**
 *   Retrieve the Last image in Image_Reader's bufferQueue, images may be
 * skipped
 */
AImage *Image_Reader::GetLatestImage(void) {
  AImage *image;
  media_status_t status = AImageReader_acquireLatestImage(reader_, &image);
  if (status != AMEDIA_OK) {
    return nullptr;
  }
  return image;
}

/**
 *   Shows max image buffer
 */
int32_t Image_Reader::GetMaxImage(void) {
  int32_t image_count;
  media_status_t status = AImageReader_getMaxImages(reader_, &image_count);
  if (status != AMEDIA_OK) {
    return -1;
  }
  return image_count;
}

/**
 * Delete Image
 * @param image {@link AImage} instance to be deleted
 */
void Image_Reader::DeleteImage(AImage *image) {
  if (image) AImage_delete(image);
}

/**
 * Helper function for YUV_420 to RGB conversion. Courtesy of Tensorflow
 * ImageClassifier Sample:
 * https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/android/jni/yuv2rgb.cc
 * The difference is that here we have to swap UV plane when calling it.
 */
#ifndef MAX
#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })
#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })
#endif

// This value is 2 ^ 18 - 1, and is used to clamp the RGB values before their
// ranges
// are normalized to eight bits.
static const int kMaxChannelValue = 262143;

static inline uint32_t YUV2RGB(int nY, int nU, int nV) {
  nY -= 16;
  nU -= 128;
  nV -= 128;
  if (nY < 0) nY = 0;

  // This is the floating point equivalent. We do the conversion in integer
  // because some Android devices do not have floating point in hardware.
  // nR = (int)(1.164 * nY + 1.596 * nV);
  // nG = (int)(1.164 * nY - 0.813 * nV - 0.391 * nU);
  // nB = (int)(1.164 * nY + 2.018 * nU);

  int nR = (int)(1192 * nY + 1634 * nV);
  int nG = (int)(1192 * nY - 833 * nV - 400 * nU);
  int nB = (int)(1192 * nY + 2066 * nU);

  nR = MIN(kMaxChannelValue, MAX(0, nR));
  nG = MIN(kMaxChannelValue, MAX(0, nG));
  nB = MIN(kMaxChannelValue, MAX(0, nB));

  nR = (nR >> 10) & 0xff;
  nG = (nG >> 10) & 0xff;
  nB = (nB >> 10) & 0xff;

  return 0xff000000 | (nR << 16) | (nG << 8) | nB;
}

/**
 * Convert yuv image inside AImage into ANativeWindow_Buffer
 * ANativeWindow_Buffer format is guaranteed to be
 *      WINDOW_FORMAT_RGBX_8888
 *      WINDOW_FORMAT_RGBA_8888
 * @param buf a {@link ANativeWindow_Buffer } instance, destination of
 *            image conversion
 * @param image a {@link AImage} instance, source of image conversion.
 *            it will be deleted via {@link AImage_delete}
 */
bool Image_Reader::DisplayImage(ANativeWindow_Buffer *buf, AImage *image) {
  ASSERT(buf->format == WINDOW_FORMAT_RGBX_8888 ||
      buf->format == WINDOW_FORMAT_RGBA_8888,
         "Not supported buffer format");

  int32_t srcFormat = -1;
  AImage_getFormat(image, &srcFormat);
  ASSERT(AIMAGE_FORMAT_YUV_420_888 == srcFormat, "Failed to get format");
  int32_t srcPlanes = 0;
  AImage_getNumberOfPlanes(image, &srcPlanes);
  ASSERT(srcPlanes == 3, "Is not 3 planes");

  switch (presentRotation_) {
    case 0:
      PresentImage(buf, image);
      break;
    case 90:
        PresentImage90(buf, image);
      break;
    case 180:
      PresentImage180(buf, image);
      break;
    case 270:
      PresentImage270(buf, image);
      break;
    default:
      ASSERT(0, "NOT recognized display rotation: %d", presentRotation_);
  }

  AImage_delete(image);

  return true;
}

/*
 * PresentImage()
 *   Converting yuv to RGB
 *   No rotation: (x,y) --> (x, y)
 *   Refer to:
 * https://mathbits.com/MathBits/TISection/Geometry/Transformations2.htm
 */
void Image_Reader::PresentImage(ANativeWindow_Buffer *buf, AImage *image) {
  AImageCropRect srcRect;
  AImage_getCropRect(image, &srcRect);

  int32_t yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  AImage_getPlaneRowStride(image, 0, &yStride);
  AImage_getPlaneRowStride(image, 1, &uvStride);
  AImage_getPlaneData(image, 0, &yPixel, &yLen);
  AImage_getPlaneData(image, 1, &vPixel, &vLen);
  AImage_getPlaneData(image, 2, &uPixel, &uLen);
  int32_t uvPixelStride;
  AImage_getPlanePixelStride(image, 1, &uvPixelStride);

  int32_t height = MIN(buf->height, (srcRect.bottom - srcRect.top));
  int32_t width = MIN(buf->width, (srcRect.right - srcRect.left));

  uint32_t *out = static_cast<uint32_t *>(buf->bits);

  for (int32_t y = 0; y < height; y++) {
    const uint8_t *pY = yPixel + yStride * (y + srcRect.top) + srcRect.left;

    int32_t uv_row_start = uvStride * ((y + srcRect.top) >> 1);
    const uint8_t *pU = uPixel + uv_row_start + (srcRect.left >> 1);
    const uint8_t *pV = vPixel + uv_row_start + (srcRect.left >> 1);

    for (int32_t x = 0; x < width; x++) {
      const int32_t uv_offset = (x >> 1) * uvPixelStride;
      out[x] = YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset]);
    }
    out += buf->stride;
  }
}

/*
 * PresentImage90()
 *   Converting YUV to RGB
 *   Rotation image anti-clockwise 90 degree -- (x, y) --> (-y, x)
 */
void Image_Reader::PresentImage90(ANativeWindow_Buffer *buf, AImage *image) {
  AImageCropRect srcRect;
  AImage_getCropRect(image, &srcRect);

  int32_t yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  AImage_getPlaneRowStride(image, 0, &yStride);
  AImage_getPlaneRowStride(image, 1, &uvStride);
  yPixel = imageBuffer_;
  AImage_getPlaneData(image, 0, &yPixel, &yLen);
  vPixel = imageBuffer_ + yLen;
  AImage_getPlaneData(image, 1, &vPixel, &vLen);
  uPixel = imageBuffer_ + yLen + vLen;
  AImage_getPlaneData(image, 2, &uPixel, &uLen);
  int32_t uvPixelStride;
  AImage_getPlanePixelStride(image, 1, &uvPixelStride);

//  memcpy(temp, imageBuffer_, yLen + uLen + vLen);
//  cv::Mat inputMat = cv::Mat(imageHeight_, imageWidth_, CV_8UC1, imageBuffer_);
//  cv::Mat outputMat = cv::Mat(buf->height, buf->stride, CV_8UC4, buf->bits);
//  cv::cvtColor(inputMat, outputMat, CV_YUV2RGBA_NV21);

  int32_t height = MIN(buf->width, (srcRect.bottom - srcRect.top));
  int32_t width = MIN(buf->height, (srcRect.right - srcRect.left));

  uint32_t *out = static_cast<uint32_t *>(buf->bits);
//  uint32_t *out = static_cast<uint32_t *>(temp);
  out += height - 1;
  for (int32_t y = 0; y < height; y++) {
    const uint8_t *pY = yPixel + yStride * (y + srcRect.top) + srcRect.left;

    int32_t uv_row_start = uvStride * ((y + srcRect.top) >> 1);
    const uint8_t *pU = uPixel + uv_row_start + (srcRect.left >> 1);
    const uint8_t *pV = vPixel + uv_row_start + (srcRect.left >> 1);

    for (int32_t x = 0; x < width; x++) {
      const int32_t uv_offset = (x >> 1) * uvPixelStride;
      // [x, y]--> [-y, x]
      out[x * buf->stride] = YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset]);
    }
    out -= 1;  // move to the next column
  }
}

/*
 * PresentImage180()
 *   Converting yuv to RGB
 *   Rotate image 180 degree: (x, y) --> (-x, -y)
 */
void Image_Reader::PresentImage180(ANativeWindow_Buffer *buf, AImage *image) {
  AImageCropRect srcRect;
  AImage_getCropRect(image, &srcRect);

  int32_t yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  AImage_getPlaneRowStride(image, 0, &yStride);
  AImage_getPlaneRowStride(image, 1, &uvStride);
  AImage_getPlaneData(image, 0, &yPixel, &yLen);
  AImage_getPlaneData(image, 1, &vPixel, &vLen);
  AImage_getPlaneData(image, 2, &uPixel, &uLen);
  int32_t uvPixelStride;
  AImage_getPlanePixelStride(image, 1, &uvPixelStride);

  int32_t height = MIN(buf->height, (srcRect.bottom - srcRect.top));
  int32_t width = MIN(buf->width, (srcRect.right - srcRect.left));

  uint32_t *out = static_cast<uint32_t *>(buf->bits);
  out += (height - 1) * buf->stride;
  for (int32_t y = 0; y < height; y++) {
    const uint8_t *pY = yPixel + yStride * (y + srcRect.top) + srcRect.left;

    int32_t uv_row_start = uvStride * ((y + srcRect.top) >> 1);
    const uint8_t *pU = uPixel + uv_row_start + (srcRect.left >> 1);
    const uint8_t *pV = vPixel + uv_row_start + (srcRect.left >> 1);

    for (int32_t x = 0; x < width; x++) {
      const int32_t uv_offset = (x >> 1) * uvPixelStride;
      // mirror image since we are using front camera
      out[width - 1 - x] = YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset]);
      // out[x] = YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset]);
    }
    out -= buf->stride;
  }
}

/*
 * PresentImage270()
 *   Converting image from YUV to RGB
 *   Rotate Image counter-clockwise 270 degree: (x, y) --> (y, x)
 */
void Image_Reader::PresentImage270(ANativeWindow_Buffer *buf, AImage *image) {
  AImageCropRect srcRect;
  AImage_getCropRect(image, &srcRect);

  int32_t yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  AImage_getPlaneRowStride(image, 0, &yStride);
  AImage_getPlaneRowStride(image, 1, &uvStride);
  yPixel = imageBuffer_;
  AImage_getPlaneData(image, 0, &yPixel, &yLen);
  vPixel = imageBuffer_ + yLen;
  AImage_getPlaneData(image, 1, &vPixel, &vLen);
  uPixel = imageBuffer_ + yLen + vLen;
  AImage_getPlaneData(image, 2, &uPixel, &uLen);
  int32_t uvPixelStride;
  AImage_getPlanePixelStride(image, 1, &uvPixelStride);

  int32_t height = MIN(buf->width, (srcRect.bottom - srcRect.top));
  int32_t width = MIN(buf->height, (srcRect.right - srcRect.left));

  uint32_t *out = static_cast<uint32_t *>(buf->bits);
  for (int32_t y = 0; y < height; y++) {
    const uint8_t *pY = yPixel + yStride * (y + srcRect.top) + srcRect.left;

    int32_t uv_row_start = uvStride * ((y + srcRect.top) >> 1);
    const uint8_t *pU = uPixel + uv_row_start + (srcRect.left >> 1);
    const uint8_t *pV = vPixel + uv_row_start + (srcRect.left >> 1);

    for (int32_t x = 0; x < width; x++) {
      const int32_t uv_offset = (x >> 1) * uvPixelStride;
      out[(width - 1 - x) * buf->stride] =
          YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset]);
    }
    out += 1;  // move to the next column
  }
}

void Image_Reader::SetPresentRotation(int32_t angle) {
  presentRotation_ = angle;
}