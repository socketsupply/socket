#include "internal.hh"

// clang-format off
//
#pragma StringWrap

namespace SSC::android {
  StringWrap::StringWrap (JNIEnv *env) {
    this->env = env;
    this->ref = nullptr;
    this->length = 0;
    this->string = nullptr;
    this->needsRelease = false;
  }

  StringWrap::StringWrap (const StringWrap &copy)
    : env(copy.env),
      ref(copy.ref),
      length(copy.length),
      string(copy.string),
      needsRelease(false)
    {}

  StringWrap::StringWrap (JNIEnv *env, jstring ref) : StringWrap(env) {
    if (ref) {
      this->set(ref);
    }
  }

  StringWrap::StringWrap (JNIEnv *env, SSC::String string) : StringWrap(env) {
    if (string.size() > 0) {
      this->set(string);
    }
  }

  StringWrap::StringWrap (JNIEnv *env, const char *string) : StringWrap(env) {
    if (string) {
      this->set(string);
    }
  }

  StringWrap::~StringWrap () {
    this->release();
  }

  void StringWrap::set (SSC::String string) {
    this->set(string.c_str());
  }

  void StringWrap::set (const char *string) {
    this->set(this->env->NewStringUTF(string));
  }

  void StringWrap::set (jstring ref) {
    if (ref) {
      this->ref = ref;
      this->string = this->env->GetStringUTFChars(ref, &this->needsRelease);
      this->length = this->env->GetStringUTFLength(this->ref);
    }
  }

  void StringWrap::release () {
    if (this->ref && this->string && this->needsRelease) {
      this->env->ReleaseStringUTFChars(this->ref, this->string);
    }

    this->ref = nullptr;
    this->length = 0;
    this->string = nullptr;
    this->needsRelease = false;
  }

  const char * StringWrap::c_str () {
    return this->string;
  }

  const SSC::String StringWrap::str () {
    SSC::String value;

    if (this->string) {
      value.assign(this->string);
    }

    return value;
  }

  const jstring StringWrap::j_str () {
    return this->ref;
  }

  const size_t StringWrap::size () {
    if (!this->string || !this->ref) {
      return 0;
    }

    return this->length;
  }
}
