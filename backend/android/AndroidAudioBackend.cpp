#include "AndroidAudioBackend.h"

AndroidAudioBackend::AndroidAudioBackend() = default;
AndroidAudioBackend::~AndroidAudioBackend() = default;

bool AndroidAudioBackend::init() {
  // TODO: initialize OpenSL/AAudio
  return true;
  }

void AndroidAudioBackend::shutdown() {
  }

void AndroidAudioBackend::playSample(const void* ,size_t ,int ,int ) {
  }

