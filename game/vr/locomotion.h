#pragma once
#ifdef OPENXR_ENABLED
#include <Tempest/Vec3>
#include <xr/IXRBackend.h>
#include <cstdint>

class PlayerControl;
class VRCharacter;
class VRNav;
class CommandLine;

namespace vr {
class VRLocomotion {
  public:
    VRLocomotion(PlayerControl&, VRCharacter&, VRNav&, const CommandLine&);
    void setEnabled(bool e);
    void setSnapTurn(float deg);
    void smoothTurn(float yawRad);
    void requestTeleport(const Tempest::Vec3& hint, bool keepHeading);
    void tick(double dt, const IXRBackend::XRInputState& in);

    bool isGrounded() const { return grounded; }
    bool lastTeleportOk() const { return teleportOk; }
    const char* lastRejectReason() const { return rejectReason; }
  private:
    PlayerControl&  player;
    VRCharacter&    vrChar;
    VRNav&          nav;
    const CommandLine& cmd;

    bool            enabled = true;
    Tempest::Vec3   velocity{}; // m/s
    bool            grounded = false;

    float           queuedSnap = 0.f; // degrees
    float           queuedSmooth = 0.f; // radians
    uint64_t        lastSnapTime = 0;

    bool            teleReq = false;
    Tempest::Vec3   teleHint{};
    bool            teleKeepHeading = false;
    bool            teleportOk = false;
    const char*     rejectReason = nullptr;
    bool            teleClickPrev = false;
};
}
#endif
