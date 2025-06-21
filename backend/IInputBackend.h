#pragma once
#include <cstdint>
#include <functional>

class IInputBackend {
  public:
    virtual ~IInputBackend() = default;

    using KeyCallback    = std::function<void(int32_t,bool)>;
    using MotionCallback = std::function<void(float,float)>;

    virtual void setKeyCallback(KeyCallback cb)    = 0;
    virtual void setMotionCallback(MotionCallback cb) = 0;
    virtual void pollEvents() = 0;
  };

