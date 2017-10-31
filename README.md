# OpenCV For Android NDK

This is a showcase how to get OpenCV up and running with just the Android NDK

- This uses the NDK camera2 API to get the image buffers
- It then uses OpenCV to processes the image buffer
- Lastly it gets the NDK Native Window and displays the image for live previewing

## Prerequisites

- Android API 24 (Nougat) or greater
- Android Studio 2.3+
- [OpenCV-Android-SDK](https://sourceforge.net/projects/opencvlibrary/files/opencv-android/)

## How to setup

- Clone or download repo and open the OpenCV-NDK folder. (The 2nd one that is the child)
	- Click "Open project" in Android studio and find the folder saved in
- Change the OpenCV Android SDK library path in [Android.mk file](/OpenCV-NDK/app/src/main/cpp/Android.mk#L8) to your `my/path/to/OpenCV-android-sdk/sdk/native/jni/OpenCV.mk`
- Move all files in the [Asset folder](/OpenCV-NDK/app/src/main/assets) to your device
	- This is annoying af, but no good solution I could find easily fixed it since OpenCV requires a file string which the NDK won't give permission to.
	- Currently the folder the asset files is listed in the [CV_Main.h](/OpenCV-NDK/app/src/main/cpp/CV_Main.h) private values so make sure it matches

## How to run 32 bit (or x86)

To make compiling faster I only have it load and run ARM 64-bit (arm64-v8a). To add a different abi architecture:

- In the [build.gradle file](/OpenCV-NDK/app/build.gradle#L14) add `, armeabi-v7a` so gradle
- in the [Application.mk file](/OpenCV-NDK/app/src/main/cpp/Application.mk#L1) add `armeabi-v7a`
