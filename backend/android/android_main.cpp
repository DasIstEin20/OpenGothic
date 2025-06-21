#include <android_native_app_glue.h>
#include <android/log.h>

#include "AndroidInputBackend.h"
#include "AndroidAudioBackend.h"
#include "AndroidFileSystemBackend.h"
#include "AndroidNativeGlue.h"

extern "C" void GameMain(IInputBackend&,IAudioBackend&,IFileSystemBackend&,INativeGlue&);

extern "C" void android_main(struct android_app* app) {
    app_dummy();
    __android_log_print(ANDROID_LOG_INFO,"OpenGothic","android_main started");

    AndroidInputBackend  input;
    AndroidAudioBackend  audio;
    AndroidFileSystemBackend fs(app->activity->assetManager);
    AndroidNativeGlue    glue(app->activity);

    app->onInputEvent = ::onInputEvent;

    GameMain(input,audio,fs,glue);

    int events;
    android_poll_source* source;
    while(true) {
        while(ALooper_pollAll(0,nullptr,&events,(void**)&source) >= 0) {
            if(source)
                source->process(app, source);
            if(app->destroyRequested != 0)
                return;
        }
        input.pollEvents();
    }
}
