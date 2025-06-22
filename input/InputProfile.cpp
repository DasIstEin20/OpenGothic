#include "InputProfile.h"
#include "InputEvent.h"
#include <nlohmann/json.hpp>
#ifdef __ANDROID__
#include <android/log.h>
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"InputMapper",__VA_ARGS__)
#else
#include <cstdio>
#define LOGI(...) std::fprintf(stderr, __VA_ARGS__)
#endif

using json = nlohmann::json;

namespace InputMapper {

bool InputProfile::loadFromJson(const std::string& jsonStr) {
  map_.clear();
  analog_.clear();
  json j;
  try {
    j = json::parse(jsonStr);
  } catch(const std::exception& e) {
    LOGI("Failed to parse profile: %s", e.what());
    return false;
  }
  for(auto it=j.begin(); it!=j.end(); ++it) {
    int src = std::stoi(it.key());
    if(!it->is_object())
      continue;
    for(auto a=it->begin(); a!=it->end(); ++a) {
      const std::string key = a.key();
      if(key.rfind("axis:",0)==0){
        // format axis:<id>:<min>:<max>
        auto pos1 = key.find(':',5);
        auto pos2 = key.find(':',pos1+1);
        if(pos1!=std::string::npos && pos2!=std::string::npos){
          int axis = std::stoi(key.substr(5,pos1-5));
          float v1 = std::stof(key.substr(pos1+1,pos2-pos1-1));
          float v2 = std::stof(key.substr(pos2+1));
          analog_.push_back({axis,v1,v2,a->get<std::string>()});
        }
      } else {
        int code = std::stoi(key);
        map_[Key{src,code}] = a->get<std::string>();
      }
    }
  }
  return true;
}

std::optional<std::string> InputProfile::actionFor(const InputEvent& ev) const {
  if(ev.type==EventType::KEY){
    auto it = map_.find(Key{static_cast<int>(ev.source),ev.code});
    if(it!=map_.end())
      return it->second;
  }
  if(ev.type==EventType::MOTION){
    for(const auto& a:analog_){
      if(a.axis==ev.code && ev.value>=a.min && ev.value<=a.max)
        return a.action;
    }
  }
  return std::nullopt;
}

}

