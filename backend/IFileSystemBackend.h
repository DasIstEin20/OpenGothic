#pragma once
#include <string>
#include <vector>

class IFileSystemBackend {
  public:
    virtual ~IFileSystemBackend() = default;

    virtual std::vector<uint8_t> readAsset(const std::string& path) = 0;
    virtual std::string userDirectory() const = 0;
  };

