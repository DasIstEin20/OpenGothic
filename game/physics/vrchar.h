#pragma once

#ifdef OPENXR_ENABLED
#include <Tempest/Vec3>

class World;
class Npc;

struct VRCharMove {
  Tempest::Vec3 proposedPos{};
  float         proposedYaw = 0.f;
  bool          hitObstacle = false;
  bool          steppedUp   = false;
  bool          onGround    = false;
  bool          nearLedge   = false;
  float         groundSlopeDeg = 0.f;
};

struct VRCharConfig {
  float stepOffset    = 0.3f;   // meters
  float slopeLimitDeg = 45.f;
  float edgeStop      = 0.25f;  // meters
  float groundProbe   = 1.f;    // meters
};

class VRCharacter {
  public:
    explicit VRCharacter(World& world);

    VRCharMove predictMove(const Tempest::Vec3& pos, float yaw,
                           const Tempest::Vec3& v, float dt,
                           const VRCharConfig& cfg) const;
    float raycastDown(const Tempest::Vec3& pos, float maxDist, Tempest::Vec3& normal) const;

  private:
    World&  world;
};
#endif
