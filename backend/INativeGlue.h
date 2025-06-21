#pragma once

class INativeGlue {
  public:
    virtual ~INativeGlue() = default;

    virtual void onStart() = 0;
    virtual void onStop()  = 0;
  };

