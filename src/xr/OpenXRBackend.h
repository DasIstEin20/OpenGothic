#pragma once
#ifdef OPENXR_ENABLED
#include <xr/IXRBackend.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <array>

class OpenXRBackend : public IXRBackend {
public:
  OpenXRBackend();
  ~OpenXRBackend() override;

  bool initialize(Tempest::Device& dev, Tempest::Window& win) override;
  void shutdown() override;
  bool beginFrame() override;
  void endFrame() override;
  std::array<EyeInfo,2> views() const override;

private:
  XrInstance    instance = XR_NULL_HANDLE;
  XrSystemId    systemId = XR_NULL_SYSTEM_ID;
  XrSession     session  = XR_NULL_HANDLE;
  XrSpace       space    = XR_NULL_HANDLE;
  XrFrameState  frameState{XR_TYPE_FRAME_STATE};
};
#endif
