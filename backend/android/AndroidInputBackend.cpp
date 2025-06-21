#include "AndroidInputBackend.h"

AndroidInputBackend::AndroidInputBackend() = default;
AndroidInputBackend::~AndroidInputBackend() = default;

void AndroidInputBackend::setKeyCallback(KeyCallback cb) {
  keyCb = std::move(cb);
  }

void AndroidInputBackend::setMotionCallback(MotionCallback cb) {
  motionCb = std::move(cb);
  }

void AndroidInputBackend::pollEvents() {
  // TODO: hook into AInputQueue
  }

