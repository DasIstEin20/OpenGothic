#pragma once

#include <Tempest/Platform>
#include <Tempest/Dir>
#include <Tempest/Event>
#include <Tempest/Vec3>

#include <cstdint>
#include <stdexcept>
#include <string>

#include "game/constants.h"

class VersionInfo;
class GothicNotFoundException : std::logic_error {
  using logic_error::logic_error;
  };

class CommandLine {
  public:
    CommandLine(int argc,const char** argv);
    static const CommandLine& inst();

    enum GraphicBackend : uint8_t {
      Vulkan,
      DirectX12
      };
    auto                graphicsApi() const -> GraphicBackend;
    std::u16string_view rootPath() const;
    std::u16string      scriptPath() const;
    std::u16string      scriptPath(ScriptLang lang) const;
    std::u16string      cutscenePath() const;
    std::u16string      cutscenePath(ScriptLang lang) const;
    std::u16string_view modPath() const { return gmod; }
    std::u16string      nestedPath(const std::initializer_list<const char16_t*> &name, Tempest::Dir::FileType type) const;

    bool                isDevMode()        const { return devmode;      }
    bool                isValidationMode() const { return isDebug;      }
    bool                isWindowMode()     const { return isWindow;     }
    bool                isRayQuery()       const { return isRQuery;     }
    bool                isRtGi()           const { return isGi;         }
    bool                isMeshShading()    const { return isMeshSh;     }
    bool                isBindless()       const { return isBindlessSh; }
    bool                isVirtualShadow()  const { return isVsm;        }
    bool                isSoftwareShadow() const { return isRtSm;       }
    bool                doStartMenu()      const { return !noMenu;      }
    bool                isBenchmarkMode()  const { return isBenchmark;  }
    bool                doForceG1()        const { return forceG1;      }
    bool                doForceG2()        const { return forceG2;      }
    bool                doForceG2NR()      const { return forceG2NR;    }
    bool                aaPreset()         const { return aaPresetId;   }
    bool                isVr()            const;
    bool                isVrFirstPerson() const { return vrFirstPerson; }
    float               vrHeightOffset()  const { return vrHeight;      }
    bool                vrAllowRoll()     const { return allowRoll;     }
    int                 vrSnapAngle()     const { return vrSnap;        }
    bool                vrTeleport()      const { return vrUseTeleport; }
    bool                vrIsSmoothTurn()  const { return vrTurnSmooth;  }
    float               vrTurnSpeed()     const { return vrTurnSpeedVal;}
    float               vrTurnDeadzone()  const { return vrTurnDeadzoneVal;}
    int                 vrSnapCooldown()  const { return vrSnapCooldownVal;}
    float               vrMoveSpeedScale()const { return vrMoveScale;   }
    float               vrVignetteStrength() const { return vrVignette; }
    float               vrRenderScale()   const { return vrRenderScaleVal; }
    Tempest::Event::KeyType vrRecenterKey() const { return vrRecenterKeyVal; }
    bool                vrSeated()        const { return vrSeatedVal; }
    enum class VrHand { Left, Right };
    VrHand             vrDominantHand() const { return vrDominantHandVal; }
    enum class VrLog { Off, Basic, Verbose };
    VrLog              vrLog() const { return vrLogLevel; }
    bool               vrShowHands() const { return vrShowHandsVal; }
    enum class VrHandsMode { Controller, Ghost };
    VrHandsMode        vrHandsMode() const { return vrHandsModeVal; }
    bool               vrLaser() const { return vrLaserVal; }
    float              vrHandScale() const { return vrHandScaleVal; }
    Tempest::Vec3      vrHandColorLeft() const { return vrHandColorLeftVal; }
    Tempest::Vec3      vrHandColorRight() const { return vrHandColorRightVal; }
    bool               vrGrab() const { return vrGrabVal; }
    float              vrGrabDistance() const { return vrGrabDist; }
    float              vrGrabRadius() const { return vrGrabRadiusVal; }
    float              vrThrowScale() const { return vrThrowScaleVal; }
    bool               vrTeleportGrounded() const { return vrTeleGround; }
    float              vrTeleportMaxSlope() const { return vrTeleMaxSlope; }
    bool               vrHaptics() const { return vrHapticsVal; }
    float              vrWalkStep() const { return vrWalkStepVal; }
    float              vrWalkSlope() const { return vrWalkSlopeVal; }
    float              vrWalkAccel() const { return vrWalkAccelVal; }
    float              vrWalkMaxSpeed() const { return vrWalkMaxSpeedVal; }
    bool               vrKeepHeading() const { return vrKeepHeadingVal; }
    float              vrUiScrollScale() const { return vrUiScrollScaleVal; }
    float              vrUiLongPress() const { return vrUiLongPressVal; }

      // VR HUD parameters
      float               vrHudDistance()   const { return vrHudDist;    }
      float               vrHudWidth()      const { return vrHudWidthVal;   }
      float               vrHudScale()      const { return vrHudScaleVal;   }
      float               vrHudPitchDeg()   const { return vrHudPitch;   }
      bool                vrHudFollow()     const { return vrHudFollowVal;  }
      float               vrHudResScale()   const { return vrHudRes;     }
    std::string_view    defaultSave()      const { return saveDef;    }

    std::string         wrldDef;

  private:
    bool                validateGothicPath() const;

    GraphicBackend      graphics = GraphicBackend::Vulkan;
    std::u16string      gpath, gmod;
    std::u16string      gscript;
    std::u16string      gcutscene;
    std::string         saveDef;
    bool                devmode      = false;
    bool                noMenu       = false;
    bool                isBenchmark  = false;
    bool                isWindow     = false;
    bool                isDebug      = false;
#if defined(__OSX__)
    bool                isRQuery     = false;
    bool                isMeshSh     = false;
#else
    bool                isRQuery     = true;
    bool                isMeshSh     = true;
#endif
    bool                isBindlessSh = true;
    bool                isVsm        = false;
    bool                isRtSm       = false;
    bool                isGi         = false;
    bool                forceG1      = false;
    bool                forceG2      = false;
    bool                forceG2NR    = false;
    uint32_t            aaPresetId = 0;
    bool                vr          = false;
    bool                vrFirstPerson = true;
    float               vrHeight      = 1.75f;
    bool                allowRoll     = false;
    int                 vrSnap        = 30;
    bool                vrUseTeleport = true;
    bool                vrTurnSmooth  = false;
    float               vrTurnSpeedVal= 120.f;
    float               vrTurnDeadzoneVal= 0.25f;
    int                 vrSnapCooldownVal= 250;
    float               vrMoveScale   = 1.f;
    float               vrVignette    = 0.f;
    float               vrRenderScaleVal = 1.f;
    Tempest::Event::KeyType vrRecenterKeyVal = Tempest::Event::K_R;
    bool                vrSeatedVal   = false;
    VrHand              vrDominantHandVal = VrHand::Right;
    VrLog               vrLogLevel = VrLog::Basic;
    float               vrHudDist     = 1.4f;
    float               vrHudWidthVal = 1.0f;
    float               vrHudScaleVal = 1.0f;
    float               vrHudPitch    = -10.f;
    bool                vrHudFollowVal= true;
    float               vrHudRes      = 1.0f;
    bool                vrShowHandsVal = true;
    VrHandsMode         vrHandsModeVal = VrHandsMode::Controller;
    bool                vrLaserVal    = true;
    float               vrHandScaleVal= 1.f;
    Tempest::Vec3       vrHandColorLeftVal  = {0.2f,0.7f,1.0f};
    Tempest::Vec3       vrHandColorRightVal = {1.0f,0.5f,0.2f};
    bool                vrGrabVal = true;
    float               vrGrabDist = 3.f;
    float               vrGrabRadiusVal = 0.10f;
    float               vrThrowScaleVal = 1.f;
    bool                vrTeleGround = true;
    float               vrTeleMaxSlope = 45.f;
    bool                vrHapticsVal = true;
    float               vrWalkStepVal = 0.3f;
    float               vrWalkSlopeVal = 45.f;
    float               vrWalkAccelVal = 10.f;
    float               vrWalkMaxSpeedVal = 3.f;
    bool                vrKeepHeadingVal = true;
    float               vrUiScrollScaleVal = 1.f;
    float               vrUiLongPressVal = 0.45f;
  };

