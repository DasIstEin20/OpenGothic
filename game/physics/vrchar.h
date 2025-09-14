#pragma once

#ifdef OPENXR_ENABLED
#include <Tempest/Vec3>

class World;
class Npc;

class VRCharacter {
  public:
    explicit VRCharacter(World& world);

    void setTransform(const Tempest::Vec3& pos, float yaw);
    Tempest::Vec3 move(const Tempest::Vec3& v, float dt, bool& grounded);
    float raycastDown(const Tempest::Vec3& pos, float maxDist, Tempest::Vec3& normal);

  private:
    World&  world;
    Npc*    npc = nullptr;
    Tempest::Vec3 pos{};
    float   yaw = 0.f;
    float   stepOffset = 30.f;
    float   slopeLimitDeg = 45.f;
    float   lastGround = 0.f;
};
#endif
