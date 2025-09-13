#ifdef OPENXR_ENABLED
#include "OpenXRBackend.h"
#include <Tempest/Log>
#include <cstring>

OpenXRBackend::OpenXRBackend() = default;
OpenXRBackend::~OpenXRBackend() {
  shutdown();
}

bool OpenXRBackend::initialize(Tempest::Device& dev, Tempest::Window& win) {
  (void)dev;
  (void)win;
  XrInstanceCreateInfo ici{XR_TYPE_INSTANCE_CREATE_INFO};
  std::strcpy(ici.applicationInfo.applicationName, "OpenGothic");
  std::strcpy(ici.applicationInfo.engineName, "OpenGothic");
  ici.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  if(xrCreateInstance(&ici,&instance)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateInstance failed");
    return false;
  }

  XrInstanceProperties ip{XR_TYPE_INSTANCE_PROPERTIES};
  if(xrGetInstanceProperties(instance,&ip)==XR_SUCCESS) {
    Tempest::Log::i("OpenXR runtime: ", ip.runtimeName);
  }

  XrSystemGetInfo sgi{XR_TYPE_SYSTEM_GET_INFO};
  sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  if(xrGetSystem(instance,&sgi,&systemId)!=XR_SUCCESS) {
    Tempest::Log::e("xrGetSystem failed");
    xrDestroyInstance(instance);
    instance = XR_NULL_HANDLE;
    return false;
  }

  XrSessionCreateInfo sci{XR_TYPE_SESSION_CREATE_INFO};
  sci.systemId = systemId;
  sci.next = nullptr; // no graphics binding for now
  if(xrCreateSession(instance,&sci,&session)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateSession failed");
    xrDestroyInstance(instance);
    instance = XR_NULL_HANDLE;
    return false;
  }

  XrReferenceSpaceCreateInfo rs{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  rs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  rs.poseInReferenceSpace.orientation.w = 1.f;
  if(xrCreateReferenceSpace(session,&rs,&space)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateReferenceSpace failed");
    xrDestroySession(session);
    xrDestroyInstance(instance);
    session = XR_NULL_HANDLE;
    instance = XR_NULL_HANDLE;
    return false;
  }

  Tempest::Log::i("View configuration: PRIMARY_STEREO");
  return true;
}

void OpenXRBackend::shutdown() {
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
  return xrBeginFrame(session,&bi)==XR_SUCCESS;
}

void OpenXRBackend::endFrame() {
  if(session==XR_NULL_HANDLE)
    return;
  XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};
  ei.displayTime = frameState.predictedDisplayTime;
  ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  ei.layerCount = 0;
  ei.layers = nullptr;
  xrEndFrame(session,&ei);
}

std::array<EyeInfo,2> OpenXRBackend::views() const {
  return {};
}
#endif
