#include "../backend/android/AndroidInputBackend.h"
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
  return 0;
}
