#pragma once

#ifdef OPENXR_ENABLED
#include <Tempest/Vec3>

class World;

class VRNav {
  public:
    explicit VRNav(World& w);
    bool findWalkable(const Tempest::Vec3& hint, Tempest::Vec3& out, float maxSlopeDeg);
  private:
    World& world;
};
#endif
