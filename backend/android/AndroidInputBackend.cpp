#include "AndroidInputBackend.h"
#include <android/looper.h>
#include <android/log.h>
#include <cstring>

namespace {
  AndroidInputBackend* g_inst = nullptr;

  int32_t mapKey(int32_t k){
    switch(k){
      case AKEYCODE_W: return 0x1100; // W key
      case AKEYCODE_S: return 0x1f00; // S key
      case AKEYCODE_A: return 0x1e00; // A key
      case AKEYCODE_D: return 0x2000; // D key

      case AKEYCODE_DPAD_UP:    return 0xc800; // Arrow up
      case AKEYCODE_DPAD_DOWN:  return 0xd000; // Arrow down
      case AKEYCODE_DPAD_LEFT:  return 0xcb00; // Arrow left
      case AKEYCODE_DPAD_RIGHT: return 0xcd00; // Arrow right

      case AKEYCODE_SPACE:      return 0x3900; // Space
      case AKEYCODE_BUTTON_A:   return 0x3800; // Jump - Alt
      case AKEYCODE_BUTTON_B:   return 0x1d00; // Action - Ctrl
      case AKEYCODE_BUTTON_X:   return 0x3900; // Weapon/Action
      case AKEYCODE_BUTTON_Y:   return 0x1e00; // Left? placeholder

      case AKEYCODE_BACK:       return 0x0100; // Escape
      case AKEYCODE_BUTTON_START:
      case AKEYCODE_MENU:       return 0x0100; // Menu / Escape
      default:
        return 0;
      }
    }
}

AndroidInputBackend::AndroidInputBackend() {
  g_inst = this;
  }

AndroidInputBackend::~AndroidInputBackend() {
  if(g_inst==this)
    g_inst=nullptr;
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

  switch(AInputEvent_getType(event)) {
    case AINPUT_EVENT_TYPE_KEY: {
      int32_t code = mapKey(AKeyEvent_getKeyCode(event));
      if(code!=0 && keyCb) {
        bool pressed = (AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN);
        keyCb(code, pressed);
        return 1;
        }
      break;
      }
    case AINPUT_EVENT_TYPE_MOTION: {
      if(motionCb) {
        const int32_t src = AInputEvent_getSource(event);
        float x=0.f, y=0.f;
        if(src & AINPUT_SOURCE_JOYSTICK) {
          x = AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_X, 0);
          y = -AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_Y, 0);
        } else if(src & AINPUT_SOURCE_TOUCHSCREEN) {
          x = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event),0);
          y = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event),0);
        }
        motionCb(x,y);
        return 1;
        }
      break;
      }
    }
  return 0;
  }

extern "C" int32_t onInputEvent(AInputEvent* event) {
  if(g_inst)
    return g_inst->onInputEvent(event);
  return 0;
  }

