#include "../../filesystem.hh"
#include "../../string.hh"
#include "../../url.hh"

#include "content_resolver.hh"

using ssc::runtime::string::split;

namespace ssc::runtime::android {
  ContentResolver::ContentResolver (const ContentResolver& resolver) {
    this->activity = resolver.activity;
    this->jvm = resolver.jvm;
  }

  ContentResolver::ContentResolver (ContentResolver&& resolver) {
    this->activity = resolver.activity;
    this->jvm = resolver.jvm;
    resolver.jvm = nullptr;
    resolver.activity = nullptr;
  }

  ContentResolver& ContentResolver::operator = (const ContentResolver& resolver) {
    this->activity = resolver.activity;
    this->jvm = resolver.jvm;
    return *this;
  }

  ContentResolver& ContentResolver::operator = (ContentResolver&& resolver) {
    this->activity = resolver.activity;
    this->jvm = resolver.jvm;
    resolver.jvm = nullptr;
    resolver.activity = nullptr;
    return *this;
  }

  bool ContentResolver::isDocumentURI (const String& uri) {
    if (this->jvm && this->activity) {
      const auto attachment = JNIEnvironmentAttachment(this->jvm);
      const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->activity,
        "getAppPlatform",
        "()Lsocket/runtime/app/AppPlatform;"
      );

      return CallClassMethodFromAndroidEnvironment(
        attachment.env,
        Boolean,
        platform,
        "isDocumentURI",
        "(Ljava/lang/String;)Z",
        attachment.env->NewStringUTF(uri.c_str())
      );
    }

    return false;
  }

  bool ContentResolver::isContentURI (const String& uri) {
    const auto url = URL(uri);
    return url.scheme == "content" || url.scheme == "android.resource";
  }

  bool ContentResolver::isExternalStorageDocumentURI (const String& uri) {
    const auto url = URL(uri);
    return url.hostname == "com.android.externalstorage.documents";
  }

  bool ContentResolver::isDownloadsDocumentURI (const String& uri) {
    const auto url = URL(uri);
    return url.hostname == "com.android.providers.downloads.documents";
  }

  bool ContentResolver::isMediaDocumentURI (const String& uri) {
    const auto url = URL(uri);
    return url.hostname == "com.android.providers.media.documents";
  }

  bool ContentResolver::isPhotosURI (const String& uri) {
    const auto url = URL(uri);
    return url.hostname == "com.android.providers.media.documents";
  }

  String ContentResolver::getContentMimeType (const String& uri) {
    if (this->jvm && this->activity) {
      const auto attachment = JNIEnvironmentAttachment(this->jvm);
      const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->activity,
        "getAppPlatform",
        "()Lsocket/runtime/app/AppPlatform;"
      );

      const auto contentMimeTypeString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        platform,
        "getContentMimeType",
        "(Ljava/lang/String;)Ljava/lang/String;",
        attachment.env->NewStringUTF(uri.c_str())
      );

      return StringWrap(attachment.env, contentMimeTypeString).str();
    }

    return "";
  }

  String ContentResolver::getPathnameFromURI (const String& uri) {
    const auto url = URL(uri);

    if (url.scheme == "file") {
      return url.pathname;
    }

    if (this->jvm && this->isDocumentURI(uri)) {
      const auto attachment = JNIEnvironmentAttachment(this->jvm);
      const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->activity,
        "getAppPlatform",
        "()Lsocket/runtime/app/AppPlatform;"
      );

      const auto documentIDString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        platform,
        "getDocumentID",
        "(Ljava/lang/String;)Ljava/lang/String;",
        attachment.env->NewStringUTF(uri.c_str())
      );

      const auto documentID = StringWrap(attachment.env, documentIDString).str();

      if (this->isExternalStorageDocumentURI(uri)) {
        const auto externalStorage = filesystem::Resource::getExternalAndroidStorageDirectory();
        const auto parts = split(documentID, ":");
        const auto type = parts[0];

        if (type == "primary" && parts.size() > 1) {
          return externalStorage.string() + "/" + parts[1];
        }
      }

      if (this->isDownloadsDocumentURI(uri)) {
        const auto contentURIString = (jstring) CallObjectClassMethodFromAndroidEnvironment(
          attachment.env,
          platform,
          "getContentURI",
          "(Ljava/lang/String;J)Ljava/lang/String;",
          attachment.env->NewStringUTF("content://downloads/public_downloads"),
          std::stol(documentID)
        );

        const auto contentURI = StringWrap(attachment.env, contentURIString).str();

        return this->getPathnameFromContentURIDataColumn(contentURI);
      }

      if (this->isMediaDocumentURI(uri)) {
        const auto parts = split(documentID, ":");
        const auto type = parts[0];

        if (parts.size() > 1) {
          const auto id = parts[1];
          const auto contentURI = this->getExternalContentURIForType(type);

          if (contentURI.size() > 0) {
            return this->getPathnameFromContentURIDataColumn(contentURI, id);
          }
        }
      }
    }

    if (url.scheme == "content") {
      if (this->isPhotosURI(uri)) {
        const auto parts = split(url.pathname, '/');
        if (parts.size() > 0) {
          return parts[parts.size() - 1];
        }
      }

      return this->getPathnameFromContentURIDataColumn(uri);
    }


    return "";
  }

  String ContentResolver::getPathnameFromContentURIDataColumn (
    const String& uri,
    const String& id
  ) {
    if (this->jvm && this->activity) {
      const auto attachment = JNIEnvironmentAttachment(this->jvm);
      const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->activity,
        "getAppPlatform",
        "()Lsocket/runtime/app/AppPlatform;"
      );

      const auto result = (jstring) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        platform,
        "getPathnameFromContentURIDataColumn",
        "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;",
        attachment.env->NewStringUTF(uri.c_str()),
        attachment.env->NewStringUTF(id.c_str())
      );

      return StringWrap(attachment.env, result).str();
    }

    return "";
  }

  String ContentResolver::getExternalContentURIForType (const String& type) {
    if (this->jvm && this->activity) {
      const auto attachment = JNIEnvironmentAttachment(this->jvm);
      const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        this->activity,
        "getAppPlatform",
        "()Lsocket/runtime/app/AppPlatform;"
      );

      const auto result = (jstring) CallObjectClassMethodFromAndroidEnvironment(
        attachment.env,
        platform,
        "getExternalContentURIForType",
        "(Ljava/lang/String;)Ljava/lang/String;",
        attachment.env->NewStringUTF(type.c_str())
      );

      return StringWrap(attachment.env, result).str();
    }

    return "";
  }

  Vector<String> ContentResolver::getPathnameEntriesFromContentURI (
    const String& uri
  ) {
    Vector<String> entries;

    if (!this->jvm || !this->activity) {
      return entries;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      this->activity,
      "getAppPlatform",
      "()Lsocket/runtime/app/AppPlatform;"
    );

    const auto results = (jobjectArray) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      platform,
      "getPathnameEntriesFromContentURI",
      "(Ljava/lang/String;)[Ljava/lang/String;",
      attachment.env->NewStringUTF(uri.c_str())
    );

    const auto length = attachment.env->GetArrayLength(results);

    for (int i = 0; i < length; ++i) {
      const auto result = (jstring) attachment.env->GetObjectArrayElement(results, i);
      const auto pathname = attachment.env->GetStringUTFChars(result, nullptr);
      if (pathname != nullptr) {
        entries.push_back(pathname);
        attachment.env->ReleaseStringUTFChars(result, pathname);
      }
    }

    return entries;
  }

  bool ContentResolver::hasAccess (const String& uri) {
    if (!this->jvm || !this->activity || -!uri.starts_with("content:") & !uri.starts_with("android.resource:")) {
      return false;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      this->activity,
      "getAppPlatform",
      "()Lsocket/runtime/app/AppPlatform;"
    );

    return CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      platform,
      "hasContentResolverAccess",
      "(Ljava/lang/String;)Z",
      attachment.env->NewStringUTF(uri.c_str())
    );
  }

  ContentResolver::FileDescriptor ContentResolver::openFileDescriptor (
    const String& uri,
    off_t* offset,
    off_t* length
  ) {
    if (!this->jvm || !this->activity || !uri.starts_with("content:") & !uri.starts_with("android.resource:")) {
      return nullptr;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto platform = (jobject) CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      this->activity,
      "getAppPlatform",
      "()Lsocket/runtime/app/AppPlatform;"
    );

    const auto fileDescriptor = CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      platform,
      "openContentResolverFileDescriptor",
      "(Ljava/lang/String;)Landroid/content/res/AssetFileDescriptor;",
      attachment.env->NewStringUTF(uri.c_str())
    );

    if (fileDescriptor != nullptr) {
      if (offset != nullptr) {
        *offset = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Long,
          fileDescriptor,
          "getStartOffset",
          "()J"
        );
      }

      if (length != nullptr) {
        *length = CallClassMethodFromAndroidEnvironment(
          attachment.env,
          Long,
          fileDescriptor,
          "getLength",
          "()J"
        );
      }

      return attachment.env->NewGlobalRef(fileDescriptor);
    }

    return nullptr;
  }

  bool ContentResolver::closeFileDescriptor (ContentResolver::FileDescriptor fileDescriptor) {
    if (!this->jvm || !this->activity || fileDescriptor == nullptr) {
      return false;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      fileDescriptor,
      "close",
      "()V"
    );

    attachment.env->DeleteGlobalRef(fileDescriptor);

    return true;
  }

  size_t ContentResolver::getFileDescriptorLength (FileDescriptor fileDescriptor) {
    if (!this->jvm || !this->activity || fileDescriptor == nullptr) {
      return -EINVAL;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    return CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Long,
      fileDescriptor,
      "getLength",
      "()J"
    );
  }

  size_t ContentResolver::getFileDescriptorOffset (FileDescriptor fileDescriptor) {
    if (!this->jvm || !this->activity || fileDescriptor == nullptr) {
      return -EINVAL;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    return CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Long,
      fileDescriptor,
      "getOffset",
      "()J"
    );
  }

  int ContentResolver::getFileDescriptorFD (FileDescriptor fileDescriptor) {
    if (!this->jvm || !this->activity || fileDescriptor == nullptr) {
      return -EINVAL;
    }

    const auto attachment = JNIEnvironmentAttachment(this->jvm);
    const auto parcel = CallObjectClassMethodFromAndroidEnvironment(
      attachment.env,
      fileDescriptor,
      "getParcelFileDescriptor",
      "()Landroid/os/ParcelFileDescriptor;"
    );

    return CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Int,
      parcel,
      "getFd",
      "()I"
    );
  }
}
