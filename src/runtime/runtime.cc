#include "runtime.hh"

using namespace ssc::runtime::core;
using namespace ssc::runtime::config;

namespace ssc::runtime {
  Runtime::Runtime (const Options& options)
    : windowManager()
      dispatcher(*this),
      options(options),
      services(*this, { dispatcher, options.features })
  {
    this->init();
  }

  Runtime::~Runtime() {
    this->destroy();
  }

  void Runtime::init () {
    this->start();
  }

  bool Runtime::start () {
    if (!this->loop.start()) return false;
    return this->resume();
  }

  bool Runtime::stop () {
    if (!this->pause() || !this->loop.stop()) {
      return false;
    }

    return true;
  }

  bool Runtime::resume () {
    if (!this->loop.resume()) {
      return false;
    }

    if (!this->services.start()) {
      return false;
    }

    return true;
  }

  bool Runtime::pause () {
    if (!this->services.stop()) {
      return false;
    }

  #if !SOCKET_RUNTIME_PLATFORM_ANDROID
    return this->loop.pause();
  #endif

    return true;
  }

  bool Runtime::destroy () {
    if (this->stop() && this->loop.shutdown()) {
      return true;
    }

    return false;
  }

  bool Runtime::dispatch (const DispatchCallback& callback) {
    return this->dispatcher.dispatch(callback);
  }

  bool Runtime::stopped () const {
    return this->loop.stopped();
  }

  bool Runtime::paused () const {
    return this->loop.paused();
  }
}
