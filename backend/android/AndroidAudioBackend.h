#pragma once
#include "../IAudioBackend.h"

class AndroidAudioBackend : public IAudioBackend {
  public:
    AndroidAudioBackend();
    ~AndroidAudioBackend() override;

    bool init() override;
    void shutdown() override;
    void playSample(const void* data,size_t size,int rate,int chan) override;
  };

