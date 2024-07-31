#ifndef SOCKET_RUNTIME_PLATFORM_ANDROID_CONTENT_RESOLVER_H
#define SOCKET_RUNTIME_PLATFORM_ANDROID_CONTENT_RESOLVER_H

#include "environment.hh"
#include "types.hh"

namespace SSC::Android {
  class ContentResolver {
    public:
      using FileDescriptor = jobject;

      Activity activity;
      JVMEnvironment jvm;
      ContentResolver () = default;

      bool isDocumentURI (const String& uri);
      bool isContentURI (const String& uri);
      bool isExternalStorageDocumentURI (const String& uri);
      bool isDownloadsDocumentURI (const String& uri);
      bool isMediaDocumentURI (const String& uri);
      bool isPhotosURI (const String& uri);

      String getContentMimeType (const String& url);
      String getExternalContentURIForType (const String& type);
      String getPathnameFromURI (const String& uri);
      String getPathnameFromContentURIDataColumn (
        const String& uri,
        const String& id = nullptr
      );

      Vector<String> getPathnameEntriesFromContentURI (const String& uri);

      bool hasAccess (const String& uri);
      FileDescriptor openFileDescriptor (
        const String& uri,
        off_t* offset = nullptr,
        off_t* length = nullptr
      );

      bool closeFileDescriptor (FileDescriptor fileDescriptor);
      size_t getFileDescriptorLength (FileDescriptor fileDescriptor);
      size_t getFileDescriptorOffset (FileDescriptor fileDescriptor);
      int getFileDescriptorFD (FileDescriptor fileDescriptor);
  };
}
#endif
