#pragma once
#include "../IInputBackend.h"
#ifdef __ANDROID__
#include <android/input.h>
#endif
#include <cmath>
#ifdef ENABLE_SEQUENCE_TRACKING
#include <atomic>
#endif

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

#ifdef ENABLE_GESTURE_TRACKING
enum class InputTouchClass {
  SINGLE,
  MULTI,
  GESTURE
  };

enum class GestureType {
  NONE,
  SWIPE,
  PINCH_IN,
  PINCH_OUT,
  ROTATE
  };
#endif

struct InputEventData {
  InputEventType   type = InputEventType::UNCLASSIFIED;
  InputEventSource source = InputEventSource::UNKNOWN;
  int32_t          deviceId = 0;
  int32_t          keyCode = 0;
  bool             pressed = false;
  bool             repeatable = false;
  float            x = 0.f;
  float            y = 0.f;
  uint64_t         eventTime = 0;
#ifdef ENABLE_GESTURE_TRACKING
  uint32_t         gestureId      = 0;
  uint32_t         fingerCount    = 0;
  uint64_t         gestureDuration= 0;
  InputTouchClass  touchClass     = InputTouchClass::SINGLE;
  GestureType      gesture        = GestureType::NONE;
#endif
#ifdef ENABLE_SEQUENCE_TRACKING
  uint64_t         sequenceId = 0;
  bool             longPress  = false;
#endif
  };

inline bool sameEvent(const InputEventData& a,const InputEventData& b){
  return a.type==b.type && a.source==b.source && a.deviceId==b.deviceId &&
         a.keyCode==b.keyCode && a.pressed==b.pressed &&
         std::fabs(a.x-b.x)<0.0001f && std::fabs(a.y-b.y)<0.0001f;
  }

inline bool validCoords(float x,float y){
  return std::isfinite(x) && std::isfinite(y);
  }

/// Determines if a key event can auto-repeat when held down
inline bool isRepeatableEvent(const InputEventData& d){
  if(d.source!=InputEventSource::KEYBOARD || d.type!=InputEventType::KEY)
    return false;
  switch(d.keyCode){
    // Letters
    case 0x1e00: case 0x3000: case 0x2e00: case 0x2000: case 0x1200:
    case 0x2100: case 0x2200: case 0x2300: case 0x1700: case 0x2400:
    case 0x2500: case 0x2600: case 0x3200: case 0x3100: case 0x1800:
    case 0x1900: case 0x1000: case 0x1300: case 0x1f00: case 0x1400:
    case 0x1600: case 0x2f00: case 0x1100: case 0x2d00: case 0x1500:
    case 0x2c00:
    // Numbers
    case 0x7800: case 0x7900: case 0x7a00: case 0x7b00: case 0x7c00:
    case 0x7d00: case 0x7e00: case 0x7f00: case 0x8000: case 0x8100:
    // Arrows and editing
    case 0xc800: case 0xd000: case 0xcb00: case 0xcd00:
    case 0x0e00: case 0xd300: case 0x3900: case 0x1c00: case 0x0f00:
      return true;
    default:
      return false;
    }
  }

#ifdef ENABLE_SEQUENCE_TRACKING
/// Generates monotonically increasing identifiers for input sequences
inline uint64_t generateSequenceId(){
  static std::atomic_uint64_t seq{1};
  return seq++;
  }

/// Helper to detect long press events based on press duration
inline bool isLongPress(uint64_t start,uint64_t end,uint64_t thresholdMs=500){
  return (end>start) && ((end-start)>=thresholdMs);
  }
#endif

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
#ifdef ENABLE_SEQUENCE_TRACKING
    uint64_t       currentSeqId = 0;
    uint64_t       seqStartTime = 0;
    uint64_t       lastSeqTime  = 0;
    int32_t        seqKey       = 0;
    bool           seqActive    = false;
#endif
    static bool    verboseLogging;
  };

