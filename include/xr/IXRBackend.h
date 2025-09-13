#pragma once
#include <array>
namespace Tempest { class Device; class Window; }
struct EyeInfo {
  // Placeholder for P2 (per-eye view/proj + Tempest attachments)
  // For P1 leave empty; provide size() via views()
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
