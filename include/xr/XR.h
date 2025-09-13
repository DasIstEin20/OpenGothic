#pragma once
class IXRBackend;
namespace Tempest { class Device; class Window; }
class XR {
public:
  XR();
  ~XR();
  bool initialize(Tempest::Device& dev, Tempest::Window& win); // returns true if backend active
  IXRBackend* backend() const;
private:
  IXRBackend* impl_; // owned
};
