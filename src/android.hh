#ifndef SSC_ANDROID_H
#define SSC_ANDROID_H

extern "C" {
  JNIEXPORT jboolean
  JNICALL Java_{{bundle_id}}_fsOpen(JNIEnv *env, jobject this, jstring seq, uint64_t id, String path, int flags, Cb cb);
}

#endif
