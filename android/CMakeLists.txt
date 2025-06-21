add_library(OpenGothic SHARED
    ../backend/android/android_main.cpp
    ../backend/android/AndroidInputBackend.cpp
    ../backend/android/AndroidAudioBackend.cpp
    ../backend/android/AndroidFileSystemBackend.cpp
    ../backend/android/AndroidNativeGlue.cpp)

target_include_directories(OpenGothic PRIVATE ../backend)

target_link_libraries(OpenGothic log android)
