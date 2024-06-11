#include "looper.hh"
#include "../../core/debug.hh"

namespace SSC::Android {
  Looper::Looper (JVMEnvironment jvm)
    : jvm(jvm)
  {}

  void Looper::dispatch (const DispatchCallback& callback) {
    while (!this->isReady) {
      msleep(2);
    }

    const auto attachment = Android::JNIEnvironmentAttachment(this->jvm);
    const auto context = new DispatchCallbackContext(callback, this);
    write(this->fds[1], &context, sizeof(DispatchCallbackContext*));
  }

  bool Looper::acquire (const LoopCallback& callback) {
    if (pipe(this->fds) != 0) {
      return false;
    }

    const auto attachment = Android::JNIEnvironmentAttachment(this->jvm);
    this->looper = ALooper_forThread();

    if (this->looper == nullptr) {
      return false;
    }

    ALooper_acquire(this->looper);

    const auto context = std::make_shared<LoopCallbackContext>(callback, this);
    const auto result = ALooper_addFd(
      this->looper,
      this->fds[0],
      ALOOPER_POLL_CALLBACK,
      ALOOPER_EVENT_INPUT,
      [](int fd, int events, void* data) -> int {
        const auto context = reinterpret_cast<LoopCallbackContext*>(data);
        const auto size = sizeof(DispatchCallbackContext*);
        auto buffer = new char[size]{0};
        read(fd, reinterpret_cast<void*>(buffer), size);

        auto dispatch = *((DispatchCallbackContext**) buffer);
        if (dispatch != nullptr) {
          if (dispatch->callback != nullptr) {
            dispatch->callback();
          }

        }

        delete dispatch;
        delete [] buffer;

        if (context != nullptr) {
          if (context->callback != nullptr) {
            context->callback();
          }
        }
        return 1;
      },
      context.get()
    );

    if (result == -1) {
      return false;
    }

    this->context = context;
    this->isReady = true;
    return true;
  }

  void Looper::release () {
    if (this->looper) {
      ALooper_removeFd(this->looper, this->fds[0]);
      ALooper_release(this->looper);
      close(this->fds[0]);
      close(this->fds[1]);
    }
  }

  bool Looper::isAcquired () const {
    return this->looper != nullptr && this->context != nullptr;
  }
}
