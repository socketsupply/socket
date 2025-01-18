#include "../concurrent.hh"

namespace ssc::runtime::concurrent {
  AbortSignal::AbortSignal (const AbortController* controller)
    : controller(controller)
  {}

  AbortSignal::AbortSignal (const AbortSignal& signal)
    : controller(signal.controller)
  {}

  AbortSignal::AbortSignal (AbortSignal&& signal)
    : controller(signal.controller)
  {
    signal.controller = nullptr;
  }

  AbortSignal& AbortSignal::operator = (const AbortSignal& signal) {
    this->controller = signal.controller;
    return *this;
  }

  AbortSignal& AbortSignal::operator = (AbortSignal&& signal) {
    this->controller = signal.controller;
    signal.controller = nullptr;
    return *this;
  }

  bool AbortSignal::aborted () const {
    if (this->controller == nullptr) {
      return false;
    }
    return this->controller->isAborted.load(std::memory_order_acquire);
  }

  void AbortController::abort () {
    this->isAborted = true;
  }
}
