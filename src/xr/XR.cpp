#include <xr/XR.h>
#include <xr/IXRBackend.h>

#ifdef OPENXR_ENABLED
#include "OpenXRBackend.h"
#endif

XR::XR() : impl_(nullptr) {
}

XR::~XR() {
  if(impl_) {
    impl_->shutdown();
    delete impl_;
  }
}

bool XR::initialize(Tempest::Device& dev, Tempest::Window& win) {
#ifdef OPENXR_ENABLED
  if(impl_==nullptr) {
    impl_ = new OpenXRBackend();
    if(!impl_->initialize(dev,win)) {
      delete impl_;
      impl_ = nullptr;
    }
  }
#endif
  return impl_!=nullptr;
}

IXRBackend* XR::backend() const {
  return impl_;
}
