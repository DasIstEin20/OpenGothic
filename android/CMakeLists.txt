add_library(OpenGothic SHARED
    ../backend/android/android_main.cpp
    ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    ../backend/android/AndroidInputBackend.cpp
    ../backend/android/AndroidAudioBackend.cpp
    ../backend/android/AndroidFileSystemBackend.cpp
    ../backend/android/AndroidNativeGlue.cpp)

target_include_directories(OpenGothic PRIVATE ../backend
    ${ANDROID_NDK}/sources/android/native_app_glue)

target_link_libraries(OpenGothic log android)
