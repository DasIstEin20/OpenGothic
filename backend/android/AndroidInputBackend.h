#pragma once
#include "../IInputBackend.h"

class AndroidInputBackend : public IInputBackend {
  public:
    AndroidInputBackend();
    ~AndroidInputBackend() override;

    void setKeyCallback(KeyCallback cb) override;
    void setMotionCallback(MotionCallback cb) override;
    void pollEvents() override;

  private:
    KeyCallback    keyCb;
    MotionCallback motionCb;
  };

