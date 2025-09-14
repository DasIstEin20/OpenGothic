#include "vrgrab.h"

#ifdef OPENXR_ENABLED
#include "../world/world.h"
#include "../world/objects/item.h"
#include "../commandline.h"

VRGrabber::VRGrabber(World& w) : world(w) {
}

bool VRGrabber::tryGrab(IXRBackend::XRHand hand, const Tempest::Vec3& rayOrigin, const Tempest::Vec3& rayDir) {
  float maxDist = CommandLine::inst().vrGrabDistance();
  float radius  = CommandLine::inst().vrGrabRadius();
  Item* found = nullptr;
  for(int i=0;i<16 && found==nullptr;++i) {
    float t = maxDist * (float(i)/15.f);
    Tempest::Vec3 p = rayOrigin + rayDir*t;
    world.detectItem(p, radius, [&](Item& it){
      if(found==nullptr)
        found = &it;
    });
  }
  if(found) {
    slots[size_t(hand)].obj = found;
    return true;
  }
  return false;
}

void VRGrabber::update(IXRBackend::XRHand hand, const Tempest::Matrix4x4& gripPose, float /*dt*/) {
  Slot& s = slots[size_t(hand)];
  if(s.obj==nullptr)
    return;
  s.obj->setObjMatrix(gripPose);
}

void VRGrabber::release(IXRBackend::XRHand hand, const Tempest::Vec3& /*throwVelocity*/) {
  Slot& s = slots[size_t(hand)];
  if(s.obj==nullptr)
    return;
  s.obj = nullptr;
}

#endif
