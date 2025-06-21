#pragma once
#include "../IFileSystemBackend.h"

struct AAssetManager;

class AndroidFileSystemBackend : public IFileSystemBackend {
  public:
    explicit AndroidFileSystemBackend(AAssetManager* mgr);
    ~AndroidFileSystemBackend() override;

    std::vector<uint8_t> readAsset(const std::string& path) override;
    std::string userDirectory() const override;

  private:
    AAssetManager* mgr = nullptr;
  };

