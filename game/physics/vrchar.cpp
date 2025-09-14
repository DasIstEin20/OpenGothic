#include "vrchar.h"

#ifdef OPENXR_ENABLED
#include "../world/world.h"
#include "../world/objects/npc.h"
#include "dynamicworld.h"
#include <algorithm>

VRCharacter::VRCharacter(World& w)
  : world(w) {
}

VRCharMove VRCharacter::predictMove(const Tempest::Vec3& pos, float /*yaw*/,
                                    const Tempest::Vec3& v, float dt,
                                    const VRCharConfig& cfg) const {
  VRCharMove ret{};
  auto npc = world.player();
  if(npc==nullptr) {
    ret.proposedPos = pos;
    return ret;
  }

  Tempest::Vec3 to = pos + v*dt;
  DynamicWorld::CollisionTest out;
  auto& item = npc->physic;
  bool ok = item.testMove(to, out);
  Tempest::Vec3 np = ok ? to : out.partial;
  ret.hitObstacle = !ok;

  if(auto phys = world.physic()) {
    auto r = phys->landRay(np, cfg.groundProbe*100.f);
    if(r.hasCol) {
      float slope = std::acos(std::clamp(r.n.y,-1.f,1.f))*180.f/float(M_PI);
      ret.groundSlopeDeg = slope;
      if(slope<=cfg.slopeLimitDeg) {
        np.y = r.v.y;
        ret.onGround = true;
      }
    } else {
      ret.onGround = false;
    }

    Tempest::Vec3 dir = v;
    dir.y = 0.f;
    float len = std::sqrt(dir.x*dir.x + dir.z*dir.z);
    if(len>0.001f) {
      dir *= (1.f/len);
      Tempest::Vec3 ahead = np + dir*(cfg.edgeStop*100.f);
      auto r2 = phys->landRay(ahead, cfg.groundProbe*100.f);
      ret.nearLedge = !r2.hasCol;
    }
  }

  ret.steppedUp = np.y > pos.y + cfg.stepOffset*100.f*0.5f;
  ret.proposedPos = np;
  return ret;
}

float VRCharacter::raycastDown(const Tempest::Vec3& pos, float maxDist, Tempest::Vec3& normal) const {
  if(auto phys = world.physic()) {
    auto r = phys->landRay(pos,maxDist);
    normal = r.n;
    return r.v.y;
  }
  normal = {0,1,0};
  return pos.y - maxDist;
}

#endif
