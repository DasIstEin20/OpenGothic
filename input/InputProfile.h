#pragma once
#include <string>
#include <unordered_map>
#include <optional>

namespace InputMapper {

struct InputEvent;

class InputProfile {
  public:
    bool loadFromJson(const std::string& jsonStr);
    std::optional<std::string> actionFor(int source,int code) const;

  private:
    struct Key {
      int source;
      int code;
      bool operator==(const Key& other) const noexcept { return source==other.source && code==other.code; }
      };
    struct KeyHash {
      size_t operator()(const Key& k) const noexcept { return (static_cast<size_t>(k.source)<<32) ^ static_cast<size_t>(k.code); }
      };
    std::unordered_map<Key,std::string,KeyHash> map_;
  };

}

