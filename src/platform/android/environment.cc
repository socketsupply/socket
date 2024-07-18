#include "environment.hh"
#include "../../core/debug.hh"

namespace SSC::Android {
  JVMEnvironment::JVMEnvironment (JNIEnv* env) {
    this->jniVersion = env->GetVersion();
    env->GetJavaVM(&jvm);
  }

  int JVMEnvironment::version () const {
    return this->jniVersion;
  }

  JavaVM* JVMEnvironment::get () const {
    return this->jvm;
  }

  JNIEnvironmentAttachment::JNIEnvironmentAttachment (JavaVM *jvm, int version) {
    this->attach(jvm, version);
  }

  JNIEnvironmentAttachment::JNIEnvironmentAttachment (const JVMEnvironment& jvm)
    : JNIEnvironmentAttachment(jvm.get(), jvm.version())
  {}

  JNIEnvironmentAttachment::JNIEnvironmentAttachment (JNIEnvironmentAttachment&& attachment) {
    this->jvm = attachment.jvm;
    this->env = attachment.env;
    this->status = attachment.status;
    this->version = attachment.version;
    this->attached = attachment.attached.load();

    attachment.jvm = nullptr;
    attachment.env = nullptr;
    attachment.status = 0;
    attachment.version = 0;
    attachment.attached = false;
  }

  JNIEnvironmentAttachment::~JNIEnvironmentAttachment () {
    this->detach();
  }

  JNIEnvironmentAttachment&
  JNIEnvironmentAttachment::operator= (JNIEnvironmentAttachment&& attachment) {
    this->jvm = attachment.jvm;
    this->env = attachment.env;
    this->status = attachment.status;
    this->version = attachment.version;
    this->attached = attachment.attached.load();

    attachment.jvm = nullptr;
    attachment.env = nullptr;
    attachment.status = 0;
    attachment.version = 0;
    attachment.attached = false;
    return *this;
  }

  void JNIEnvironmentAttachment::attach (JavaVM *jvm, int version) {
    this->jvm = jvm;
    this->version = version;

    if (jvm != nullptr) {
      this->status = this->jvm->GetEnv((void **) &this->env, this->version);

      if (this->status == JNI_EDETACHED) {
        this->attached = this->jvm->AttachCurrentThread(&this->env, 0);
      }

      // debug("env: %p", this->env);
    }
  }

  void JNIEnvironmentAttachment::detach () {
    const auto jvm = this->jvm;
    const auto attached = this->attached.load();

    if (this->hasException()) {
      this->printException();
    }

    this->env = nullptr;
    this->jvm = nullptr;
    this->status = 0;
    this->attached = false;

    if (attached && jvm != nullptr) {
      jvm->DetachCurrentThread();
    }
  }

  bool JNIEnvironmentAttachment::hasException () const {
    return this->env != nullptr && this->env->ExceptionCheck();
  }

  void JNIEnvironmentAttachment::printException () const {
    if (this->env != nullptr) {
      this->env->ExceptionDescribe();
    }
  }
}
