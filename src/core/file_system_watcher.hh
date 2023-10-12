#ifndef SSC_FILE_SYSTEM_WATCHER
#define SSC_FILE_SYSTEM_WATCHER

#include "platform.hh"
#include "types.hh"

namespace SSC {
  class FileSystemWatcher {
    public:
      using Loop = uv_loop_t;
      using Path = std::filesystem::path;
      using Thread = std::thread;
      using Handle = uv_fs_event_t;
      using HandleMap = std::map<String, uv_fs_event_t>;
      using Clock = std::chrono::high_resolution_clock;
      using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

      enum class Event {
        UNKNOWN,
        RENAME,
        CHANGE
      };

      struct Context {
        String name; // filename or directory name
        FileSystemWatcher* watcher;
        bool isDirectory = false;
        TimePoint lastUpdated;
      };

      struct Options {
        int debounce = 250; // in milliseconds
      };

      using EventCallback = std::function<void(
        const String&, // path
        const Vector<Event>&, // events
        const Context& // event context
      )>;

      using ContextMap = std::map<String, Context>;

      // state
      EventCallback callback = nullptr;
      ContextMap contexts;
      Vector<String> paths;
      Options options;

      // thread state
      AtomicBool isRunning = false;
      Thread* thread = nullptr;
      Mutex mutex;

      // uv
      HandleMap handles;
      Loop* loop;

      static void poll (FileSystemWatcher*);
      static void handleEventCallback (
        Handle* handle,
        const char* filename,
        int events,
        int status
      );

      FileSystemWatcher (const String& path);
      FileSystemWatcher (const Vector<String>& paths);
      ~FileSystemWatcher ();
      bool start (EventCallback callback);
      bool stop ();
  };
}

#endif
