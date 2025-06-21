#pragma once
#include <cstdint>

class IAudioBackend {
  public:
    virtual ~IAudioBackend() = default;

    virtual bool init() = 0;
    virtual void shutdown() = 0;

    virtual void playSample(const void* data,size_t size,int rate,int chan)=0;
  };

