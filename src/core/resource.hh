#ifndef SOCKET_RUNTIME_CORE_RESOURCE_H
#define SOCKET_RUNTIME_CORE_RESOURCE_H

#include "../platform/platform.hh"
#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../platform/android.hh"
#endif

namespace SSC {
  class Resource {
    public:
      Atomic<bool> accessing = false;
      Resource () = default;
      virtual bool startAccessing () = 0;
      virtual bool stopAccessing () = 0;
  };

  class FileResource : public Resource {
    public:
    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      static void setSharedAndroidAssetManager (Android::AssetManager*);
      static Android::AssetManager* getSharedAndroidAssetManager ();
    #endif

      static bool isFile (const String& resourcePath);
      static bool isFile (const Path& resourcePath);
      static bool isDirectory (const String& resourcePath);
      static bool isDirectory (const Path& resourcePath);

      struct Cache {
        SharedPointer<char[]> bytes = nullptr;
        size_t size = 0;
      };

      struct Options {
        bool cache;
      };

      Cache cache;
      Options options;
      SharedPointer<char[]> bytes = nullptr;

      static std::map<String, Set<String>> mimeTypes;
      static Path getResourcesPath ();
      static Path getResourcePath (const Path& resourcePath);
      static Path getResourcePath (const String& resourcePath);

      Path path;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      NSURL* url = nullptr;
    #elif SOCKET_RUNTIME_PLATFORM_APPLE
    #endif

      FileResource (const Path& resourcePath, const Options& options = {});
      FileResource (const String& resourcePath, const Options& options = {});
      ~FileResource ();
      FileResource (const FileResource&);
      FileResource (FileResource&&);
      FileResource& operator= (const FileResource&);
      FileResource& operator= (FileResource&&);

      bool startAccessing ();
      bool stopAccessing ();
      bool exists () const noexcept;
      bool hasAccess () const noexcept;
      const String mimeType () const noexcept;
      size_t size (bool cached = false) noexcept;
      size_t size () const noexcept;
      const char* read () const;
      const char* read (bool cached = false);
      const String str (bool cached = false);
  };
}
#endif
