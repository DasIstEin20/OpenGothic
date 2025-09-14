#include "locomotion.h"
#ifdef OPENXR_ENABLED
#include <Tempest/Application>
#include <cmath>
#include "../game/playercontrol.h"
#include "../physics/vrchar.h"
#include "../navigation/vrnav.h"
#include "../commandline.h"

using namespace Tempest;

vr::VRLocomotion::VRLocomotion(PlayerControl& pc, VRCharacter& ch, VRNav& nv, const CommandLine& cmd)
  : player(pc), vrChar(ch), nav(nv), cmd(cmd) {
}

void vr::VRLocomotion::setEnabled(bool e) {
  enabled = e;
}

void vr::VRLocomotion::setSnapTurn(float deg) {
  queuedSnap += deg;
}

void vr::VRLocomotion::smoothTurn(float yawRad) {
  queuedSmooth += yawRad;
}

void vr::VRLocomotion::requestTeleport(const Vec3& hint, bool keepHeading) {
  teleReq = true;
  teleHint = hint;
  teleKeepHeading = keepHeading;
}

static Vec3 rotateYaw(float yaw, const Vec3& v) {
  Vec3 r;
  r.x = v.x*std::cos(yaw) - v.z*std::sin(yaw);
  r.z = v.x*std::sin(yaw) + v.z*std::cos(yaw);
  r.y = v.y;
  return r;
}

void vr::VRLocomotion::tick(double dt, const IXRBackend::XRInputState& in) {
  if(!enabled)
    return;

  player.setVrJump(in.jump);
  player.setVrAttack(in.attack);
  player.setVrInteract(in.interact);
  player.setVrMenu(in.menu);

  if(cmd.vrTeleport() && in.teleportClick && !teleClickPrev && in.aim.valid) {
    requestTeleport(in.aim.pos + in.aim.dir*500.f, cmd.vrKeepHeading());
  }
  teleClickPrev = in.teleportClick;

  if(teleReq) {
    Vec3 dst{};
    if(nav.findWalkable(teleHint, dst, cmd.vrTeleportMaxSlope())) {
      player.vrSetWorldPose(dst, player.getYaw());
      teleportOk = true;
      rejectReason = nullptr;
    } else {
      teleportOk = false;
      rejectReason = "nav";
    }
    teleReq = false;
  }

  Vec3 wish{in.move.x,0.f,in.move.y};
  if(wish.x!=0.f || wish.z!=0.f)
    wish = rotateYaw(player.getYaw(), wish);

  Vec3 wishVel = wish * cmd.vrWalkMaxSpeed();
  Vec3 dv = wishVel - velocity;
  float maxDv = cmd.vrWalkAccel()*dt;
  float len = std::sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
  if(len>maxDv && len>0.f)
    dv *= maxDv/len;
  velocity += dv;

  Vec3 pos0 = player.getWorldPos();
  vrChar.setTransform(pos0, player.getYaw());
  Vec3 pos1 = vrChar.move(velocity*100.f, float(dt), grounded);
  Vec3 dpos = pos1 - pos0;
  if(dpos.x!=0.f || dpos.y!=0.f || dpos.z!=0.f)
    player.vrMoveDelta(dpos);

  float yawDelta = 0.f;
  if(cmd.vrIsSmoothTurn()) {
    yawDelta += queuedSmooth;
    queuedSmooth = 0.f;
    yawDelta += in.turnX * cmd.vrTurnSpeed() * float(dt) * float(M_PI/180.0);
    if(yawDelta!=0.f)
      player.vrRotateYaw(yawDelta);
  } else {
    uint64_t now = Tempest::Application::tickCount();
    if(std::fabs(in.turnX) > cmd.vrTurnDeadzone() && now - lastSnapTime > uint64_t(cmd.vrSnapCooldown())) {
      queuedSnap += (in.turnX>0.f?1.f:-1.f) * cmd.vrSnapAngle();
      lastSnapTime = now;
    }
    if(queuedSnap!=0.f) {
      player.vrRotateYaw(queuedSnap*float(M_PI/180.0));
      queuedSnap = 0.f;
    }
  }
}

#endif
