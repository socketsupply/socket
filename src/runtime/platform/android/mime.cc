#include "mime.hh"
#include "string_wrap.hh"

namespace ssc::android {
  static MimeTypeMap sharedMimeTypeMap;

  void initializeMimeTypeMap (MimeTypeMapRef ref, JVMEnvironment jvm) {
    sharedMimeTypeMap.ref = ref;
    sharedMimeTypeMap.jvm = jvm;
  }

  const MimeTypeMap* MimeTypeMap::sharedMimeTypeMap () {
    return &Android::sharedMimeTypeMap;
  }

  String MimeTypeMap::getMimeTypeFromExtension (const String& extension) const {
    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto extensionString = attachment.env->NewStringUTF(extension.c_str());
    const auto mimeTypeString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      this->ref,
      "getMimeTypeFromExtension",
      "(Ljava/lang/String;)Ljava/lang/String;",
      extensionString
    );

    const auto mimeType = StringWrap(attachment.env, mimeTypeString).str();
    attachment.env->DeleteLocalRef(extensionString);
    return mimeType;
  }
}
