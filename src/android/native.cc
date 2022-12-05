#include "native.hh"
#include <uv.h>

// clang-format off
//
#pragma NativeString

NativeString::NativeString (JNIEnv *env) {
  this->env = env;
  this->ref = nullptr;
  this->length = 0;
  this->string = nullptr;
  this->needsRelease = false;
}

NativeString::NativeString (const NativeString &copy)
  : env(copy.env)
  , ref(copy.ref)
  , length(copy.length)
  , string(copy.string)
  , needsRelease(false)
{
  // noop copy constructor
}

NativeString::NativeString (JNIEnv *env, jstring ref)
  : NativeString(env)
{
  if (ref) {
    this->set(ref);
  }
}

NativeString::NativeString (JNIEnv *env, SSC::String string)
  : NativeString(env)
{
  if (string.size() > 0) {
    this->set(string);
  }
}

NativeString::NativeString (JNIEnv *env, const char *string)
  : NativeString(env)
{
  if (string) {
    this->set(string);
  }
}

NativeString::~NativeString () {
  this->release();
}

void NativeString::set (SSC::String string) {
  this->set(string.c_str());
}

void NativeString::set (const char *string) {
  this->set(this->env->NewStringUTF(string));
}

void NativeString::set (jstring ref) {
  if (ref) {
    this->ref = ref;
    this->string = this->env->GetStringUTFChars(ref, &this->needsRelease);
    this->length = this->env->GetStringUTFLength(this->ref);
  }
}

void NativeString::release () {
  if (this->ref && this->string && this->needsRelease) {
    this->env->ReleaseStringUTFChars(this->ref, this->string);
  }

  this->ref = nullptr;
  this->length = 0;
  this->string = nullptr;
  this->needsRelease = false;
}

const char * NativeString::c_str () {
  return this->string;
}

const SSC::String NativeString::str () {
  SSC::String value;

  if (this->string) {
    value.assign(this->string);
  }

  return value;
}

const jstring NativeString::jstr () {
  return this->ref;
}

const size_t NativeString::size () {
  if (!this->string || !this->ref) {
    return 0;
  }

  return this->length;
}
