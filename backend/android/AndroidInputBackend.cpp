#include "AndroidInputBackend.h"
#include "GestureRecognizer.h"
#include <android/looper.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"InputBackend",__VA_ARGS__)
#include <cstring>
#include <vector>
#include <utility>
#include <cmath>

namespace {
  AndroidInputBackend* g_inst = nullptr;

#ifdef ENABLE_GESTURE_TRACKING
  struct GestureState {
    bool                         active    = false;
    uint64_t                     startTime = 0;
    std::vector<std::pair<float,float>> startPos;
    std::vector<std::pair<float,float>> lastPos;
    uint32_t                     idCounter = 1;
  } g_gesture;

  const char* gestureToStr(GestureType g) {
    switch(g) {
      case GestureType::SWIPE:     return "SWIPE";
      case GestureType::PINCH_IN:  return "PINCH_IN";
      case GestureType::PINCH_OUT: return "PINCH_OUT";
      case GestureType::ROTATE:    return "ROTATE";
      default:                     return "NONE";
      }
    }

  void logGesture(const InputEventData& d){
    if(!AndroidInputBackend::verboseLogging)
      return;
    LOGI("{ \"gesture\": \"%s\", \"id\": %u, \"fingers\": %u, \"duration\": %llu }",
         gestureToStr(d.gesture), d.gestureId, d.fingerCount,
         static_cast<unsigned long long>(d.gestureDuration));
    }
#endif

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
    if(d.type==InputEventType::KEY) {
      if(msg)
        LOGI(
             "{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"repeatable\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu"
#ifdef ENABLE_SEQUENCE_TRACKING
             ", \"seq\": %llu, \"longPress\": %s"
#endif
             ", \"msg\": \"%s\" }",
             typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
             d.pressed?"true":"false", d.repeatable?"true":"false", d.x, d.y,
             static_cast<unsigned long long>(d.eventTime)
#ifdef ENABLE_SEQUENCE_TRACKING
             , static_cast<unsigned long long>(d.sequenceId), d.longPress?"true":"false"
#endif
             , msg);
      else
        LOGI(
             "{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"repeatable\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu"
#ifdef ENABLE_SEQUENCE_TRACKING
             ", \"seq\": %llu, \"longPress\": %s"
#endif
             " }",
             typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
             d.pressed?"true":"false", d.repeatable?"true":"false", d.x, d.y,
             static_cast<unsigned long long>(d.eventTime)
#ifdef ENABLE_SEQUENCE_TRACKING
             , static_cast<unsigned long long>(d.sequenceId), d.longPress?"true":"false"
#endif
             );
    } else {
      if(msg)
        LOGI(
             "{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu"
#ifdef ENABLE_SEQUENCE_TRACKING
             ", \"seq\": %llu"
#endif
             ", \"msg\": \"%s\" }",
             typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
             d.pressed?"true":"false", d.x, d.y,
             static_cast<unsigned long long>(d.eventTime)
#ifdef ENABLE_SEQUENCE_TRACKING
             , static_cast<unsigned long long>(d.sequenceId)
#endif
             , msg);
      else
        LOGI(
             "{ \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d, \"key\": %d, \"pressed\": %s, \"x\": %.2f, \"y\": %.2f, \"eventTime\": %llu"
#ifdef ENABLE_SEQUENCE_TRACKING
             ", \"seq\": %llu"
#endif
             " }",
             typeToStr(d.type), sourceToStr(d.source), d.deviceId, d.keyCode,
             d.pressed?"true":"false", d.x, d.y,
             static_cast<unsigned long long>(d.eventTime)
#ifdef ENABLE_SEQUENCE_TRACKING
             , static_cast<unsigned long long>(d.sequenceId)
#endif
             );
    }
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
#ifdef ENABLE_SEQUENCE_TRACKING
  const uint64_t seqTimeoutMs = 200;
#endif

  switch(AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_KEY: {
      int32_t raw  = AKeyEvent_getKeyCode(event);
      d.keyCode    = mapKey(raw);
      d.pressed    = (AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN);
      bool special = (raw==AKEYCODE_BACK || raw==AKEYCODE_MENU ||
                      raw==AKEYCODE_VOLUME_UP || raw==AKEYCODE_VOLUME_DOWN);
      d.type       = special ? InputEventType::SPECIAL : InputEventType::KEY;
      d.eventTime  = static_cast<uint64_t>(AKeyEvent_getEventTime(event));
      d.repeatable = isRepeatableEvent(d);
#ifdef ENABLE_SEQUENCE_TRACKING
      if(d.pressed && (!seqActive || d.keyCode!=seqKey || d.eventTime-lastSeqTime>seqTimeoutMs)) {
        currentSeqId = generateSequenceId();
        seqStartTime = d.eventTime;
        seqActive    = true;
        seqKey       = d.keyCode;
      }
      if(!d.pressed && seqActive)
        d.longPress = isLongPress(seqStartTime,d.eventTime);
      if(seqActive)
        d.sequenceId = currentSeqId;
      if(!d.pressed)
        seqActive=false;
      lastSeqTime = d.eventTime;
#endif
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
#ifdef ENABLE_SEQUENCE_TRACKING
        if(!seqActive || d.eventTime-lastSeqTime>seqTimeoutMs) {
          currentSeqId = generateSequenceId();
          seqStartTime = d.eventTime;
          seqActive    = true;
          seqKey       = 0;
        }
        d.sequenceId = currentSeqId;
        lastSeqTime  = d.eventTime;
#endif
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
            InputEventData r = d; r.x = rx; r.y = ry;
            logEvent("joyR", r);
          }
        } else if(d.deviceId>=0) {
          LOGI("Duplicate event skipped: { \"type\": \"%s\", \"source\": \"%s\", \"dev\": %d }",
               typeToStr(d.type), sourceToStr(d.source), d.deviceId);
        }
      } else if(rawSrc & AINPUT_SOURCE_TOUCHSCREEN) {
        size_t cnt = AMotionEvent_getPointerCount(reinterpret_cast<AMotionEvent*>(event));
        int32_t action = AMotionEvent_getAction(reinterpret_cast<AMotionEvent*>(event));
        int32_t baseAction = action & AMOTION_EVENT_ACTION_MASK;
        int32_t idx = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
#ifdef ENABLE_GESTURE_TRACKING
        if(baseAction==AMOTION_EVENT_ACTION_DOWN){
          g_gesture.active   = true;
          g_gesture.startTime= d.eventTime;
          g_gesture.startPos.clear();
          g_gesture.lastPos.clear();
          for(size_t i=0;i<cnt;++i){
            float px = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), i);
            float py = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), i);
            g_gesture.startPos.push_back({px,py});
            g_gesture.lastPos.push_back({px,py});
          }
        } else if(baseAction==AMOTION_EVENT_ACTION_POINTER_DOWN && g_gesture.active){
          if(g_gesture.startPos.size()<cnt){
            float px = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), idx);
            float py = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), idx);
            g_gesture.startPos.push_back({px,py});
            g_gesture.lastPos.push_back({px,py});
          }
        } else if(baseAction==AMOTION_EVENT_ACTION_MOVE && g_gesture.active){
          for(size_t i=0;i<cnt && i<g_gesture.lastPos.size(); ++i){
            float px = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), i);
            float py = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), i);
            g_gesture.lastPos[i]={px,py};
          }
        } else if(baseAction==AMOTION_EVENT_ACTION_UP && g_gesture.active){
          for(size_t i=0;i<g_gesture.lastPos.size() && i<cnt; ++i){
            float px = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), i);
            float py = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), i);
            g_gesture.lastPos[i]={px,py};
          }
          InputEventData gd{};
          gd.type      = InputEventType::MOTION;
          gd.source    = InputEventSource::TOUCHSCREEN;
          gd.deviceId  = d.deviceId;
          gd.eventTime = d.eventTime;
          gd.gestureId = g_gesture.idCounter++;
          gd.fingerCount = static_cast<uint32_t>(g_gesture.startPos.size());
          gd.gestureDuration = d.eventTime - g_gesture.startTime;
          gd.touchClass = InputTouchClass::GESTURE;
          gd.gesture = classifyGesture(g_gesture.startPos,g_gesture.lastPos);
          logGesture(gd);
          g_gesture.active = false;
        }
#endif
        for(size_t i=0;i<cnt;++i) {
          d.x = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event), i);
          d.y = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event), i);
          d.pressed = !(baseAction==AMOTION_EVENT_ACTION_UP || (baseAction==AMOTION_EVENT_ACTION_POINTER_UP && idx==(int)i));
#ifdef ENABLE_GESTURE_TRACKING
          d.touchClass = (cnt>1) ? InputTouchClass::MULTI : InputTouchClass::SINGLE;
#endif
#ifdef ENABLE_SEQUENCE_TRACKING
          if(d.pressed && (!seqActive || d.eventTime-lastSeqTime>seqTimeoutMs)) {
            currentSeqId = generateSequenceId();
            seqStartTime = d.eventTime;
            seqActive    = true;
            seqKey       = 0;
          }
          d.sequenceId = currentSeqId;
          if(!d.pressed)
            seqActive = false;
          lastSeqTime = d.eventTime;
#endif
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

