#pragma once
#include <cstdint>

#ifdef __ANDROID__
#include <android/input.h>
#endif

namespace InputMapper {

enum class EventType {
  KEY,
  MOTION
  };

enum class EventSource {
  UNKNOWN,
  TOUCH,
  MULTITOUCH,
  KEYBOARD,
  GAMEPAD
  };

struct InputEvent {
  EventType   type = EventType::KEY;
  EventSource source = EventSource::UNKNOWN;
  int32_t     code = 0;
  float       value = 0.f;
  int64_t     timestamp = 0;
  };

}

