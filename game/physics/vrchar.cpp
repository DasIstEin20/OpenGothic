#include "vrchar.h"

#ifdef OPENXR_ENABLED
#include "../world/world.h"
#include "../world/objects/npc.h"
#include "../commandline.h"
#include "dynamicworld.h"

VRCharacter::VRCharacter(World& w)
  : world(w) {
  stepOffset    = CommandLine::inst().vrWalkStep()*100.f;
  slopeLimitDeg = CommandLine::inst().vrWalkSlope();
}

void VRCharacter::setTransform(const Tempest::Vec3& pos, float /*yaw*/) {
  npc = world.player();
  if(npc)
    npc->setPosition(pos);
  lastGround = pos.y;
}

Tempest::Vec3 VRCharacter::move(const Tempest::Vec3& v, float dt, bool& grounded) {
  grounded = false;
  if(npc==nullptr)
    return {};

  Tempest::Vec3 dp = v*dt;
  npc->tryMove(dp);
  Tempest::Vec3 pos = npc->position();
  if(auto phys = world.physic()) {
    auto r = phys->landRay(pos, stepOffset*2.f);
    if(r.hasCol) {
      float slope = std::acos(std::clamp(r.n.y,-1.f,1.f))*180.f/float(M_PI);
      if(slope<=slopeLimitDeg) {
        pos.y = r.v.y;
        npc->setPosition(pos);
        grounded = true;
        lastGround = pos.y;
      }
    }
  }
  return npc->position();
}

float VRCharacter::raycastDown(const Tempest::Vec3& pos, float maxDist, Tempest::Vec3& normal) {
  if(auto phys = world.physic()) {
    auto r = phys->landRay(pos,maxDist);
    normal = r.n;
    return r.v.y;
  }
  normal = {0,1,0};
  return pos.y - maxDist;
}

#endif
