#pragma once
#include "../IInputBackend.h"
#include <android/input.h>

class AndroidInputBackend : public IInputBackend {
  public:
    AndroidInputBackend();
    ~AndroidInputBackend() override;

    void setKeyCallback(KeyCallback cb) override;
    void setMotionCallback(MotionCallback cb) override;
    void pollEvents() override;

    /// Processes an Android input event. Called from native glue.
    int32_t onInputEvent(AInputEvent* event) override;

  private:
    KeyCallback    keyCb;
    MotionCallback motionCb;
  };

