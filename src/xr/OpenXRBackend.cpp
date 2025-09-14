#ifdef OPENXR_ENABLED
#include "OpenXRBackend.h"

#include <Tempest/Log>
#include <Tempest/Device>
#include <Tempest/Window>
#include <gapi/vulkan/vdevice.h>
#include <gapi/vulkan/vtexture.h>

#include "../game/commandline.h"

#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

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

static Tempest::Vec3 toVec3(const XrVector3f& v) {
  return Tempest::Vec3{v.x,v.y,v.z};
}

static XrVector3f toXr(const Tempest::Vec3& v) {
  return XrVector3f{v.x,v.y,v.z};
}

static Tempest::Vec3 rotate(const XrQuaternionf& q, const Tempest::Vec3& v) {
  const float xx = q.x*q.x;
  const float yy = q.y*q.y;
  const float zz = q.z*q.z;
  const float xy = q.x*q.y;
  const float xz = q.x*q.z;
  const float yz = q.y*q.z;
  const float wx = q.w*q.x;
  const float wy = q.w*q.y;
  const float wz = q.w*q.z;
  return Tempest::Vec3{
      (1.f-2.f*yy-2.f*zz)*v.x + 2.f*(xy-wz)*v.y + 2.f*(xz+wy)*v.z,
      2.f*(xy+wz)*v.x + (1.f-2.f*xx-2.f*zz)*v.y + 2.f*(yz-wx)*v.z,
      2.f*(xz-wy)*v.x + 2.f*(yz+wx)*v.y + (1.f-2.f*xx-2.f*yy)*v.z};
}

static Tempest::Vec3 rotateInv(const XrQuaternionf& q, const Tempest::Vec3& v) {
  return rotate(XrQuaternionf{-q.x,-q.y,-q.z,q.w}, v);
}

static XrPath stringToPath(XrInstance inst, const char* str) {
  XrPath p = XR_NULL_PATH;
  xrStringToPath(inst,str,&p);
  return p;
}

static XrQuaternionf removeRoll(const XrQuaternionf& q) {
  const float ysqr = q.y*q.y;
  float t0 = 2.f*(q.w*q.y + q.x*q.z);
  float t1 = 1.f - 2.f*(ysqr + q.z*q.z);
  float yaw = std::atan2(t0,t1);

  float t2 = 2.f*(q.w*q.x - q.z*q.y);
  t2 = std::clamp(t2,-1.f,1.f);
  float pitch = std::asin(t2);

  float cy = std::cos(yaw*0.5f);
  float sy = std::sin(yaw*0.5f);
  float cp = std::cos(pitch*0.5f);
  float sp = std::sin(pitch*0.5f);

  XrQuaternionf r{};
  r.w = cy*cp;
  r.x = cy*sp;
  r.y = sy*cp;
  r.z = -sy*sp;
  return r;
}

}

OpenXRBackend::OpenXRBackend() = default;
OpenXRBackend::~OpenXRBackend() {
  shutdown();
}

bool OpenXRBackend::createSwapchains() {
  for(auto& e:eyes) {
    if(e.swapchain!=XR_NULL_HANDLE) {
      xrDestroySwapchain(e.swapchain);
      e.swapchain = XR_NULL_HANDLE;
      e.images.clear();
    }
    e.width  = uint32_t(float(e.baseWidth )*renderScale);
    e.height = uint32_t(float(e.baseHeight)*renderScale);

    XrSwapchainCreateInfo sc{XR_TYPE_SWAPCHAIN_CREATE_INFO};
    sc.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
    sc.format      = e.format;
    sc.sampleCount = e.sampleCount;
    sc.width       = e.width;
    sc.height      = e.height;
    sc.faceCount   = 1;
    sc.arraySize   = 1;
    sc.mipCount    = 1;

    if(xrCreateSwapchain(session,&sc,&e.swapchain)!=XR_SUCCESS)
      return false;

    uint32_t imgCount = 0;
    xrEnumerateSwapchainImages(e.swapchain,0,&imgCount,nullptr);
    e.images.resize(imgCount, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
    xrEnumerateSwapchainImages(e.swapchain,imgCount,&imgCount,(XrSwapchainImageBaseHeader*)e.images.data());

    if(logLevel!=LogLevel::Off)
      Tempest::Log::i("Eye ", int(&e-&eyes[0]), ": ", e.width, "x", e.height);
  }
  return true;
}

void OpenXRBackend::destroySwapchains() {
  for(auto& e:eyes) {
    e.color = {};
    if(e.swapchain!=XR_NULL_HANDLE)
      xrDestroySwapchain(e.swapchain);
    e.swapchain = XR_NULL_HANDLE;
    e.images.clear();
  }
}

void OpenXRBackend::pollEvents() {
  XrEventDataBuffer ev{XR_TYPE_EVENT_DATA_BUFFER};
  while(xrPollEvent(instance, &ev)==XR_SUCCESS) {
    if(ev.type==XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
      auto& ssc = *reinterpret_cast<XrEventDataSessionStateChanged*>(&ev);
      switch(ssc.state) {
        case XR_SESSION_STATE_READY: {
          XrSessionBeginInfo bi{XR_TYPE_SESSION_BEGIN_INFO};
          bi.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
          xrBeginSession(session,&bi);
          state = SessionState::Ready;
          running = true;
          break;
        }
        case XR_SESSION_STATE_SYNCHRONIZED:
          state = SessionState::Synchronized;
          visible = false;
          running = true;
          break;
        case XR_SESSION_STATE_VISIBLE:
          state = SessionState::Visible;
          visible = true;
          running = true;
          break;
        case XR_SESSION_STATE_FOCUSED:
          state = SessionState::Focused;
          visible = true;
          running = true;
          break;
        case XR_SESSION_STATE_STOPPING:
          xrEndSession(session);
          state = SessionState::Stopping;
          visible = false;
          running = false;
          break;
        case XR_SESSION_STATE_EXITING:
        case XR_SESSION_STATE_LOSS_PENDING:
          state = SessionState::Exiting;
          visible = false;
          running = false;
          break;
        case XR_SESSION_STATE_IDLE:
          state = SessionState::Idle;
          visible = false;
          running = false;
          break;
        default:
          break;
      }
      if(logLevel==LogLevel::Verbose)
        Tempest::Log::d("XR state ", int(ssc.state));
    }
    ev = {XR_TYPE_EVENT_DATA_BUFFER};
  }
}

bool OpenXRBackend::initialize(Tempest::Device& dev, Tempest::Window& win) {
  (void)win;
  device = &dev;

  auto& cmd = CommandLine::inst();
  firstPerson  = cmd.isVrFirstPerson();
  heightOffset = cmd.vrHeightOffset();
  if(cmd.vrSeated())
    heightOffset -= 0.5f;
  allowRoll     = cmd.vrAllowRoll();
  dominantHand  = (cmd.vrDominantHand()==CommandLine::VrHand::Left ? 0u : 1u);
  renderScale   = cmd.vrRenderScale();
  logLevel      = cmd.vrLog();
  turnDeadzone  = cmd.vrTurnDeadzone();

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

  if(logLevel!=LogLevel::Off) {
    Tempest::Log::i("VR render scale: ", renderScale);
    Tempest::Log::i("VR vignette strength: ", cmd.vrVignetteStrength());
    Tempest::Log::i("VR dominant hand: ", dominantHand==1?"right":"left");
  }

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
  if(xrCreateReferenceSpace(session,&rs,&refSpace)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateReferenceSpace failed");
    shutdown();
    return false;
  }

  XrReferenceSpaceCreateInfo hs{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  hs.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  hs.poseInReferenceSpace.orientation.w = 1.f;
  if(xrCreateReferenceSpace(session,&hs,&headSpace)!=XR_SUCCESS) {
    Tempest::Log::e("xrCreateReferenceSpace VIEW failed");
    shutdown();
    return false;
  }

  Tempest::Log::i("Reference space: LOCAL");
  Tempest::Log::i("Head space: VIEW");

  // action set
  xrStringToPath(instance,"/user/hand/left", &handPath[0]);
  xrStringToPath(instance,"/user/hand/right",&handPath[1]);

  XrActionSetCreateInfo asci{XR_TYPE_ACTION_SET_CREATE_INFO};
  std::strcpy(asci.actionSetName,"gameplay");
  std::strcpy(asci.localizedActionSetName,"Gameplay");
  asci.priority = 0;
  xrCreateActionSet(instance,&asci,&actionSet);

  auto makeAction = [&](XrAction& dst, XrActionType type, const char* name, const char* lname, XrPath* sub=nullptr, uint32_t cnt=0) {
    XrActionCreateInfo ac{XR_TYPE_ACTION_CREATE_INFO};
    ac.actionSet = actionSet;
    ac.actionType = type;
    std::strncpy(ac.actionName,name,XR_MAX_ACTION_NAME_SIZE);
    std::strncpy(ac.localizedActionName,lname,XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
    ac.countSubactionPaths = cnt;
    ac.subactionPaths = sub;
    xrCreateAction(actionSet,&ac,&dst);
  };

  makeAction(moveAction, XR_ACTION_TYPE_VECTOR2F_INPUT, "move", "Move", &handPath[0],1);
  makeAction(turnAction, XR_ACTION_TYPE_VECTOR2F_INPUT, "turn", "Turn", &handPath[1],1);
  makeAction(jumpAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "jump", "Jump");
  makeAction(attackAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "attack", "Attack");
  makeAction(interactAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "interact", "Interact");
  makeAction(menuAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "menu", "Menu");
  makeAction(teleportAction, XR_ACTION_TYPE_BOOLEAN_INPUT, "teleport_click", "Teleport");
  makeAction(aimPoseAction, XR_ACTION_TYPE_POSE_INPUT, "aim_pose", "Aim Pose", handPath.data(),uint32_t(handPath.size()));
  makeAction(hapticAction, XR_ACTION_TYPE_VIBRATION_OUTPUT, "haptic", "Haptic", handPath.data(),uint32_t(handPath.size()));

  auto path = [&](const char* s) { return stringToPath(instance,s); };
  std::vector<XrActionSuggestedBinding> oculus = {
    {moveAction,      path("/user/hand/left/input/thumbstick")},
    {turnAction,      path("/user/hand/right/input/thumbstick")},
    {jumpAction,      path("/user/hand/left/input/x/click")},
    {interactAction,  path("/user/hand/right/input/a/click")},
    {menuAction,      path("/user/hand/right/input/b/click")},
    {attackAction,    path("/user/hand/right/input/trigger/click")},
    {teleportAction,  path("/user/hand/left/input/trigger/click")},
    {aimPoseAction,   path("/user/hand/right/input/aim/pose")},
    {aimPoseAction,   path("/user/hand/left/input/aim/pose")},
    {hapticAction,    path("/user/hand/right/output/haptic")},
    {hapticAction,    path("/user/hand/left/output/haptic")}
  };
  XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  suggested.interactionProfile = path("/interaction_profiles/oculus/touch_controller");
  suggested.countSuggestedBindings = (uint32_t)oculus.size();
  suggested.suggestedBindings = oculus.data();
  xrSuggestInteractionProfileBindings(instance,&suggested);

  XrPath wmrProfile = stringToPath(instance,"/interaction_profiles/microsoft/motion_controller");
  if(wmrProfile!=XR_NULL_PATH) {
    std::vector<XrActionSuggestedBinding> wmr = {
      {moveAction,      path("/user/hand/left/input/thumbstick")},
      {turnAction,      path("/user/hand/right/input/thumbstick")},
      {jumpAction,      path("/user/hand/left/input/squeeze/click")},
      {interactAction,  path("/user/hand/right/input/trigger/click")},
      {menuAction,      path("/user/hand/right/input/menu/click")},
      {attackAction,    path("/user/hand/right/input/trigger/click")},
      {teleportAction,  path("/user/hand/left/input/trigger/click")},
      {aimPoseAction,   path("/user/hand/right/input/aim/pose")},
      {aimPoseAction,   path("/user/hand/left/input/aim/pose")},
      {hapticAction,    path("/user/hand/right/output/haptic")},
      {hapticAction,    path("/user/hand/left/output/haptic")}
    };
    XrInteractionProfileSuggestedBinding sb{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    sb.interactionProfile = wmrProfile;
    sb.countSuggestedBindings = (uint32_t)wmr.size();
    sb.suggestedBindings = wmr.data();
    xrSuggestInteractionProfileBindings(instance,&sb);
  }

  XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  attach.countActionSets = 1;
  attach.actionSets      = &actionSet;
  xrAttachSessionActionSets(session,&attach);

  for(size_t i=0;i<2;++i) {
    XrActionSpaceCreateInfo asci2{XR_TYPE_ACTION_SPACE_CREATE_INFO};
    asci2.action = aimPoseAction;
    asci2.subactionPath = handPath[i];
    asci2.poseInActionSpace.orientation.w = 1.f;
    xrCreateActionSpace(session,&asci2,&aimSpace[i]);
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
    eyes[i].baseWidth   = cfg[i].recommendedImageRectWidth;
    eyes[i].baseHeight  = cfg[i].recommendedImageRectHeight;
    eyes[i].sampleCount = cfg[i].recommendedSwapchainSampleCount;
    eyes[i].format      = colorFormat;
  }

  if(!createSwapchains()) {
    shutdown();
    return false;
  }

  return true;
}

void OpenXRBackend::shutdown() {
  destroySwapchains();
  if(uiSwapchain!=XR_NULL_HANDLE) {
    xrDestroySwapchain(uiSwapchain);
    uiSwapchain = XR_NULL_HANDLE;
  }
  uiImages.clear();
  haveUi = false;
  for(auto& sp:aimSpace)
    if(sp!=XR_NULL_HANDLE) {
      xrDestroySpace(sp);
      sp = XR_NULL_HANDLE;
    }
  if(actionSet!=XR_NULL_HANDLE) {
    xrDestroyActionSet(actionSet);
    actionSet = XR_NULL_HANDLE;
  }
  if(headSpace!=XR_NULL_HANDLE) {
    xrDestroySpace(headSpace);
    headSpace = XR_NULL_HANDLE;
  }
  if(refSpace!=XR_NULL_HANDLE) {
    xrDestroySpace(refSpace);
    refSpace = XR_NULL_HANDLE;
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

  pollEvents();
  if(!running || !visible) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    return false;
  }

  XrFrameWaitInfo wi{XR_TYPE_FRAME_WAIT_INFO};
  if(xrWaitFrame(session,&wi,&frameState)!=XR_SUCCESS)
    return false;
  XrFrameBeginInfo bi{XR_TYPE_FRAME_BEGIN_INFO};
  if(xrBeginFrame(session,&bi)!=XR_SUCCESS)
    return false;

  if(lastPredicted!=0) {
    dt = double(frameState.predictedDisplayTime - lastPredicted) / 1e9;
    dt = std::clamp(dt, 1.0/144.0, 1.0/30.0);
    if(!loggedFps && logLevel!=LogLevel::Off) {
      Tempest::Log::i("XR refresh rate: ", int(std::round(1.0/dt)), " Hz");
      loggedFps = true;
    }
  }
  lastPredicted = frameState.predictedDisplayTime;

  XrSpaceLocation headLoc{XR_TYPE_SPACE_LOCATION};
  if(xrLocateSpace(headSpace, refSpace, frameState.predictedDisplayTime, &headLoc)==XR_SUCCESS) {
    const XrSpaceLocationFlags req = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if((headLoc.locationFlags & req)==req) {
      headPose = headLoc.pose;
      hasPose  = true;
    }
  }

  XrViewLocateInfo vi{XR_TYPE_VIEW_LOCATE_INFO};
  vi.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
  vi.displayTime = frameState.predictedDisplayTime;
  vi.space       = refSpace;

  uint32_t viewCount = eyes.size();
  std::vector<XrView> xrViews(viewCount, {XR_TYPE_VIEW});
  XrViewState vs{XR_TYPE_VIEW_STATE};
  if(xrLocateViews(session,&vi,&vs,viewCount,&viewCount,xrViews.data())!=XR_SUCCESS)
    return false;

  XrQuaternionf headQ = headPose.orientation;
  if(hasPose && !allowRoll)
    headQ = removeRoll(headPose.orientation);
  Tempest::Vec3 headPos = toVec3(headPose.position);
  headPos.y += heightOffset;

  for(uint32_t i=0;i<viewCount && i<eyes.size();++i) {
    XrPosef pose = xrViews[i].pose;
    if(hasPose) {
      Tempest::Vec3 eyePos   = toVec3(xrViews[i].pose.position);
      Tempest::Vec3 offset   = eyePos - toVec3(headPose.position);
      Tempest::Vec3 offLocal = rotateInv(headPose.orientation, offset);
      Tempest::Vec3 pos      = rotate(headQ, offLocal) + headPos;
      pose.orientation = headQ;
      pose.position    = toXr(pos);
    } else {
      pose.position.y += heightOffset;
    }

    eyes[i].view.pose = pose;
    eyes[i].view.fov  = xrViews[i].fov;
    eyes[i].projMat   = toTempest(xrMatProjection(xrViews[i].fov, 0.1f, 1000.f));
    eyes[i].viewMat   = toTempest(xrMatView(pose));

    XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    xrAcquireSwapchainImage(eyes[i].swapchain,&ai,&eyes[i].acquired);
    XrSwapchainImageWaitInfo    wi2{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi2.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(eyes[i].swapchain,&wi2);

    auto& img = eyes[i].images[eyes[i].acquired];
    eyes[i].color = Tempest::Attachment(*device, img.image, eyes[i].width, eyes[i].height, toTexFormat(eyes[i].format));
  }

  if(firstPerson && !loggedFp) {
    Tempest::Log::d("VR first-person camera: height ", heightOffset, " allow roll ", allowRoll);
    loggedFp = true;
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
  layer.space     = refSpace;
  layer.viewCount = (uint32_t)pv.size();
  layer.views     = pv.data();

  const XrCompositionLayerBaseHeader* layers[2];
  uint32_t layerCount = 0;
  layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&layer);

  XrCompositionLayerQuad quadLayer{XR_TYPE_COMPOSITION_LAYER_QUAD};
  if(haveUi) {
    if(uiSwapchain==XR_NULL_HANDLE || uiW!=uiReq.width || uiH!=uiReq.height) {
      if(uiSwapchain!=XR_NULL_HANDLE)
        xrDestroySwapchain(uiSwapchain);
      XrSwapchainCreateInfo sci{XR_TYPE_SWAPCHAIN_CREATE_INFO};
      sci.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
      sci.format     = uiFormat;
      sci.sampleCount= 1;
      sci.width      = uint32_t(uiReq.width);
      sci.height     = uint32_t(uiReq.height);
      sci.arraySize  = 1;
      sci.mipCount   = 1;
      xrCreateSwapchain(session,&sci,&uiSwapchain);
      uint32_t imgCount=0;
      xrEnumerateSwapchainImages(uiSwapchain,0,&imgCount,nullptr);
      uiImages.resize(imgCount,{XR_TYPE_SWAPCHAIN_IMAGE_VULKAN2_KHR});
      xrEnumerateSwapchainImages(uiSwapchain,imgCount,&imgCount,
                                 reinterpret_cast<XrSwapchainImageBaseHeader*>(uiImages.data()));
      uiW = sci.width;
      uiH = sci.height;
    }

    XrSwapchainImageAcquireInfo ai{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    uint32_t acquired = 0;
    xrAcquireSwapchainImage(uiSwapchain,&ai,&acquired);
    XrSwapchainImageWaitInfo wi2{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    wi2.timeout = XR_INFINITE_DURATION;
    xrWaitSwapchainImage(uiSwapchain,&wi2);

    Tempest::Attachment src(*device, uiReq.image, uiReq.width, uiReq.height, toTexFormat(uiFormat));
    VkImage dstImg = uiImages[acquired].image;
    Tempest::Attachment dst(*device, dstImg, uiReq.width, uiReq.height, toTexFormat(uiFormat));

    struct DevAccess { Tempest::AbstractGraphicsApi& api; struct Impl { Tempest::AbstractGraphicsApi& api; Tempest::AbstractGraphicsApi::Device* dev; } impl; Tempest::AbstractGraphicsApi::Device* dev; };
    auto& dx = *reinterpret_cast<Tempest::Detail::VDevice*>(reinterpret_cast<DevAccess*>(device)->dev);
    auto cmd = dx.dataMgr().get();
    cmd->begin(true);
    auto& sTex = *reinterpret_cast<Tempest::Detail::VTexture*>(Tempest::textureCast<Tempest::Texture2d&>(src).impl.handler);
    auto& dTex = *reinterpret_cast<Tempest::Detail::VTexture*>(Tempest::textureCast<Tempest::Texture2d&>(dst).impl.handler);
    cmd->barrier(sTex, Tempest::ResourceAccess::ColorAttach, Tempest::ResourceAccess::TransferSrc, uint32_t(-1));
    cmd->barrier(dTex, Tempest::ResourceAccess::None, Tempest::ResourceAccess::TransferDst, uint32_t(-1));
    cmd->blit(sTex, uiReq.width, uiReq.height, 0, dTex, uiReq.width, uiReq.height, 0);
    cmd->barrier(sTex, Tempest::ResourceAccess::TransferSrc, Tempest::ResourceAccess::Sampler, uint32_t(-1));
    cmd->barrier(dTex, Tempest::ResourceAccess::TransferDst, Tempest::ResourceAccess::ColorAttach, uint32_t(-1));
    cmd->end();
    dx.dataMgr().submitAndWait(std::move(cmd));

    quadLayer.space = refSpace;
    quadLayer.pose  = uiReq.pose;
    quadLayer.size  = {uiReq.metersWidth, uiReq.metersWidth * float(uiReq.height)/float(uiReq.width)};
    quadLayer.subImage.swapchain = uiSwapchain;
    quadLayer.subImage.imageRect.offset = {0,0};
    quadLayer.subImage.imageRect.extent = {uiW, uiH};
    layers[layerCount++] = reinterpret_cast<const XrCompositionLayerBaseHeader*>(&quadLayer);
  }

  XrFrameEndInfo ei{XR_TYPE_FRAME_END_INFO};
  ei.displayTime = frameState.predictedDisplayTime;
  ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  ei.layerCount = layerCount;
  ei.layers = layers;
  xrEndFrame(session,&ei);
  if(haveUi) {
    XrSwapchainImageReleaseInfo ri{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    xrReleaseSwapchainImage(uiSwapchain,&ri);
    haveUi = false;
  }
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

void OpenXRBackend::setUiQuad(const XRQuadLayerDesc* q) {
  if(q) {
    uiReq  = *q;
    haveUi = true;
  } else {
    haveUi = false;
  }
}

Tempest::Vec3 OpenXRBackend::headPosition() const {
  return toVec3(headPose.position);
}

Tempest::Vec4 OpenXRBackend::headOrientation() const {
  return Tempest::Vec4{headPose.orientation.x, headPose.orientation.y, headPose.orientation.z, headPose.orientation.w};
}

void OpenXRBackend::pollInput() {
  input = {};
  if(session==XR_NULL_HANDLE || actionSet==XR_NULL_HANDLE)
    return;

  XrActiveActionSet act{actionSet, XR_NULL_PATH};
  XrActionsSyncInfo sync{XR_TYPE_ACTIONS_SYNC_INFO};
  sync.countActiveActionSets = 1;
  sync.activeActionSets      = &act;
  if(xrSyncActions(session,&sync)!=XR_SUCCESS)
    return;

  if(!profileQueried) {
    XrInteractionProfileState prof{XR_TYPE_INTERACTION_PROFILE_STATE};
    if(xrGetCurrentInteractionProfile(session, handPath[dominantHand], &prof)==XR_SUCCESS && prof.interactionProfile!=XR_NULL_PATH) {
      char path[XR_MAX_PATH_LENGTH] = {};
      uint32_t sz = 0;
      xrPathToString(instance, prof.interactionProfile, XR_MAX_PATH_LENGTH, &sz, path);
      if(std::strstr(path, "simple_controller")!=nullptr)
        simpleController = true;
    }
    profileQueried = true;
    if(simpleController && turnDeadzone<=0.25f)
      turnDeadzone = 0.3f;
  }

  XrActionStateGetInfo gi{XR_TYPE_ACTION_STATE_GET_INFO};
  XrActionStateVector2f v2{XR_TYPE_ACTION_STATE_VECTOR2F};
  gi.action = moveAction;
  xrGetActionStateVector2f(session,&gi,&v2);
  if(v2.isActive) {
    input.haveControllers = true;
    input.move = {v2.currentState.x, v2.currentState.y};
  }

  gi.action = turnAction;
  xrGetActionStateVector2f(session,&gi,&v2);
  if(v2.isActive) {
    input.haveControllers = true;
    float x = v2.currentState.x;
    if(std::fabs(x) < turnDeadzone)
      x = 0.f;
    input.turnX = x;
  }

  XrActionStateBoolean b{XR_TYPE_ACTION_STATE_BOOLEAN};
  gi.action = jumpAction;
  xrGetActionStateBoolean(session,&gi,&b);
  input.jump = b.currentState;

  gi.action = attackAction;
  xrGetActionStateBoolean(session,&gi,&b);
  input.attack = b.currentState;

  gi.action = interactAction;
  xrGetActionStateBoolean(session,&gi,&b);
  input.interact = b.currentState;

  gi.action = menuAction;
  xrGetActionStateBoolean(session,&gi,&b);
  input.menu = b.currentState;

  gi.action = teleportAction;
  xrGetActionStateBoolean(session,&gi,&b);
  input.teleportClick = b.currentState;

  size_t handIdx = dominantHand;
  for(int attempt=0; attempt<2; ++attempt) {
    if(aimSpace[handIdx]==XR_NULL_HANDLE) {
      handIdx = 1 - handIdx;
      continue;
    }
    XrSpaceLocation loc{XR_TYPE_SPACE_LOCATION};
    if(xrLocateSpace(aimSpace[handIdx], refSpace, frameState.predictedDisplayTime, &loc)==XR_SUCCESS) {
      const XrSpaceLocationFlags req = XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
      if((loc.locationFlags & req)==req) {
        input.aim.valid = true;
        input.aim.pos   = toVec3(loc.pose.position);
        input.aim.dir   = rotate(loc.pose.orientation, Tempest::Vec3{0,0,-1});
        break;
      }
    }
    handIdx = 1 - handIdx;
  }
}

void OpenXRBackend::hapticPulse(float amplitude, float seconds) {
  if(session==XR_NULL_HANDLE || hapticAction==XR_NULL_HANDLE)
    return;
  XrHapticVibration vib{XR_TYPE_HAPTIC_VIBRATION};
  vib.amplitude = amplitude;
  vib.duration  = XrDuration(uint64_t(seconds*1e9));
  vib.frequency = XR_FREQUENCY_UNSPECIFIED;
  XrHapticActionInfo hi{XR_TYPE_HAPTIC_ACTION_INFO};
  hi.action = hapticAction;
  hi.subactionPath = handPath[dominantHand];
  xrApplyHapticFeedback(session,&hi,(XrHapticBaseHeader*)&vib);
}

bool OpenXRBackend::isVisible() const {
  return visible;
}

bool OpenXRBackend::isRunning() const {
  return running;
}

void OpenXRBackend::recenter() {
  if(session==XR_NULL_HANDLE)
    return;
  XrPosef p{};
  p.orientation.w = 1.f;
  xrResetReferenceSpace(session, XR_REFERENCE_SPACE_TYPE_LOCAL, &p);
}

void OpenXRBackend::setRenderScale(float s) {
  if(s<=0.f || std::fabs(s-renderScale)<1e-3f)
    return;
  renderScale = s;
  createSwapchains();
}

#endif

