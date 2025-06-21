#include "AndroidFileSystemBackend.h"
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

AndroidFileSystemBackend::AndroidFileSystemBackend(AAssetManager* m)
  : mgr(m) {}

AndroidFileSystemBackend::~AndroidFileSystemBackend() = default;

std::vector<uint8_t> AndroidFileSystemBackend::readAsset(const std::string& path) {
  std::vector<uint8_t> out;
  if(!mgr)
    return out;
  AAsset* a = AAssetManager_open(mgr, path.c_str(), AASSET_MODE_BUFFER);
  if(a){
    size_t sz = AAsset_getLength(a);
    out.resize(sz);
    AAsset_read(a,out.data(),sz);
    AAsset_close(a);
    }
  return out;
  }

std::string AndroidFileSystemBackend::userDirectory() const {
  return "/sdcard/REGoth"; // placeholder
  }

