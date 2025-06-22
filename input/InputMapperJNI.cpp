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


#include <fstream>
#include <sstream>

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_profilesapp_InputMapper_nativeLoadProfile(JNIEnv* env,jobject,jlong ptr,
                                                           jstring path){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  const char* p = env->GetStringUTFChars(path,nullptr);
  std::ifstream f(p);
  if(!f.is_open()){
    env->ReleaseStringUTFChars(path,p);
    return JNI_FALSE;
  }
  std::stringstream ss;
  ss << f.rdbuf();
  bool ok = mapper->loadProfile(p, ss.str());
  env->ReleaseStringUTFChars(path,p);
  return static_cast<jboolean>(ok);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_profilesapp_InputMapper_nativeActivateProfile(JNIEnv* env,jobject,jlong ptr,
                                                              jstring name){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  const char* n = env->GetStringUTFChars(name,nullptr);
  mapper->setActiveProfile(n);
  env->ReleaseStringUTFChars(name,n);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_profilesapp_InputMapper_nativeMapEvent(JNIEnv* env,jobject,jlong ptr,
                                                        jint source,jint code){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  InputMapper::InputEvent ev;
  ev.type   = InputMapper::EventType::KEY;
  ev.source = static_cast<InputMapper::EventSource>(source);
  ev.code   = code;
  auto res = mapper->mapEvent(ev);
  if(!res)
    return nullptr;
  return env->NewStringUTF(res->c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_profilesapp_InputMapper_nativeMapMotionEvent(JNIEnv* env,jobject,jlong ptr,
                                                             jint source,jint axis,jfloat value){
  auto* mapper = reinterpret_cast<InputMapper::InputMapper*>(ptr);
  InputMapper::InputEvent ev;
  ev.type   = InputMapper::EventType::MOTION;
  ev.source = static_cast<InputMapper::EventSource>(source);
  ev.code   = axis;
  ev.value  = value;
  auto res = mapper->mapEvent(ev);
  if(!res)
    return nullptr;
  return env->NewStringUTF(res->c_str());
}
