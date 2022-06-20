#include "android.hh"

extern "C" {
  jstring package_function (create_hello_world_string)(JNIEnv *env) {
    return env->NewStringUTF("hello world");
  }

  jstring package_export(hello)(JNIEnv *env, jobject self) {
    return package_function(create_hello_world_string)(env);
  }
}
