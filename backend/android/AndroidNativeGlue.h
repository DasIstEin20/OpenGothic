#pragma once
#include "../INativeGlue.h"
#include <android/native_activity.h>

class AndroidNativeGlue : public INativeGlue {
  public:
    explicit AndroidNativeGlue(ANativeActivity* act);
    ~AndroidNativeGlue() override;

    void onStart() override;
    void onStop() override;

  private:
    ANativeActivity* activity = nullptr;
  };

