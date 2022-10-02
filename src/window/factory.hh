#ifndef SSC_WINDOW_FACTORY
#define SSC_WINDOW_FACTORY

#include "../core/runtime-preload.hh"
#include "window.hh"

namespace SSC {
  struct WindowFactoryOptions {
    int defaultHeight = 0;
    int defaultWidth = 0;
    bool headless = false;
    bool isTest;
    std::string argv = "";
    std::string cwd = "";
    Map appData;
    MessageCallback onMessage = [](const std::string) {};
    ExitCallback onExit = nullptr;
  };

  template <class Window> class IWindowFactory {
    public:
      WindowFactoryOptions options;

      virtual void destroy () = 0;
      virtual void configure (WindowFactoryOptions) = 0;
      virtual Window* getWindow (int) = 0;
      virtual void destroyWindow (int) = 0;
      virtual void destroyWindow (Window*) = 0;
      virtual Window* createWindow (WindowOptions) = 0;
      virtual Window* createDefaultWindow (WindowOptions) = 0;
  };

  template <class Window, class App> class WindowFactory : public IWindowFactory<Window> {
    public:
      enum WindowStatus {
        WINDOW_ERROR = -1,
        WINDOW_NONE = 0,
        WINDOW_CREATING = 10,
        WINDOW_CREATED,
        WINDOW_HIDING = 20,
        WINDOW_HIDDEN,
        WINDOW_SHOWING = 30,
        WINDOW_SHOWN,
        WINDOW_CLOSING = 40,
        WINDOW_CLOSED,
        WINDOW_EXITING = 50,
        WINDOW_EXITED,
        WINDOW_KILLING = 60,
        WINDOW_KILLED
      };

      class WindowWithMetadata : public Window {
        public:
          WindowStatus status;
          WindowFactory<Window, App> &factory;

          WindowWithMetadata (
            WindowFactory &factory,
            App &app,
            WindowOptions opts
          ) : Window(app, opts)
            , factory(factory)
          {
            // noop
          }

          ~WindowWithMetadata () {}

          void show (const std::string &seq) {
            auto index = std::to_string(this->opts.index);
            factory.log("Showing Window#" + index + " (seq=" + seq + ")");
            status = WindowStatus::WINDOW_SHOWING;
            Window::show(seq);
            status = WindowStatus::WINDOW_SHOWN;
          }

          void hide (const std::string &seq) {
            if (
              status > WindowStatus::WINDOW_HIDDEN &&
              status < WindowStatus::WINDOW_EXITING
            ) {
              auto index = std::to_string(this->opts.index);
              factory.log("Hiding Window#" + index + " (seq=" + seq + ")");
              status = WindowStatus::WINDOW_HIDING;
              Window::hide(seq);
              status = WindowStatus::WINDOW_HIDDEN;
            }
          }

          void close (int code) {
            if (status < WindowStatus::WINDOW_CLOSING) {
              auto index = std::to_string(this->opts.index);
              factory.log("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_CLOSING;
              Window::close(code);
              status = WindowStatus::WINDOW_CLOSED;
              // gc();
            }
          }

          void exit (int code) {
            if (status < WindowStatus::WINDOW_EXITING) {
              auto index = std::to_string(this->opts.index);
              factory.log("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_EXITING;
              Window::exit(code);
              status = WindowStatus::WINDOW_EXITED;
              gc();
            }
          }

          void kill () {
            if (status < WindowStatus::WINDOW_KILLING) {
              auto index = std::to_string(this->opts.index);
              factory.log("Killing Window#" + index);
              status = WindowStatus::WINDOW_KILLING;
              Window::kill();
              status = WindowStatus::WINDOW_KILLED;
              gc();
            }
          }

          void gc () {
            factory.destroyWindow(reinterpret_cast<Window*>(this));
          }
      };

#if DEBUG
      std::chrono::system_clock::time_point lastDebugLogLine;
#endif

      App &app;
      bool destroyed = false;
      std::vector<bool> inits;
      std::vector<WindowWithMetadata*> windows;
      std::recursive_mutex mutex;

      WindowFactory (App &app) :
        app(app),
        inits(SOCKET_MAX_WINDOWS),
        windows(SOCKET_MAX_WINDOWS)
    {
#if DEBUG
        lastDebugLogLine = std::chrono::system_clock::now();
#endif
      }

      ~WindowFactory () {
        destroy();
      }

      void destroy () {
        if (this->destroyed) return;
        for (auto window : windows) {
          destroyWindow(window);
        }

        this->destroyed = true;

        windows.clear();
        inits.clear();
      }

      void configure (WindowFactoryOptions configuration) {
        if (destroyed) return;
        this->options.defaultHeight = configuration.defaultHeight;
        this->options.defaultWidth = configuration.defaultWidth;
        this->options.onMessage = configuration.onMessage;
        this->options.appData = configuration.appData;
        this->options.onExit = configuration.onExit;
        this->options.headless = configuration.headless;
        this->options.isTest = configuration.isTest;
        this->options.argv = configuration.argv;
        this->options.cwd = configuration.cwd;
      }

      void inline log (const std::string line) {
        if (destroyed) return;
#if DEBUG
        using namespace std::chrono;

#ifdef _WIN32 // unicode console support
        // SetConsoleOutputCP(CP_UTF8);
        // setvbuf(stdout, nullptr, _IOFBF, 1000);
#endif

        auto now = system_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastDebugLogLine).count();

        std::cout << "• " << line;
        std::cout << " \033[0;32m+" << delta << "ms\033[0m";
        std::cout << std::endl;

        lastDebugLogLine = now;
#endif
      }

      Window* getWindow (int index, WindowStatus status) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (this->destroyed) return nullptr;
        if (
          getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
          getWindowStatus(index) < status
        ) {
          return reinterpret_cast<Window*>(windows[index]);
        }

        return nullptr;
      }

      Window* getWindow (int index) {
        return getWindow(index, WindowStatus::WINDOW_EXITING);
      }

      Window* getOrCreateWindow (int index) {
        return getOrCreateWindow(index, WindowOptions {});
      }

      Window* getOrCreateWindow (int index, WindowOptions opts) {
        if (this->destroyed) return nullptr;
        if (index < 0) return nullptr;
        if (getWindowStatus(index) == WindowStatus::WINDOW_NONE) {
          opts.index = index;
          return createWindow(opts);
        }

        return getWindow(index);
      }

      WindowStatus getWindowStatus (int index) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (this->destroyed) return WindowStatus::WINDOW_NONE;
        if (index >= 0 && inits[index]) {
          return windows[index]->status;
        }

        return WindowStatus::WINDOW_NONE;
      }

      void destroyWindow (int index) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return;
        if (index >= 0 && inits[index] && windows[index] != nullptr) {
          return destroyWindow(windows[index]);
        }
      }

      void destroyWindow (WindowWithMetadata* window) {
        if (destroyed) return;
        if (window != nullptr) {
          return destroyWindow(reinterpret_cast<Window*>(window));
        }
      }

      void destroyWindow (Window* window) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return;
        if (window != nullptr && windows[window->index] != nullptr) {
          auto metadata = reinterpret_cast<WindowWithMetadata*>(window);

          inits[window->index] = false;
          windows[window->index] = nullptr;

          if (metadata->status < WINDOW_CLOSING) {
            window->close(0);
          }

          if (metadata->status < WINDOW_KILLING) {
            window->kill();
          }

          delete window;
        }
      }

      Window* createWindow (WindowOptions opts) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return nullptr;
        std::stringstream env;

        if (inits[opts.index]) {
          return reinterpret_cast<Window*>(windows[opts.index]);
        }

        if (opts.appData.size() > 0) {
          for (auto const &envKey : split(opts.appData["env"], ',')) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << std::string(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
            );
          }
        } else {
          for (auto const &envKey : split(this->options.appData["env"], ',')) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << std::string(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
            );
          }
        }

        auto height = opts.height > 0 ? opts.height : this->options.defaultHeight;
        auto width = opts.width > 0 ? opts.width : this->options.defaultWidth;

        WindowOptions windowOptions = {
          .resizable = opts.resizable,
          .frameless = opts.frameless,
          .utility = opts.utility,
          .canExit = opts.canExit,
          .height = height,
          .width = width,
          .index = opts.index,
#if DEBUG
          .debug = DEBUG || opts.debug,
#else
          .debug = opts.debug,
#endif
          .port = opts.port,
          .isTest = this->options.isTest,
          .headless = this->options.headless || opts.headless || opts.appData["headless"] == "true",
          .forwardConsole = opts.appData["linux_forward_console_to_stdout"] == "true",

          .cwd = this->options.cwd,
          .executable = opts.appData["executable"],
          .title = opts.title.size() > 0 ? opts.title : opts.appData["title"],
          .url = opts.url.size() > 0 ? opts.url : "data:text/html,<html>",
          .version = "v" + opts.appData["version"],
          .argv = this->options.argv,
          .preload = opts.preload.size() > 0 ? opts.preload : gPreloadDesktop,
          .env = env.str(),
          .appData = opts.appData.size() > 0 ? opts.appData : this->options.appData
        };

#if DEBUG
        this->log("Creating Window#" + std::to_string(opts.index));
#endif
        auto window = new WindowWithMetadata(*this, app, windowOptions);

        window->status = WindowStatus::WINDOW_CREATED;
        window->onExit = this->options.onExit;
        window->onMessage = this->options.onMessage;

        windows[opts.index] = window;
        inits[opts.index] = true;

        return reinterpret_cast<Window*>(window);
      }

      Window* createDefaultWindow (WindowOptions opts) {
        return createWindow(WindowOptions {
          .resizable = true,
          .frameless = false,
          .canExit = true,
          .height = opts.height,
          .width = opts.width,
          .index = 0,
#ifdef PORT
          .port = PORT,
#endif
          .appData = opts.appData
        });
      }
  };
}
#endif
