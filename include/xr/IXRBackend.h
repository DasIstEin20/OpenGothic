#pragma once
#include <array>
#include <Tempest/Matrix4x4>
#include <Tempest/Attachment>
#include <Tempest/Vec2>
#include <Tempest/Vec3>
#include <Tempest/Vec4>

namespace Tempest { class Device; class Window; }
struct EyeInfo {
  Tempest::Matrix4x4 view;
  Tempest::Matrix4x4 proj;
  Tempest::Attachment color; // wrapped external VkImage for this frame
  Tempest::Attachment depth; // optional depth buffer
};

#ifdef OPENXR_ENABLED
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>

struct XRQuadLayerDesc {
  VkImage  image = VK_NULL_HANDLE; // source color image
  int      width = 0;              // pixels
  int      height = 0;             // pixels
  float    metersWidth = 1.f;      // physical width of quad
  XrPosef  pose{};                 // center pose (quad faces -Z)
};
#endif
class IXRBackend {
public:
  virtual ~IXRBackend() = default;
  virtual bool initialize(Tempest::Device& dev, Tempest::Window& win) = 0;
  virtual void shutdown() = 0;
  virtual bool beginFrame() = 0;
  virtual void endFrame() = 0;
  virtual std::array<EyeInfo,2> views() const = 0; // returns default/empty at P1
#ifdef OPENXR_ENABLED
  virtual void setUiQuad(const XRQuadLayerDesc* q) = 0; // nullptr clears for this frame
#endif

  virtual Tempest::Vec3 headPosition() const = 0;
  virtual Tempest::Vec4 headOrientation() const = 0;

  struct XRInputState {
    struct Pose {
      Tempest::Vec3 pos{};
      Tempest::Vec3 dir{};
      bool          valid=false;
    };
    bool          haveControllers=false;
    Tempest::Vec2 move{};
    float         turnX=0.f;
    bool          jump=false;
    bool          interact=false;
    bool          attack=false;
    bool          menu=false;
    bool          teleportClick=false;
    Pose          aim{};
  };

  virtual void pollInput() = 0;
  virtual const XRInputState& inputState() const = 0;
  virtual void hapticPulse(float amplitude, float seconds) = 0;
};
