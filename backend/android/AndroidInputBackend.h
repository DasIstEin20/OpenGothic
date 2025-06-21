#pragma once
#include "../IInputBackend.h"
#include <android/input.h>

enum class InputEventType {
  KEY,         //!< Standard key press or release
  MOTION,      //!< Touch screen or joystick motion
  SPECIAL,     //!< Buttons like BACK, MENU or VOLUME
  SYSTEM,      //!< System level actions such as HOME
  VIRTUAL,     //!< On screen or emulated controls
  UNCLASSIFIED //!< Anything that doesn't fit other groups
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

    /// Enables or disables verbose input logging. Defaults to true in debug
    /// builds and false otherwise.
    static void setVerboseLogging(bool v);

    void setKeyCallback(KeyCallback cb) override;
    void setMotionCallback(MotionCallback cb) override;
    void pollEvents() override;

    /// Processes an Android input event. Called from native glue.
    int32_t onInputEvent(AInputEvent* event) override;

  private:
    KeyCallback    keyCb;
    MotionCallback motionCb;
    InputEventData lastEvent{};
    static bool    verboseLogging;
  };

