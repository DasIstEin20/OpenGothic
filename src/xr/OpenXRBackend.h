#pragma once
#ifdef OPENXR_ENABLED
#include <xr/IXRBackend.h>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <array>
#include <vector>
#include <vulkan/vulkan.h>

class OpenXRBackend : public IXRBackend {
public:
  OpenXRBackend();
  ~OpenXRBackend() override;

  bool initialize(Tempest::Device& dev, Tempest::Window& win) override;
  void shutdown() override;
  bool beginFrame() override;
  void endFrame() override;
  std::array<EyeInfo,2> views() const override;
  void pollInput() override;
  const XRInputState& inputState() const override { return input; }
  void hapticPulse(XRHand hand, float amplitude, float seconds) override;
  void setUiQuad(const XRQuadLayerDesc* q) override;
  Tempest::Vec3 headPosition() const override;
  Tempest::Vec4 headOrientation() const override;
  XRHandState handState(XRHand hand) const override { return hands[size_t(hand)]; }

  bool   isVisible() const override;
  bool   isRunning() const override;
  double xrDeltaSeconds() const override { return dt; }
  void   recenter() override;
  void   setRenderScale(float s);

private:
  struct Eye {
    XrSwapchain                             swapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageVulkan2KHR> images;
    uint32_t                                width  = 0;
    uint32_t                                height = 0;
    uint32_t                                baseWidth  = 0;
    uint32_t                                baseHeight = 0;
    uint32_t                                sampleCount = 1;
    VkFormat                                format = VK_FORMAT_R8G8B8A8_UNORM;
    uint32_t                                acquired = 0;
    XrView                                  view{XR_TYPE_VIEW};
    Tempest::Matrix4x4                      viewMat;
    Tempest::Matrix4x4                      projMat;
    Tempest::Attachment                     color; // per-frame wrapped image
  };

  enum class SessionState : uint8_t {
    Idle,
    Ready,
    Synchronized,
    Visible,
    Focused,
    Stopping,
    Exiting,
  };

  enum class LogLevel : uint8_t {
    Off,
    Basic,
    Verbose,
  };

  Tempest::Device* device = nullptr;

  PFN_xrGetVulkanInstanceExtensionsKHR pfnGetInstanceExtensions = nullptr;
  PFN_xrGetVulkanDeviceExtensionsKHR   pfnGetDeviceExtensions   = nullptr;

  std::array<Eye,2> eyes;

  XrInstance    instance   = XR_NULL_HANDLE;
  XrSystemId    systemId   = XR_NULL_SYSTEM_ID;
  XrSession     session    = XR_NULL_HANDLE;
  XrSpace       refSpace   = XR_NULL_HANDLE;
  XrSpace       headSpace  = XR_NULL_HANDLE;
  XrPosef       headPose{{0,0,0,1},{0,0,0}};
  bool          hasPose    = false;
  bool          firstPerson = true;
  float         heightOffset = 1.75f;
  bool          allowRoll    = false;
  bool          loggedFp     = false;
  XrFrameState  frameState{XR_TYPE_FRAME_STATE};

  XrActionSet   actionSet   = XR_NULL_HANDLE;
  XrAction      moveAction  = XR_NULL_HANDLE;
  XrAction      turnAction  = XR_NULL_HANDLE;
  XrAction      jumpAction  = XR_NULL_HANDLE;
  XrAction      attackAction= XR_NULL_HANDLE;
  XrAction      interactAction=XR_NULL_HANDLE;
  XrAction      menuAction  = XR_NULL_HANDLE;
  XrAction      teleportAction=XR_NULL_HANDLE;
  XrAction      aimPoseAction=XR_NULL_HANDLE;
  XrAction      gripPoseAction=XR_NULL_HANDLE;
  XrAction      squeezeAction =XR_NULL_HANDLE;
  XrAction      hapticAction=XR_NULL_HANDLE;
  XrSpace       aimSpace[2] = {XR_NULL_HANDLE,XR_NULL_HANDLE};
  XrSpace       gripSpace[2]= {XR_NULL_HANDLE,XR_NULL_HANDLE};
  XrPath        handPath[2] = {XR_NULL_PATH,XR_NULL_PATH};
  XRInputState  input{};
  XRHandState   hands[2];

  SessionState  state       = SessionState::Idle;
  LogLevel      logLevel    = LogLevel::Basic;
  bool          visible     = false;
  bool          running     = false;
  double        dt          = 0.0;
  XrTime        lastPredicted = 0;
  bool          loggedFps   = false;
  float         renderScale = 1.f;
  size_t        dominantHand = 1;
  bool          simpleController = false;
  float         turnDeadzone = 0.25f;
  bool          profileQueried = false;

  XRQuadLayerDesc uiReq{};
  bool            haveUi = false;
  XrSwapchain                             uiSwapchain = XR_NULL_HANDLE;
  VkFormat                                uiFormat    = VK_FORMAT_R8G8B8A8_UNORM;
  int                                     uiW = 0;
  int                                     uiH = 0;
  std::vector<XrSwapchainImageVulkan2KHR> uiImages;

  bool createSwapchains();
  void destroySwapchains();
  void pollEvents();
};
#endif
