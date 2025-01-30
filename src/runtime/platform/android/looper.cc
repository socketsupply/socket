#include "../../platform.hh"
#include "../../debug.hh"

#include "looper.hh"

namespace ssc::runtime::android {
  Looper::Looper (JVMEnvironment jvm)
    : jvm(jvm)
  {}

  Looper::Looper (const Looper& looper)
    : jvm(looper.jvm),
      isUnique(false),
      isReady(looper.isReady.load()),
      looper(looper.looper),
      context(looper.context)
  {
    this->fds[0] = looper.fds[0];
    this->fds[1] = looper.fds[1];
  }

  Looper::Looper (Looper&& looper)
    : jvm(std::move(looper.jvm)),
      isUnique(looper.isUnique.load()),
      isReady(looper.isReady.load()),
      looper(std::move(looper.looper)),
      context(std::move(looper.context))
  {
    this->fds[0] = looper.fds[0];
    this->fds[1] = looper.fds[1];
    looper.jvm = nullptr;
    looper.isUnique = false;
    looper.isReady = false;
    looper.context = nullptr;
    looper.looper = nullptr;
    looper.fds[0] = 0;
    looper.fds[1] = 0;
  }

  Looper::~Looper () {
    this->release();
  }

  Looper Looper::operator= (const Looper& looper) {
    this->jvm = std::move(looper.jvm);
    this->isUnique = looper.isReady ? false : true;
    this->isReady = looper.isReady.load();
    this->looper = std::move(looper.looper);
    this->context = std::move(looper.context);
    this->fds[0] = looper.fds[0];
    this->fds[1] = looper.fds[1];
    return *this;
  }

  Looper Looper::operator= (Looper&& looper) {
    this->jvm = std::move(looper.jvm);
    this->isUnique = looper.isUnique.load();
    this->isReady = looper.isReady.load();
    this->looper = std::move(looper.looper);
    this->context = std::move(looper.context);
    this->fds[0] = looper.fds[0];
    this->fds[1] = looper.fds[1];
    looper.jvm = nullptr;
    looper.isUnique = false;
    looper.isReady = false;
    looper.context = nullptr;
    looper.looper = nullptr;
    looper.fds[0] = 0;
    looper.fds[1] = 0;
    return *this;
  }

  void Looper::dispatch (const DispatchCallback callback) {
    if (!this->acquired()) {
      this->acquire();
    }
    while (!this->isReady) {
      msleep(2);
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto context = new DispatchCallbackContext(callback, this);
    write(this->fds[1], &context, sizeof(DispatchCallbackContext*));
  }

  bool Looper::acquire (const LoopCallback callback) {
    if (pipe(this->fds) != 0) {
      return false;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
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
        auto buffer = std::make_shared<char[]>(size);
        read(fd, reinterpret_cast<void*>(buffer.get()), size);

        auto dispatch = *((DispatchCallbackContext**) buffer.get());
        if (dispatch != nullptr) {
          if (dispatch->callback != nullptr) {
            dispatch->callback();
          }
        }

        delete dispatch;

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
    if (this->looper && this->isUnique) {
      if (this->fds[0] != 0) {
        ALooper_removeFd(this->looper, this->fds[0]);
      }

      ALooper_release(this->looper);
      this->looper = nullptr;

      if (this->fds[0] != 0) {
        close(this->fds[0]);
      }

      if (this->fds[1] != 0) {
        close(this->fds[1]);
      }

      this->fds[0] = 0;
      this->fds[1] = 0;
    }

    if (this->context) {
      this->context = nullptr;
    }
  }

  bool Looper::acquired () const {
    return this->looper != nullptr && this->context != nullptr;
  }
}
