#ifdef OPENXR_ENABLED
#include "OpenXRBackend.h"

#include <Tempest/Log>
#include <Tempest/Device>
#include <Tempest/Window>

#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>

namespace {

struct XrMatrix4x4f {
  float m[4][4];
};

static XrMatrix4x4f xrMatIdentity() {
  XrMatrix4x4f r{};
  r.m[0][0] = r.m[1][1] = r.m[2][2] = r.m[3][3] = 1.f;
  return r;
}

static XrMatrix4x4f xrMatProjection(const XrFovf& fov, float nearZ, float farZ) {
  const float tanLeft   = std::tan(fov.angleLeft);
  const float tanRight  = std::tan(fov.angleRight);
  const float tanDown   = std::tan(fov.angleDown);
  const float tanUp     = std::tan(fov.angleUp);

  const float tanWidth  = tanRight - tanLeft;
  const float tanHeight = tanUp - tanDown;

  XrMatrix4x4f m{};
  m.m[0][0] =  2.f / tanWidth;
  m.m[1][1] =  2.f / tanHeight;
  m.m[2][0] =  (tanRight + tanLeft) / tanWidth;
  m.m[2][1] =  (tanUp + tanDown)   / tanHeight;
  m.m[2][2] =  (nearZ + farZ) / (nearZ - farZ);
  m.m[2][3] = -1.f;
  m.m[3][2] =  (nearZ * farZ * 2.f) / (nearZ - farZ);
  return m;
}

static XrMatrix4x4f xrMatView(const XrPosef& pose) {
  const float x = pose.orientation.x;
  const float y = pose.orientation.y;
  const float z = pose.orientation.z;
  const float w = pose.orientation.w;

  XrMatrix4x4f m = xrMatIdentity();
  m.m[0][0] = 1 - 2*y*y - 2*z*z;
  m.m[0][1] = 2*x*y - 2*w*z;
  m.m[0][2] = 2*x*z + 2*w*y;

  m.m[1][0] = 2*x*y + 2*w*z;
  m.m[1][1] = 1 - 2*x*x - 2*z*z;
  m.m[1][2] = 2*y*z - 2*w*x;

  m.m[2][0] = 2*x*z - 2*w*y;
  m.m[2][1] = 2*y*z + 2*w*x;
  m.m[2][2] = 1 - 2*x*x - 2*y*y;

  m.m[0][3] = -pose.position.x;
  m.m[1][3] = -pose.position.y;
  m.m[2][3] = -pose.position.z;
  return m;
}

static Tempest::Matrix4x4 toTempest(const XrMatrix4x4f& m) {
  Tempest::Matrix4x4 r = Tempest::Matrix4x4::mkIdentity();
  std::memcpy(&r, m.m, sizeof(m.m));
  return r;
}

static VkFormat pickFormat(const std::vector<int64_t>& formats) {
  const VkFormat prefer[] = {
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_FORMAT_R8G8B8A8_UNORM,
    VK_FORMAT_B8G8R8A8_SRGB,
    VK_FORMAT_B8G8R8A8_UNORM
  };
  for(VkFormat f:prefer)
    if(std::find(formats.begin(),formats.end(),(int64_t)f)!=formats.end())
      return f;
  return formats.empty() ? VK_FORMAT_R8G8B8A8_UNORM : (VkFormat)formats[0];
}

static Tempest::TextureFormat toTexFormat(VkFormat f) {
  (void)f;
  return Tempest::TextureFormat::RGBA8;
}

}

OpenXRBackend::OpenXRBackend() = default;
OpenXRBackend::~OpenXRBackend() {
  shutdown();
}

bool OpenXRBackend::initialize(Tempest::Device& dev, Tempest::Window& win) {
  (void)win;
  device = &dev;

  uint32_t extCount = 0;
  xrEnumerateInstanceExtensionProperties(nullptr, 0, &extCount, nullptr);
  std::vector<XrExtensionProperties> extProps(extCount, {XR_TYPE_EXTENSION_PROPERTIES});
  xrEnumerateInstanceExtensionProperties(nullptr, extCount, &extCount, extProps.data());

  std::vector<const char*> instExt{"XR_KHR_vulkan_enable2"};
  for(auto& e:extProps)
    if(std::strcmp(e.extensionName, "XR_EXT_debug_utils")==0)
      instExt.push_back("XR_EXT_debug_utils");

  XrInstanceCreateInfo ici{XR_TYPE_INSTANCE_CREATE_INFO};
  std::strcpy(ici.applicationInfo.applicationName, "OpenGothic");
  std::strcpy(ici.applicationInfo.engineName, "OpenGothic");
  ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  ici.enabledExtensionCount = uint32_t(instExt.size());
  ici.enabledExtensionNames = instExt.data();

  if(xrCreateInstance(&ici,&instance)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateInstance failed");
    return false;
  }

  XrInstanceProperties ip{XR_TYPE_INSTANCE_PROPERTIES};
  if(xrGetInstanceProperties(instance,&ip)==XR_SUCCESS)
    Tempest::Log::i("OpenXR runtime: ", ip.runtimeName);

  xrGetInstanceProcAddr(instance,"xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction*)&pfnGetInstanceExtensions);
  xrGetInstanceProcAddr(instance,"xrGetVulkanDeviceExtensionsKHR",   (PFN_xrVoidFunction*)&pfnGetDeviceExtensions);

  XrSystemGetInfo sgi{XR_TYPE_SYSTEM_GET_INFO};
  sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  if(xrGetSystem(instance,&sgi,&systemId)!=XR_SUCCESS) {
    Tempest::Log::e("xrGetSystem failed");
    shutdown();
    return false;
  }

  VkInstance       vkInst  = dev.vkInstance();
  VkPhysicalDevice vkPhys  = dev.vkPhysicalDevice();
  VkDevice         vkDev   = dev.vkDevice();
  VkQueue          vkQueue = dev.vkQueue();
  uint32_t         qFam    = dev.vkQueueFamily();

  XrGraphicsBindingVulkan2KHR bind{XR_TYPE_GRAPHICS_BINDING_VULKAN2_KHR};
  bind.instance          = vkInst;
  bind.physicalDevice    = vkPhys;
  bind.device            = vkDev;
  bind.queueFamilyIndex  = qFam;
  bind.queueIndex        = 0;
  bind.queue             = vkQueue;

  XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
  sci.systemId = systemId;
  sci.next     = &bind;
  if(xrCreateSession(instance,&sci,&session)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateSession failed");
    shutdown();
    return false;
  }

  XrReferenceSpaceCreateInfo rs{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  rs.poseInReferenceSpace.orientation.w = 1.f;
  if(xrCreateReferenceSpace(session,&rs,&space)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateReferenceSpace failed");
    shutdown();
    return false;
  }

  Tempest::Log::i("View configuration: PRIMARY_STEREO");

  XrViewConfigurationType viewType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  uint32_t viewCount = 0;
  xrEnumerateViewConfigurationViews(instance, systemId, viewType, 0, &viewCount, nullptr);
  std::vector<XrViewConfigurationView> cfg(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  xrEnumerateViewConfigurationViews(instance, systemId, viewType, viewCount, &viewCount, cfg.data());

  uint32_t formatCount = 0;
  xrEnumerateSwapchainFormats(session, 0, &formatCount, nullptr);
  std::vector<int64_t> formats(formatCount);
  xrEnumerateSwapchainFormats(session, formatCount, &formatCount, formats.data());
  VkFormat colorFormat = pickFormat(formats);
  Tempest::Log::i("Swapchain format: ", int(colorFormat));

  for(uint32_t i=0;i<viewCount && i<eyes.size();++i) {
    eyes[i].width  = cfg[i].recommendedImageRectWidth;
    eyes[i].height = cfg[i].recommendedImageRectHeight;
    eyes[i].format = colorFormat;

    XrSwapchainCreateInfo sc{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    sc.usageFlags   = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    sc.format       = colorFormat;
    sc.sampleCount  = cfg[i].recommendedSwapchainSampleCount;
    sc.width        = eyes[i].width;
    sc.height       = eyes[i].height;
    sc.faceCount    = 1;
    sc.arraySize    = 1;
    sc.mipCount     = 1;

    if(xrCreateSwapchain(session,&sc,&eyes[i].swapchain)!=XR_SUCCESS) {
      Tempest::Log::e("xrCreateSwapchain failed");
      shutdown();
      return false;
    }

    uint32_t imgCount = 0;
    xrEnumerateSwapchainImages(eyes[i].swapchain,0,&imgCount,nullptr);
    eyes[i].images.resize(imgCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
    xrEnumerateSwapchainImages(eyes[i].swapchain,imgCount,&imgCount,(XrSwapchainImageBaseHeader*)eyes[i].images.data());

    Tempest::Log::i("Eye ", i, ": ", eyes[i].width, "x", eyes[i].height, " format ", int(colorFormat));
  }

  return true;
}

void OpenXRBackend::shutdown() {
  for(auto& e:eyes) {
    e.color = {};
    if(e.swapchain!=XR_NULL_HANDLE)
      xrDestroySwapchain(e.swapchain);
    e.swapchain = XR_NULL_HANDLE;
  }
  if(space!=XR_NULL_HANDLE) {
    xrDestroySpace(space);
    space = XR_NULL_HANDLE;
  }
  if(session!=XR_NULL_HANDLE) {
    xrDestroySession(session);
    session = XR_NULL_HANDLE;
  }
  if(instance!=XR_NULL_HANDLE) {
    xrDestroyInstance(instance);
    instance = XR_NULL_HANDLE;
  }
}

bool OpenXRBackend::beginFrame() {
  if(session==XR_NULL_HANDLE)
    return false;

  XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};
  if(xrWaitFrame(session,&wi,&frameState)!=XR_SUCCESS)
    return false;
  XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};
  if(xrBeginFrame(session,&bi)!=XR_SUCCESS)
    return false;

  XrViewLocateInfo vi{XR_TYPE_VIEW_LOCATE_INFO};
  vi.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  vi.displayTime = frameState.predictedDisplayTime;
  vi.space       = space;

  uint32_t viewCount = eyes.size();
  std::vector<XrView> xrViews(viewCount, {XR_TYPE_VIEW});
  XrViewState vs{XR_TYPE_VIEW_STATE};
  if(xrLocateViews(session,&vi,&vs,viewCount,&viewCount,xrViews.data())!=XR_SUCCESS)
    return false;

  for(uint32_t i=0;i<viewCount && i<eyes.size();++i) {
    eyes[i].view    = xrViews[i];
    eyes[i].projMat = toTempest(xrMatProjection(xrViews[i].fov, 0.1f, 1000.f));
    eyes[i].viewMat = toTempest(xrMatView(xrViews[i].pose));

    XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    xrAcquireSwapchainImage(eyes[i].swapchain,&ai,&eyes[i].acquired);
    XrSwapchainImageWaitInfo    wi2{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi2.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(eyes[i].swapchain,&wi2);

    auto& img = eyes[i].images[eyes[i].acquired];
    eyes[i].color = Tempest::Attachment(*device, img.image, eyes[i].width, eyes[i].height, toTexFormat(eyes[i].format));
  }

  return true;
}

void OpenXRBackend::endFrame() {
  if(session==XR_NULL_HANDLE)
    return;

  std::array<XrCompositionLayerProjectionView,2> pv{};
  for(size_t i=0;i<eyes.size();++i) {
    pv[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    pv[i].pose = eyes[i].view.pose;
    pv[i].fov  = eyes[i].view.fov;
    pv[i].subImage.swapchain = eyes[i].swapchain;
    pv[i].subImage.imageArrayIndex = 0;
    pv[i].subImage.imageRect.offset = {0,0};
    pv[i].subImage.imageRect.extent = {(int32_t)eyes[i].width,(int32_t)eyes[i].height};

    eyes[i].color = {};
    XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(eyes[i].swapchain,&ri);
  }

  XrCompositionLayerProjection layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  layer.space     = space;
  layer.viewCount = (uint32_t)pv.size();
  layer.views     = pv.data();

  const XrCompositionLayerBaseHeader* layers[] = {
    reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer)
  };

  XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};
  ei.displayTime = frameState.predictedDisplayTime;
  ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  ei.layerCount = 1;
  ei.layers = layers;
  xrEndFrame(session,&ei);
}

std::array<EyeInfo,2> OpenXRBackend::views() const {
  std::array<EyeInfo,2> ret{};
  for(size_t i=0;i<eyes.size();++i) {
    ret[i].view  = eyes[i].viewMat;
    ret[i].proj  = eyes[i].projMat;
    ret[i].color = eyes[i].color;
  }
  return ret;
}

#endif

