#ifndef SOCKET_RUNTIME_CORE_RESOURCE_H
#define SOCKET_RUNTIME_CORE_RESOURCE_H

#include "../platform/platform.hh"

#include "trace.hh"
#include "url.hh"

namespace SSC {
  // forward
  class Core;

  class Resource {
    public:
      Atomic<bool> accessing = false;
      String name;
      String type;
      Tracer tracer;
      Resource (const String& type, const String& name);
      bool hasAccess () const noexcept;
      virtual bool startAccessing () = 0;
      virtual bool stopAccessing () = 0;
  };

  class FileResource : public Resource {
    public:
      struct Cache {
        SharedPointer<char[]> bytes = nullptr;
        size_t size = 0;
      };

      struct WellKnownPaths {
        Path resources;
        Path downloads;
        Path documents;
        Path pictures;
        Path desktop;
        Path videos;
        Path config;
        Path music;
        Path media;
        Path home;
        Path data;
        Path log;
        Path tmp;

        static void setDefaults (const WellKnownPaths& paths);

        WellKnownPaths ();
        const Vector<Path> entries () const;
        JSON::Object json () const;
      };

      class ReadStream {
        public:
        #if SOCKET_RUNTIME_PLATFORM_APPLE
          using Error = NSError;
        #elif SOCKET_RUNTIME_PLATFORM_LINUX
          using Error = GError;
        #else
          using Error = char*;
        #endif

          struct Options {
            Path resourcePath;
            size_t highWaterMark = 64 * 1024;
            size_t size = 0;

            Options (
              const Path& resourcePath = Path(""),
              size_t highWaterMark = 64 * 1024,
              size_t size = 0
            )
              : highWaterMark(highWaterMark),
                resourcePath(resourcePath),
                size(size)
            {}
          };

          struct Buffer {
            Atomic<size_t> size = 0;
            SharedPointer<char[]> bytes = nullptr;
            Buffer (size_t size);
            Buffer (const Options& options);
            Buffer (const Buffer& buffer);
            Buffer (Buffer&& buffer);
            Buffer& operator= (const Buffer&);
            Buffer& operator= (Buffer&&);
            bool isEmpty () const;
          };

        #if SOCKET_RUNTIME_PLATFORM_APPLE
          NSData* data = nullptr;
        #elif SOCKET_RUNTIME_PLATFORM_LINUX
          GFileInputStream* stream = nullptr;
          GFile* file = nullptr;
        #endif

          Options options;
          Error* error = nullptr;
          Atomic<off_t> offset = 0;
          Atomic<bool> ended = false;

          ReadStream (const Options& options);
          ~ReadStream ();
          ReadStream (const ReadStream&);
          ReadStream (ReadStream&&);
          ReadStream& operator= (const ReadStream&);
          ReadStream& operator= (ReadStream&&);

          const Buffer read (off_t offset = -1, size_t highWaterMark = -1);
          size_t remaining (off_t offset = -1) const;
      };

      struct Options {
        bool cache;
        Core* core;
      };

      Cache cache;
      Options options;
      SharedPointer<char[]> bytes = nullptr;

      static std::map<String, Set<String>> mimeTypes;
      static Path getResourcesPath ();
      static Path getResourcePath (const Path& resourcePath);
      static Path getResourcePath (const String& resourcePath);
      static bool isFile (const String& resourcePath);
      static bool isFile (const Path& resourcePath);
      static bool isDirectory (const String& resourcePath);
      static bool isDirectory (const Path& resourcePath);
      static bool isMountedPath (const Path& path);
      static const WellKnownPaths& getWellKnownPaths ();
      static const Map getMountedPaths ();

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      static const Path getMicrosoftEdgeRuntimePath ();
    #endif

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      static void setSharedAndroidAssetManager (Android::AssetManager*);
      static Android::AssetManager* getSharedAndroidAssetManager ();

      static void setExternalAndroidStorageDirectory (const Path&);
      static Path getExternalAndroidStorageDirectory ();

      static void setExternalAndroidFilesDirectory (const Path&);
      static Path getExternalAndroidFilesDirectory ();

      static void setExternalAndroidCacheDirectory (const Path&);
      static Path getExternalAndroidCacheDirectory ();
    #endif

      Path path;
      URL url;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      NSURL* nsURL = nullptr;
    #endif

      FileResource (
        const Path& resourcePath,
        const Options& options = {0}
      );

      FileResource (
        const String& resourcePath,
        const Options& options = {0}
      );

      ~FileResource ();
      FileResource (const FileResource&);
      FileResource (FileResource&&);
      FileResource& operator= (const FileResource&);
      FileResource& operator= (FileResource&&);

      bool startAccessing ();
      bool stopAccessing ();
      bool exists () const noexcept;
      int access (int mode = F_OK) const noexcept;
      const String mimeType () const noexcept;
      size_t size (bool cached = false) noexcept;
      size_t size () const noexcept;
      const char* read () const;
      const char* read (bool cached = false);
      const String str (bool cached = false);
      ReadStream stream (const ReadStream::Options& options = {});

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      bool isAndroidLocalAsset () const noexcept;
      bool isAndroidContent () const noexcept;
    #endif
  };
}
#endif
