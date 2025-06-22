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
      int code = std::stoi(a.key());
      map_[Key{src,code}] = a->get<std::string>();
    }
  }
  return true;
}

std::optional<std::string> InputProfile::actionFor(int source,int code) const {
  auto it = map_.find(Key{source,code});
  if(it!=map_.end())
    return it->second;
  return std::nullopt;
}

}

