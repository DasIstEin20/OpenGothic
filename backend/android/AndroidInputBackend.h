#pragma once
#include "../IInputBackend.h"
#ifdef __ANDROID__
#include <android/input.h>
#endif
#include <cmath>

enum class InputEventType {
  KEY,         //!< Hardware key press or release from keyboard or gamepad
  MOTION,      //!< Pointer or joystick axis movement
  SPECIAL,     //!< Navigation keys such as BACK, MENU or VOLUME
  SYSTEM,      //!< Reserved system actions like HOME or POWER
  VIRTUAL,     //!< On‑screen or emulated controls
  UNCLASSIFIED //!< Any event not matching the above groups
  };

enum class InputEventSource {
  UNKNOWN,
  TOUCHSCREEN,
  KEYBOARD,
  JOYSTICK
  };

struct InputEventData {
  InputEventType   type = InputEventType::UNCLASSIFIED;
  InputEventSource source = InputEventSource::UNKNOWN;
  int32_t          deviceId = 0;
  int32_t          keyCode = 0;
  bool             pressed = false;
  float            x = 0.f;
  float            y = 0.f;
  uint64_t         eventTime = 0;
  };

inline bool sameEvent(const InputEventData& a,const InputEventData& b){
  return a.type==b.type && a.source==b.source && a.deviceId==b.deviceId &&
         a.keyCode==b.keyCode && a.pressed==b.pressed &&
         std::fabs(a.x-b.x)<0.0001f && std::fabs(a.y-b.y)<0.0001f;
  }

inline bool validCoords(float x,float y){
  return std::isfinite(x) && std::isfinite(y);
  }

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

#ifdef __ANDROID__
    /// Processes an Android input event. Called from native glue.
    int32_t onInputEvent(AInputEvent* event) override;
#endif

  private:
    KeyCallback    keyCb;
    MotionCallback motionCb;
    InputEventData lastEvent{};
    static bool    verboseLogging;
  };

