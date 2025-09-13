#pragma once
#include <array>
#include <Tempest/Matrix4x4>
#include <Tempest/Attachment>
namespace Tempest { class Device; class Window; }
struct EyeInfo {
  Tempest::Matrix4x4 view;
  Tempest::Matrix4x4 proj;
  Tempest::Attachment color;
  Tempest::Attachment depth;
};
class IXRBackend {
public:
  virtual ~IXRBackend() = default;
  virtual bool initialize(Tempest::Device& dev, Tempest::Window& win) = 0;
  virtual void shutdown() = 0;
  virtual bool beginFrame() = 0;
  virtual void endFrame() = 0;
  virtual std::array<EyeInfo,2> views() const = 0; // returns default/empty at P1
};
