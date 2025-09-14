#include "vrnav.h"

#ifdef OPENXR_ENABLED
#include "../world/world.h"
#include "../physics/dynamicworld.h"

VRNav::VRNav(World& w)
  : world(w) {
}

bool VRNav::nearestWalkable(const Tempest::Vec3& hint, Tempest::Vec3& out, float maxSlopeDeg) {
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

bool VRNav::localReplan(const Tempest::Vec3& from, const Tempest::Vec3& to, Tempest::Vec3& out, float maxRadiusMeters) {
  float maxDist = maxRadiusMeters*100.f;
  Tempest::Vec3 diff = to - from;
  diff.y = 0.f;
  float len = std::sqrt(diff.x*diff.x + diff.z*diff.z);
  if(len > maxDist)
    return false;
  if(nearestWalkable(to, out, 90.f))
    return true;
  return false;
}

bool VRNav::isWalkable(const Tempest::Vec3& pos, float maxSlopeDeg) {
  Tempest::Vec3 tmp{};
  return nearestWalkable(pos, tmp, maxSlopeDeg);
}

#endif
