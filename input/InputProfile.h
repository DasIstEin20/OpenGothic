#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <vector>

namespace InputMapper {

struct InputEvent;

class InputProfile {
  public:
    bool loadFromJson(const std::string& jsonStr);
    std::optional<std::string> actionFor(const InputEvent& ev) const;

  private:
    struct Key {
      int source;
      int code;
      bool operator==(const Key& other) const noexcept { return source==other.source && code==other.code; }
      };
    struct KeyHash {
      size_t operator()(const Key& k) const noexcept { return (static_cast<size_t>(k.source)<<32) ^ static_cast<size_t>(k.code); }
      };
    struct Analog {
      int   axis;
      float min;
      float max;
      std::string action;
      };
    std::unordered_map<Key,std::string,KeyHash> map_;
    std::vector<Analog> analog_;
  };

}

