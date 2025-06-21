#include "AndroidNativeGlue.h"

AndroidNativeGlue::AndroidNativeGlue(ANativeActivity* act)
  : activity(act) {}

AndroidNativeGlue::~AndroidNativeGlue() = default;

void AndroidNativeGlue::onStart() {
  // placeholder for lifecycle
  }

void AndroidNativeGlue::onStop() {
  }

