#include <android/native_activity.h>
#include <android/log.h>
#include <android/native_window_jni.h>

#include "AndroidInputBackend.h"
#include "AndroidAudioBackend.h"
#include "AndroidFileSystemBackend.h"
#include "AndroidNativeGlue.h"

extern "C" void GameMain(IInputBackend&,IAudioBackend&,IFileSystemBackend&,INativeGlue&);

extern "C" void ANativeActivity_onCreate(ANativeActivity* activity,void*,size_t){
  __android_log_print(ANDROID_LOG_INFO,"OpenGothic","activity created");
  AndroidInputBackend  input;
  AndroidAudioBackend  audio;
  AndroidFileSystemBackend fs(activity->assetManager);
  AndroidNativeGlue    glue(activity);
  GameMain(input,audio,fs,glue);
  }

