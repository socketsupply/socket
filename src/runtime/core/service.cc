#include "../core.hh"

namespace ssc::runtime::core {
  Service::~Service () noexcept {}

  Service::Service (const Options& options)
    : dispatcher(options.dispatcher),
      services(options.services),
      context(options.context),
      enabled(options.enabled),
      loop(options.loop)
  {}

  bool Service::dispatch (const context::Dispatcher::Callback callback) {
    return this->dispatcher.dispatch(callback);
  }

  bool Service::start () {
    return this->enabled.load();
  }

  bool Service::stop () {
    return this->enabled.load();
  }

  context::RuntimeContext* Service::getRuntimeContext () {
    return &this->context;
  }
}
