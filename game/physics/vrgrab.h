#pragma once

#include <array>

#ifdef OPENXR_ENABLED
#include <xr/IXRBackend.h>
#include <Tempest/Matrix4x4>
#include <Tempest/Vec3>

class World;
class Item;

struct VRPickupTag {
  bool  enabled = true;
  float mass    = 1.f;
};

class VRGrabber {
  public:
    explicit VRGrabber(World& world);

    bool tryGrab(IXRBackend::XRHand hand, const Tempest::Vec3& rayOrigin, const Tempest::Vec3& rayDir);
    void update(IXRBackend::XRHand hand, const Tempest::Matrix4x4& gripPose, float dt);
    void release(IXRBackend::XRHand hand, const Tempest::Vec3& throwVelocity);

  private:
    struct Slot {
      Item*          obj = nullptr;
    };

    World&               world;
    std::array<Slot,2>   slots;
};
#endif
