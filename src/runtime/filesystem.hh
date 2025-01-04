#ifndef SOCKET_RUNTIME_FILESYSTEM_H
#define SOCKET_RUNTIME_FILESYSTEM_H

#include "platform.hh"
#include "resource.hh"
#include "core.hh"

namespace ssc::runtime::filesystem {
  class Resource : public ssc::runtime::Resource {
    public:
      struct Cache {
        SharedPointer<unsigned char[]> bytes = nullptr;
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
            SharedPointer<unsigned char[]> bytes = nullptr;
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
      #if SOCKET_RUNTIME_PLATFORM_ANDROID
        android::ContentResolver* contentResolver;
      #endif
      };

      Cache cache;
      Options options;
      SharedPointer<unsigned char[]> bytes = nullptr;

      static Map<String, Set<String>> mimeTypes;
      static Path getResourcesPath ();
      static Path getResourcePath (const Path& resourcePath);
      static Path getResourcePath (const String& resourcePath);
      static bool isFile (const String& resourcePath);
      static bool isFile (const Path& resourcePath);
      static bool isDirectory (const String& resourcePath);
      static bool isDirectory (const Path& resourcePath);
      static bool isMountedPath (const Path& path);
      static const WellKnownPaths& getWellKnownPaths ();
      static const Map<String, String> getMountedPaths ();

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      static const Path getMicrosoftEdgeRuntimePath ();
    #endif

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      static void setSharedAndroidAssetManager (android::AssetManager*);
      static android::AssetManager* getSharedAndroidAssetManager ();

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

      Resource (
        const Path& resourcePath,
        const Options& options = {0}
      );

      Resource (
        const String& resourcePath,
        const Options& options = {0}
      );

      ~Resource ();
      Resource (const Resource&);
      Resource (Resource&&);
      Resource& operator= (const Resource&);
      Resource& operator= (Resource&&);

      bool startAccessing ();
      bool stopAccessing ();
      bool exists () const noexcept;
      int access (int mode = F_OK) const noexcept;
      const String mimeType () const noexcept;
      size_t size (bool cached = false) noexcept;
      size_t size () const noexcept;
      const unsigned char* read () const;
      const unsigned char* read (bool cached = false);
      const String str (bool cached = false);
      ReadStream stream (const ReadStream::Options& options = {});

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      bool isAndroidLocalAsset () const noexcept;
      bool isAndroidContent () const noexcept;
    #endif
  };

  class Watcher {
    public:
      // uv types
      using EventHandle = uv_fs_event_t;
      using PollHandle = uv_fs_poll_t;
      using Loop = uv_loop_t;
      using Async = uv_async_t;
      using Stat = uv_stat_t;

      struct Handle {
        EventHandle event;
        PollHandle poll;
      };

      // std types
      using Clock = std::chrono::high_resolution_clock;
      using HandleMap = Map<String, Handle>;
      using Path = std::filesystem::path;
      using Thread = std::thread;
      using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

      // event types
      enum class Event {
        UNKNOWN,
        RENAME,
        CHANGE
      };

      // callback context
      struct Context {
        String name; // filename or directory name
        Watcher* watcher;
        bool isDirectory = false;
        TimePoint lastUpdated;
      };

      struct Options {
        int debounce = 250; // in milliseconds
        bool recursive = true;
      };

      using EventCallback = Function<void(
        const String, // path
        const Vector<Event>, // events
        const Context // event context
      )>;

      using ContextMap = Map<String, Context>;

      // state
      EventCallback callback = nullptr;
      Vector<String> paths;
      Vector<String> watchedPaths;
      ContextMap contexts;
      HandleMap handles;
      Options options;
      AtomicBool ownsLoop = false;
      AtomicBool isRunning = false;
      loop::Loop* loop = nullptr;

      static void handleEventCallback (
        EventHandle* handle,
        const char* filename,
        int events,
        int status
      );

      static void handlePollCallback (
        PollHandle* handle,
        int status,
        const Stat* previousStat,
        const Stat* currentStat
      );

      Watcher (const String& path);
      Watcher (const Vector<String>& paths);
      ~Watcher ();
      bool start (EventCallback callback);
      bool stop ();
  };

  const Map<String, int32_t>& constants ();
}
#endif
