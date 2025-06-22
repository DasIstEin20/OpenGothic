#pragma once
#include "InputEvent.h"
#include "InputProfile.h"
#include <unordered_map>
#include <memory>
#include <string>

namespace InputMapper {

class InputMapper {
  public:
    bool loadProfile(const std::string& name,const std::string& jsonStr);
    void setActiveProfile(const std::string& name);
    std::optional<std::string> mapEvent(const InputEvent& ev) const;

  private:
    std::unordered_map<std::string,InputProfile> profiles_;
    const InputProfile* active_ = nullptr;
  };

}

