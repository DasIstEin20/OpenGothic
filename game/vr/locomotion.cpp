#include "locomotion.h"
#ifdef OPENXR_ENABLED
#include <Tempest/Application>
#include <Tempest/Log>
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
    teleportOk = false;
    if(nav.nearestWalkable(teleHint, dst, cmd.vrTeleportMaxSlope())) {
      Vec3 out = dst;
      if(nav.localReplan(player.worldPos(), dst, out, 1.5f)) {
        player.vrSetWorldPose(out, player.yaw());
        teleportOk = true;
        rejectReason = nullptr;
      } else {
        rejectReason = "path";
      }
    } else {
      rejectReason = "nav";
    }
    teleReq = false;
  }

  Vec3 wish{in.move.x,0.f,in.move.y};
  if(wish.x!=0.f || wish.z!=0.f)
    wish = rotateYaw(player.yaw(), wish);

  Vec3 wishVel = wish * cmd.vrWalkMaxSpeed();
  Vec3 dv = wishVel - velocity;
  float maxDv = cmd.vrWalkAccel()*dt;
  float len = std::sqrt(dv.x*dv.x + dv.y*dv.y + dv.z*dv.z);
  if(len>maxDv && len>0.f)
    dv *= maxDv/len;
  velocity += dv;

  Vec3 pos0 = player.worldPos();
  VRCharConfig cfg{};
  cfg.stepOffset    = cmd.vrWalkStep();
  cfg.slopeLimitDeg = cmd.vrWalkSlope();
  cfg.edgeStop      = cmd.vrWalkEdgeStop();
  cfg.groundProbe   = cmd.vrWalkGroundProbe();
  auto mv = vrChar.predictMove(pos0, player.yaw(), velocity*100.f, float(dt), cfg);

  if(mv.nearLedge) {
    velocity.x = 0.f;
    velocity.z = 0.f;
  }

  Vec3 target = mv.proposedPos;
  if(cmd.vrWalkReplan() && mv.hitObstacle) {
    Vec3 out;
    if(nav.localReplan(pos0, target, out, 1.5f))
      target = out;
  }
  Vec3 dpos = target - pos0;
  if(dpos.x!=0.f || dpos.y!=0.f || dpos.z!=0.f)
    player.vrMoveDelta(dpos);
  grounded = mv.onGround;
  if(cmd.vrLog()==CommandLine::VrLog::Verbose && grounded!=wasGrounded) {
    if(grounded)
      Tempest::Log::d("vr: grounded");
    else
      Tempest::Log::d("vr: airborne");
  }
  wasGrounded = grounded;

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
