#ifndef SOCKET_RUNTIME_ANDROID_STRING_WRAP_H
#define SOCKET_RUNTIME_ANDROID_STRING_WRAP_H

#include "../types.hh"
#include "native.hh"

namespace SSC::Android {
  /**
   * A container for a JNI string (jstring).
   */
  class StringWrap {
    JNIEnv *env = nullptr;
    jstring ref = nullptr;
    const char *string = nullptr;
    size_t length = 0;
    jboolean needsRelease = false;

    public:
      StringWrap (JNIEnv *env);
      StringWrap (const StringWrap& copy);
      StringWrap (StringWrap&& copy);
      StringWrap (JNIEnv *env, jstring ref);
      StringWrap (JNIEnv *env, jobject ref);
      StringWrap (JNIEnv *env, String string);
      StringWrap (JNIEnv *env, const char *string);
      ~StringWrap ();

      void set (String string);
      void set (const char *string);
      void set (jstring ref);
      void release ();

      const String str ();
      const jstring j_str ();
      const char * c_str ();
      const size_t size ();

      const StringWrap& operator= (const StringWrap& string);
      StringWrap& operator= (StringWrap&& string);
  };
}
#endif
