#include "InputMapper.h"
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"InputMapper",__VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) std::fprintf(stderr, __VA_ARGS__)
#endif

namespace InputMapper {

bool InputMapper::loadProfile(const std::string& name,const std::string& jsonStr){
  InputProfile p;
  if(!p.loadFromJson(jsonStr))
    return false;
  profiles_[name] = std::move(p);
  if(!active_)
    active_ = &profiles_[name];
  return true;
}

void InputMapper::setActiveProfile(const std::string& name){
  auto it = profiles_.find(name);
  if(it!=profiles_.end())
    active_ = &it->second;
}

std::optional<std::string> InputMapper::mapEvent(const InputEvent& ev) const {
  if(!active_)
    return std::nullopt;
  return active_->actionFor(static_cast<int>(ev.source), ev.code);
}

}

