# OpenCV For Android NDK

This is a showcase how to get OpenCV up and running with just the Android NDK

- This uses the NDK camera2 API to get the image buffers
- It then uses OpenCV to processes the image buffer
- Lastly it gets the NDK Native Window and displays the image for live previewing

## How to setup

- Change the OpenCV Android SDK library path in [Android.mk file](/OpenCV-NDK/app/src/main/cpp/Android.mk#L8) to your `my/path/to/OpenCV-android-sdk/sdk/native/jni/OpenCV.mk`

## How to run 32 bit (or x86)

To make compiling faster I only have it load and run ARM 64-bit (arm64-v8a). To add a different abi architecture:

- In the [build.gradle file](/OpenCV-NDK/app/build.gradle#L14) add `, armeabi-v7a` so gradle
- in the [Application.mk file](/OpenCV-NDK/app/src/main/cpp/Application.mk#L1) add `armeabi-v7a`
