#pragma once
#include "../IInputBackend.h"
#include <android/input.h>

enum class InputEventType {
  KEY,
  MOTION,
  SPECIAL,
  UNCLASSIFIED
  };

struct InputEventData {
  InputEventType type = InputEventType::UNCLASSIFIED;
  int32_t        source = 0;
  int32_t        deviceId = 0;
  int32_t        keyCode = 0;
  bool           pressed = false;
  float          x = 0.f;
  float          y = 0.f;
  };

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

