#include "vrnav.h"

#ifdef OPENXR_ENABLED
#include "../world/world.h"
#include "../physics/dynamicworld.h"

VRNav::VRNav(World& w)
  : world(w) {
}

bool VRNav::findWalkable(const Tempest::Vec3& hint, Tempest::Vec3& out, float maxSlopeDeg) {
  if(auto phys = world.physic()) {
    auto r = phys->landRay(hint, 5000.f);
    if(r.hasCol) {
      float slope = std::acos(std::clamp(r.n.y,-1.f,1.f))*180.f/float(M_PI);
      if(slope <= maxSlopeDeg) {
        out = r.v;
        return true;
      }
    }
  }
  return false;
}

#endif
