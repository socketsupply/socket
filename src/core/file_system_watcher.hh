#ifndef SOCKET_RUNTIME_FILE_SYSTEM_WATCHER_H
#define SOCKET_RUNTIME_FILE_SYSTEM_WATCHER_H

#include "../platform/platform.hh"

namespace SSC {
  class Core;
  class FileSystemWatcher {
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
      using HandleMap = std::map<String, Handle>;
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
        FileSystemWatcher* watcher;
        bool isDirectory = false;
        TimePoint lastUpdated;
      };

      struct Options {
        int debounce = 250; // in milliseconds
        bool recursive = true;
      };

      using EventCallback = std::function<void(
        const String&, // path
        const Vector<Event>&, // events
        const Context& // event context
      )>;

      using ContextMap = std::map<String, Context>;

      // state
      EventCallback callback = nullptr;
      Vector<String> paths;
      Vector<String> watchedPaths;
      ContextMap contexts;
      HandleMap handles;
      Options options;
      AtomicBool ownsCore = false;
      AtomicBool isRunning = false;
      Core* core = nullptr;

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

      FileSystemWatcher (const String& path);
      FileSystemWatcher (const Vector<String>& paths);
      ~FileSystemWatcher ();
      bool start (EventCallback callback);
      bool stop ();
  };
}

#endif
