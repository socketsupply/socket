#include "file_system_watcher.hh"
#include "core.hh"

namespace SSC {
#if defined(__linux__) && !defined(__ANDROID__)
  static void initializeUVSource (FileSystemWatcher* watcher) {
    watcher->uvSourceFunctions.prepare = [](GSource *source, gint *timeout) -> gboolean {
      auto watcher = reinterpret_cast<FileSystemWatcher::UVSource*>(source)->watcher;

      if (watcher == nullptr) {
        return false;
      }

      auto loop = watcher->loop;

      if (loop == nullptr) {
        return false;
      }

      if (!uv_loop_alive(loop) || !watcher->isRunning) {
        return false;
      }

      *timeout = uv_backend_timeout(loop);

      return *timeout == 0;
    };

    watcher->uvSourceFunctions.dispatch = [](
      GSource *source,
      GSourceFunc callback,
      gpointer user_data
    ) -> gboolean {
      auto watcher = reinterpret_cast<FileSystemWatcher::UVSource*>(source)->watcher;

      if (watcher == nullptr) {
        return false;
      }

      auto loop = watcher->loop;

      if (loop == nullptr) {
        return false;
      }

      uv_run(loop, UV_RUN_NOWAIT);
      return G_SOURCE_CONTINUE;
    };
  }
#endif

  static FileSystemWatcher::Path resolveFileNameForContext (
    const String& filename,
    const FileSystemWatcher::Context* context
  ) {
    if (context->isDirectory) {
      return FileSystemWatcher::Path(context->name) / filename;
    }

    return FileSystemWatcher::Path(filename);
  }

  void FileSystemWatcher::poll (FileSystemWatcher* watcher) {
    Lock lock(watcher->mutex);

    while (watcher->isRunning) {
      do {
        uv_update_time(watcher->loop);
        const auto timeout = uv_backend_timeout(watcher->loop);
        msleep(timeout);
        uv_run(watcher->loop, UV_RUN_NOWAIT);
      } while (watcher->isRunning && uv_loop_alive(watcher->loop));
    }
  }

  void FileSystemWatcher::handleEventCallback (
    Handle* handle,
    const char* eventTarget,
    int eventTypes,
    int eventStatus
  ) {
    const auto filename = String(eventTarget);
    const auto context = reinterpret_cast<Context*>(handle->data);
    const auto path = resolveFileNameForContext(filename, context);
    const auto now = Clock::now();

    if (!std::filesystem::exists(path)) {
      return;
    }

    const auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - context->lastUpdated
    );

    if (delta.count() < context->watcher->options.debounce) {
      return;
    }

    // build events vector
    auto events = Vector<Event>();

    if ((eventTypes & UV_RENAME) == UV_RENAME) {
      events.push_back(Event::RENAME);
    }

    if ((eventTypes & UV_CHANGE) == UV_CHANGE) {
      events.push_back(Event::CHANGE);
    }

    context->lastUpdated = now;
    context->watcher->callback(path.string(), events, *context);
  }

  FileSystemWatcher::FileSystemWatcher (const String& path) {
    this->paths.push_back(path);
  #if defined(__linux__) && !defined(__ANDROID__)
    initializeUVSource(this);
  #endif
  }

  FileSystemWatcher::FileSystemWatcher (const Vector<String>& paths) {
    this->paths = paths;
  #if defined(__linux__) && !defined(__ANDROID__)
    initializeUVSource(this);
  #endif
  }

  FileSystemWatcher::~FileSystemWatcher () {
    this->paths.clear();

    if (this->thread != nullptr) {
      if (this->thread->joinable()) {
        this->thread->join();
      }

      delete this->thread;
      this->thread = nullptr;
    }
  }

  bool FileSystemWatcher::start (EventCallback callback) {
    if (this->isRunning) {
      return false;
    }

    Lock lock(this->mutex);
    this->callback = callback;

    // a loop may be configured for the instance already, perhaps here or
    // manually by the caller
    if (this->loop == nullptr) {
      this->loop = uv_loop_new();
      if (uv_loop_init(this->loop) != 0) {
        return false;
      }

    #if defined(__linux__) && !defined(__ANDROID__)
      GSource *source = g_source_new(&this->uvSourceFunctions, sizeof(UVSource));
      UVSource *uvSource = (UVSource *) source;
      uvSource->watcher = this;
      uvSource->tag = g_source_add_unix_fd(
        source,
        uv_backend_fd(this->loop),
        (GIOCondition) (G_IO_IN | G_IO_OUT | G_IO_ERR)
      );

      g_source_attach(source, nullptr);
    #else
      this->thread = new std::thread(&poll, this);
    #endif
    }

    for (const auto& path : this->paths) {
      const bool exists = this->handles.contains(path);
      auto context = &this->contexts[path];
      auto handle = &this->handles[path];

      // init if not already in handles mapping
      if (!exists) {
        // init context
        context->isDirectory = std::filesystem::is_directory(path);
        context->lastUpdated = Clock::now();
        context->watcher = this;
        context->name = std::filesystem::absolute(path).string();

        // init uv fs event handle
        uv_fs_event_init(this->loop, handle);

        // associcate context
        handle->data = reinterpret_cast<void*>(context);
      }

      uv_fs_event_flags flags;

      if (context->isDirectory) {
        flags = UV_FS_EVENT_RECURSIVE;
      } else {
        flags = UV_FS_EVENT_WATCH_ENTRY;
      }

      // start (or restart)
      uv_fs_event_start(
        handle,
        FileSystemWatcher::handleEventCallback,
        path.c_str(),
        flags
      );
    }

    this->isRunning = true;
    return true;
  }

  bool FileSystemWatcher::stop () {
    Lock lock(this->mutex);
    if (!this->isRunning) {
      return false;
    }

    this->isRunning = false;

    for (auto& handle : this->handles) {
      uv_fs_event_stop(&handle.second);
    }

    // stop loop if `thread` is not a `nullptr` which means we created it
    if (this->loop != nullptr && this->thread != nullptr) {
      uv_stop(this->loop);
    }

    return true;
  }
}
