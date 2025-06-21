#include "AndroidInputBackend.h"
#include <android/looper.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"InputBackend",__VA_ARGS__)
#include <cstring>

namespace {
  AndroidInputBackend* g_inst = nullptr;

  const char* typeToStr(InputEventType t) {
    switch(t) {
      case InputEventType::KEY:        return "KEY";
      case InputEventType::MOTION:     return "MOTION";
      case InputEventType::SPECIAL:    return "SPECIAL";
      case InputEventType::SYSTEM:     return "SYSTEM";
      case InputEventType::VIRTUAL:    return "VIRTUAL";
      default:                         return "UNCLASSIFIED";
      }
    }

  const char* sourceToStr(InputEventSource s) {
    switch(s) {
      case InputEventSource::TOUCHSCREEN: return "TOUCHSCREEN";
      case InputEventSource::KEYBOARD:    return "KEYBOARD";
      case InputEventSource::JOYSTICK:    return "JOYSTICK";
      default:                            return "UNKNOWN";
      }
    }

  InputEventSource mapSource(int32_t s){
    if(s & AINPUT_SOURCE_TOUCHSCREEN)
      return InputEventSource::TOUCHSCREEN;
    if(s & (AINPUT_SOURCE_JOYSTICK|AINPUT_SOURCE_GAMEPAD))
      return InputEventSource::JOYSTICK;
    if(s & AINPUT_SOURCE_KEYBOARD)
      return InputEventSource::KEYBOARD;
    return InputEventSource::UNKNOWN;
    }

  // Log an input event using a compact JSON style string
  void logEvent(const char* msg, const InputEventData& d) {
    if(!AndroidInputBackend::verboseLogging)
      return;
    if(msg)
      LOGI("{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu, \"msg\": \"%s\" }",
           typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
           d.pressed?"true":"false", d.x, d.y,
           static_cast<unsigned long long>(d.eventTime), msg);
    else
      LOGI("{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu }",
           typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
           d.pressed?"true":"false", d.x, d.y,
           static_cast<unsigned long long>(d.eventTime));
    }

  int32_t mapKey(int32_t k){
    switch(k){
      case AKEYCODE_A: return 0x1e00;
      case AKEYCODE_B: return 0x3000;
      case AKEYCODE_C: return 0x2e00;
      case AKEYCODE_D: return 0x2000;
      case AKEYCODE_E: return 0x1200;
      case AKEYCODE_F: return 0x2100;
      case AKEYCODE_G: return 0x2200;
      case AKEYCODE_H: return 0x2300;
      case AKEYCODE_I: return 0x1700;
      case AKEYCODE_J: return 0x2400;
      case AKEYCODE_K: return 0x2500;
      case AKEYCODE_L: return 0x2600;
      case AKEYCODE_M: return 0x3200;
      case AKEYCODE_N: return 0x3100;
      case AKEYCODE_O: return 0x1800;
      case AKEYCODE_P: return 0x1900;
      case AKEYCODE_Q: return 0x1000;
      case AKEYCODE_R: return 0x1300;
      case AKEYCODE_S: return 0x1f00;
      case AKEYCODE_T: return 0x1400;
      case AKEYCODE_U: return 0x1600;
      case AKEYCODE_V: return 0x2f00;
      case AKEYCODE_W: return 0x1100;
      case AKEYCODE_X: return 0x2d00;
      case AKEYCODE_Y: return 0x1500;
      case AKEYCODE_Z: return 0x2c00;

      case AKEYCODE_0: return 0x8100;
      case AKEYCODE_1: return 0x7800;
      case AKEYCODE_2: return 0x7900;
      case AKEYCODE_3: return 0x7a00;
      case AKEYCODE_4: return 0x7b00;
      case AKEYCODE_5: return 0x7c00;
      case AKEYCODE_6: return 0x7d00;
      case AKEYCODE_7: return 0x7e00;
      case AKEYCODE_8: return 0x7f00;
      case AKEYCODE_9: return 0x8000;

      case AKEYCODE_DPAD_UP:    return 0xc800; // Arrow up
      case AKEYCODE_DPAD_DOWN:  return 0xd000; // Arrow down
      case AKEYCODE_DPAD_LEFT:  return 0xcb00; // Arrow left
      case AKEYCODE_DPAD_RIGHT: return 0xcd00; // Arrow right

      case AKEYCODE_SPACE:      return 0x3900; // Space
      case AKEYCODE_ENTER:      return 0x1c00; // Enter
      case AKEYCODE_DEL:        return 0x0e00; // Backspace
      case AKEYCODE_FORWARD_DEL:return 0xd300; // Delete
      case AKEYCODE_TAB:        return 0x0f00; // Tab
      case AKEYCODE_CAPS_LOCK:  return 0x3a00; // Caps Lock
      case AKEYCODE_CTRL_LEFT:  return 0x1d00; // LCtrl
      case AKEYCODE_CTRL_RIGHT: return 0x9d00; // RCtrl
      case AKEYCODE_SHIFT_LEFT: return 0x2a00; // LShift
      case AKEYCODE_SHIFT_RIGHT:return 0x3600; // RShift
      case AKEYCODE_ALT_LEFT:   return 0x3800; // LAlt
      case AKEYCODE_ALT_RIGHT:  return 0xb800; // RAlt

      case AKEYCODE_BUTTON_A:   return 0x3800; // Jump - Alt
      case AKEYCODE_BUTTON_B:   return 0x1d00; // Action - Ctrl
      case AKEYCODE_BUTTON_X:   return 0x3900; // Weapon/Action
      case AKEYCODE_BUTTON_Y:   return 0x1e00; // Additional action

      case AKEYCODE_BACK:
      case AKEYCODE_ESCAPE:     return 0x0100; // Escape
      case AKEYCODE_BUTTON_START:
      case AKEYCODE_MENU:       return 0x0100; // Menu / Escape
      default:
        return 0;
      }
    }
}

#ifndef NDEBUG
bool AndroidInputBackend::verboseLogging = true;
#else
bool AndroidInputBackend::verboseLogging = false;
#endif

AndroidInputBackend::AndroidInputBackend() {
  g_inst = this;
  }

AndroidInputBackend::~AndroidInputBackend() {
  if(g_inst==this)
    g_inst=nullptr;
  }

void AndroidInputBackend::setVerboseLogging(bool v) {
  verboseLogging = v;
  }

void AndroidInputBackend::setKeyCallback(KeyCallback cb) {
  keyCb = std::move(cb);
  }

void AndroidInputBackend::setMotionCallback(MotionCallback cb) {
  motionCb = std::move(cb);
  }

void AndroidInputBackend::pollEvents() {
  // Pump looper so queued input events trigger callbacks
  ALooper_pollAll(0,nullptr,nullptr,nullptr);
  }

int32_t AndroidInputBackend::onInputEvent(AInputEvent* event) {
  if(event==nullptr)
    return 0;

  InputEventData d;
  int32_t rawSrc = AInputEvent_getSource(event);
  d.source   = mapSource(rawSrc);
  d.deviceId = AInputEvent_getDeviceId(event);

  switch(AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_KEY: {
      int32_t raw  = AKeyEvent_getKeyCode(event);
      d.keyCode    = mapKey(raw);
      d.pressed    = (AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN);
      bool special = (raw==AKEYCODE_BACK || raw==AKEYCODE_MENU ||
                      raw==AKEYCODE_VOLUME_UP || raw==AKEYCODE_VOLUME_DOWN);
      d.type       = special ? InputEventType::SPECIAL : InputEventType::KEY;
      d.eventTime  = static_cast<uint64_t>(AKeyEvent_getEventTime(event));
      if(d.deviceId<0 || rawSrc==0)
        return 0;

      if(d.keyCode!=0) {
        if(sameEvent(d,lastEvent)) {
          LOGI("Duplicate event skipped: { \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d }",
               typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode);
          return 0;
        }
        lastEvent = d;
        if(keyCb)
          keyCb(d.keyCode, d.pressed);
        else
          logEvent(nullptr, d);
        return 1;
      }

      logEvent("unmapped", d);
      break;
      }
    case AINPUT_EVENT_TYPE_MOTION: {
      d.type      = InputEventType::MOTION;
      d.eventTime = static_cast<uint64_t>(AMotionEvent_getEventTime(reinterpret_cast<AMotionEvent*>(event)));
      if(rawSrc & (AINPUT_SOURCE_JOYSTICK|AINPUT_SOURCE_GAMEPAD)) {
        d.x = AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_X, 0);
        d.y = -AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_Y, 0);
        float rx = AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_Z, 0);
        float ry = -AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_RZ, 0);
        if(!validCoords(d.x,d.y)) {
          d.x = 0.f;
          d.y = 0.f;
        }
        if(!validCoords(rx,ry)) {
          rx = 0.f;
          ry = 0.f;
        }
        if(d.deviceId>=0 && !sameEvent(d,lastEvent)) {
          lastEvent = d;
          if(motionCb) {
            motionCb(d.x, d.y);
            motionCb(rx, ry);
          } else {
            logEvent("joyL", d);
            InputEventData r = d; r.x = rx; r.y = ry; logEvent("joyR", r);
          }
        } else if(d.deviceId>=0) {
          LOGI("Duplicate event skipped: { \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d }",
               typeToStr(d.type), sourceToStr(d.source), d.deviceId);
        }
      } else if(rawSrc & AINPUT_SOURCE_TOUCHSCREEN) {
        size_t cnt = AMotionEvent_getPointerCount(reinterpret_cast<AMotionEvent*>(event));
        for(size_t i=0;i<cnt;++i) {
          d.x = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), i);
          d.y = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), i);
          if(!validCoords(d.x,d.y))
            continue;
          if(d.deviceId>=0 && !sameEvent(d,lastEvent)) {
            lastEvent = d;
            if(motionCb)
              motionCb(d.x, d.y);
            else {
              char buf[32];
              std::snprintf(buf,sizeof(buf),"touch%zu",i);
              logEvent(buf, d);
            }
          } else if(d.deviceId>=0) {
            LOGI("Duplicate event skipped: { \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d }",
                 typeToStr(d.type), sourceToStr(d.source), d.deviceId);
          }
        }
      } else {
        logEvent("unhandled-motion", d);
      }
      return 1;
      }
    default:
      d.type = InputEventType::UNCLASSIFIED;
      logEvent("unclassified", d);
      break;
    }
  return 0;
  }

extern "C" int32_t onInputEvent(AInputEvent* event) {
  if(g_inst)
    return g_inst->onInputEvent(event);
  return 0;
  }

