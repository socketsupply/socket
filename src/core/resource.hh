#ifndef SOCKET_RUNTIME_CORE_RESOURCE_H
#define SOCKET_RUNTIME_CORE_RESOURCE_H

#include "../platform/platform.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../platform/android.hh"
#endif

#include "trace.hh"

namespace SSC {
  class Resource {
    public:
      Atomic<bool> accessing = false;
      const String name;
      const String type;
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

      struct Options {
        bool cache;
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
        Path home;
        Path data;
        Path log;
        Path tmp;

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
            SharedPointer<char[]> bytes;
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
        #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
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
      static const WellKnownPaths& getWellKnownPaths ();

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      static void setSharedAndroidAssetManager (Android::AssetManager*);
      static Android::AssetManager* getSharedAndroidAssetManager ();
    #endif

      Path path;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      NSURL* url = nullptr;
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
      const String mimeType () const noexcept;
      size_t size (bool cached = false) noexcept;
      size_t size () const noexcept;
      const char* read () const;
      const char* read (bool cached = false);
      const String str (bool cached = false);
      ReadStream stream (const ReadStream::Options& options = {});
  };
}
#endif
