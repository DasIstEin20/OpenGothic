#pragma once
#include <cstdint>
#include <functional>
#ifdef __ANDROID__
#include <android/input.h>
#endif

class IInputBackend {
  public:
    virtual ~IInputBackend() = default;

    using KeyCallback    = std::function<void(int32_t,bool)>;
    using MotionCallback = std::function<void(float,float)>;

    virtual void setKeyCallback(KeyCallback cb)    = 0;
    virtual void setMotionCallback(MotionCallback cb) = 0;
    virtual void pollEvents() = 0;

#ifdef __ANDROID__
    /// Processes a platform specific input event.
    virtual int32_t onInputEvent(AInputEvent* event) = 0;
#endif
  };

