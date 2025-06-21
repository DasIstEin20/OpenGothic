#include "../backend/android/AndroidInputBackend.h"
#ifdef ENABLE_GESTURE_TRACKING
#include "../backend/android/GestureRecognizer.h"
#endif
#include <cassert>
#include <cmath>
int main(){
  InputEventData a{};
  a.type = InputEventType::KEY;
  a.source = InputEventSource::KEYBOARD;
  a.deviceId = 1;
  a.keyCode = 42;
  a.pressed = true;
  a.x = 1.0f;
  a.y = -2.0f;

  InputEventData b = a;
  assert(sameEvent(a,b));

  b.x += 0.00005f;
  b.y -= 0.00005f;
  assert(sameEvent(a,b));

  b.x += 0.001f;
  assert(!sameEvent(a,b));

  assert(validCoords(0.f,0.f));
  assert(!validCoords(NAN,0.f));
  assert(!validCoords(0.f,INFINITY));

  a.keyCode = 0x1e00; // 'A'
  assert(isRepeatableEvent(a));
  a.keyCode = 0x1d00; // LCtrl
  assert(!isRepeatableEvent(a));
#ifdef ENABLE_GESTURE_TRACKING
  std::vector<std::pair<float,float>> s{{0.f,0.f},{100.f,0.f}};
  std::vector<std::pair<float,float>> e{{60.f,0.f},{160.f,0.f}};
  assert(classifyGesture(s,e)==GestureType::SWIPE);

  e = {{-20.f,0.f},{120.f,0.f}};
  assert(classifyGesture(s,e)==GestureType::PINCH_OUT);

  e = {{20.f,0.f},{80.f,0.f}};
  assert(classifyGesture(s,e)==GestureType::PINCH_IN);

  s = {{0.f,0.f},{0.f,100.f}};
  e = {{0.f,0.f},{100.f,0.f}};
  assert(classifyGesture(s,e)==GestureType::ROTATE);
#endif

#ifdef ENABLE_SEQUENCE_TRACKING
  // test generateSequenceId increments
  uint64_t id1 = generateSequenceId();
  uint64_t id2 = generateSequenceId();
  assert(id2 == id1 + 1);

  // simulate sequence tracking for consecutive key press/release
  uint64_t currentSeqId = 0;
  uint64_t seqStart     = 0;
  uint64_t lastTime     = 0;
  bool     active       = false;
  int32_t  seqKey       = 0;

  InputEventData e1{};
  e1.type = InputEventType::KEY;
  e1.source = InputEventSource::KEYBOARD;
  e1.deviceId = 1;
  e1.keyCode = 10;
  e1.pressed = true;
  e1.eventTime = 1000;

  if(e1.pressed && (!active || e1.keyCode!=seqKey || e1.eventTime-lastTime>200)){
    currentSeqId = generateSequenceId();
    seqStart = e1.eventTime;
    active   = true;
    seqKey   = e1.keyCode;
  }
  if(active)
    e1.sequenceId = currentSeqId;
  lastTime = e1.eventTime;

  InputEventData e2 = e1;
  e2.pressed = false;
  e2.eventTime += 100; // within timeout
  if(e2.pressed && (!active || e2.keyCode!=seqKey || e2.eventTime-lastTime>200)){
    currentSeqId = generateSequenceId();
    seqStart = e2.eventTime;
    active   = true;
    seqKey   = e2.keyCode;
  }
  if(!e2.pressed && active)
    e2.longPress = isLongPress(seqStart, e2.eventTime);
  if(active)
    e2.sequenceId = currentSeqId;
  if(!e2.pressed)
    active = false;
  lastTime = e2.eventTime;

  assert(e1.sequenceId == e2.sequenceId);

  InputEventData e3 = e1; // new press after timeout
  e3.eventTime += 400;
  e3.pressed = true;
  if(e3.pressed && (!active || e3.keyCode!=seqKey || e3.eventTime-lastTime>200)){
    currentSeqId = generateSequenceId();
    seqStart = e3.eventTime;
    active   = true;
    seqKey   = e3.keyCode;
  }
  if(active)
    e3.sequenceId = currentSeqId;

  assert(e3.sequenceId != e2.sequenceId);
#endif
  return 0;
}
