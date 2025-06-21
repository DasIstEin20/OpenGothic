#include "AndroidInputBackend.h"
#include <android/looper.h>
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"InputBackend",__VA_ARGS__)
#include <cstring>

namespace {
  AndroidInputBackend* g_inst = nullptr;

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
      if(code!=0) {
        bool pressed = (AKeyEvent_getAction(event)==AKEY_EVENT_ACTION_DOWN);
        if(keyCb)
          keyCb(code, pressed);
        else
          LOGI("key %d %s", code, pressed?"down":"up");
        return 1;
        }
      break;
      }
    case AINPUT_EVENT_TYPE_MOTION: {
      const int32_t src = AInputEvent_getSource(event);
      float x=0.f, y=0.f;
      if(src & (AINPUT_SOURCE_JOYSTICK|AINPUT_SOURCE_GAMEPAD)) {
        x = AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_X, 0);
        y = -AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_Y, 0);
        if(motionCb)
          motionCb(x,y);
        else
          LOGI("joy %.2f %.2f", x, y);
        x = AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_Z, 0);
        y = -AMotionEvent_getAxisValue(reinterpret_cast<AMotionEvent*>(event), AMOTION_EVENT_AXIS_RZ, 0);
      } else if(src & AINPUT_SOURCE_TOUCHSCREEN) {
        x = AMotionEvent_getX(reinterpret_cast<AMotionEvent*>(event),0);
        y = AMotionEvent_getY(reinterpret_cast<AMotionEvent*>(event),0);
      }
      if(motionCb)
        motionCb(x,y);
      else
        LOGI("motion %.2f %.2f", x, y);
      return 1;
      }
    }
  return 0;
  }

extern "C" int32_t onInputEvent(AInputEvent* event) {
  if(g_inst)
    return g_inst->onInputEvent(event);
  return 0;
  }

