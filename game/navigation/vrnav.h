#pragma once

#ifdef OPENXR_ENABLED
#include <Tempest/Vec3>

class World;

class VRNav {
  public:
    explicit VRNav(World& w);
    bool nearestWalkable(const Tempest::Vec3& hint, Tempest::Vec3& out, float maxSlopeDeg);
    bool localReplan(const Tempest::Vec3& from, const Tempest::Vec3& to, Tempest::Vec3& out, float maxRadiusMeters);
    bool isWalkable(const Tempest::Vec3& pos, float maxSlopeDeg);
    // compatibility helper
    bool findWalkable(const Tempest::Vec3& hint, Tempest::Vec3& out, float maxSlopeDeg) {
      return nearestWalkable(hint, out, maxSlopeDeg);
    }
  private:
    World& world;
};
#endif
