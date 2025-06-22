#include "InputMapper.h"
#include <jni.h>
#include <memory>


extern "C" JNIEXPORT jlong JNICALL
Java_com_example_input_InputMapper_nativeCreate(JNIEnv*, jclass){
  return reinterpret_cast<jlong>(new InputMapper::InputMapper());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_input_InputMapper_nativeDestroy(JNIEnv*, jclass,jlong ptr){
  delete reinterpret_cast<InputMapper::InputMapper*>(ptr);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_input_InputMapper_nativeLoadProfile(JNIEnv* env,jclass,jlong ptr,
                                                     jstring name,jstring json){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  const char* n = env->GetStringUTFChars(name,nullptr);
  const char* j = env->GetStringUTFChars(json,nullptr);
  bool ok = mapper->loadProfile(n,j);
  env->ReleaseStringUTFChars(name,n);
  env->ReleaseStringUTFChars(json,j);
  return static_cast<jboolean>(ok);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_input_InputMapper_nativeSetActiveProfile(JNIEnv* env,jclass,jlong ptr,
                                                          jstring name){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  const char* n = env->GetStringUTFChars(name,nullptr);
  mapper->setActiveProfile(n);
  env->ReleaseStringUTFChars(name,n);
}

