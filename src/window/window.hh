#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#include "../ipc/ipc.hh"
#include "../app/app.hh"
#include "options.hh"

#ifndef SSC_MAX_WINDOWS
#define SSC_MAX_WINDOWS 32
#endif

#if defined(__APPLE__)
#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@interface SSCBridgedWebView : WKWebView
@end
#else
@interface SSCBridgedWebView : WKWebView<
  WKUIDelegate,
  NSDraggingDestination,
  NSFilePromiseProviderDelegate,
  NSDraggingSource
>
-   (NSDragOperation) draggingSession: (NSDraggingSession *) session
sourceOperationMaskForDraggingContext: (NSDraggingContext) context;
@end
#endif

@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
-                  (void) webview: (SSCBridgedWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end
#endif

namespace SSC {
#if defined(_WIN32)
  class DragDrop;
#endif

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  class Window {
    public:
      App& app;
      WindowOptions opts;

      MessageCallback onMessage = [](const String) {};
      ExitCallback onExit = nullptr;
      IPC::Bridge *bridge = nullptr;
      int index = 0;
      int width = 0;
      int height = 0;
      fs::path modulePath;

#if defined(__APPLE__)
#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSWindow* window;
#endif
      SSCBridgedWebView* webview;
#elif defined(__linux__) && !defined(__ANDROID__)
      GtkSelectionData *selectionData = nullptr;
      GtkAccelGroup *accelGroup = nullptr;
      GtkWidget *webview = nullptr;
      GtkWidget *window = nullptr;
      GtkWidget *menubar = nullptr;
      GtkWidget *vbox = nullptr;
      GtkWidget *popup = nullptr;
      std::vector<String> draggablePayload;
      double dragLastX = 0;
      double dragLastY = 0;
      bool isDragInvokedInsideWindow;
      int popupId;
#elif defined(_WIN32)
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      ICoreWebView2Controller *controller = nullptr;
      ICoreWebView2 *webview = nullptr;
      HMENU systemMenu;
      DWORD mainThread = GetCurrentThreadId();
      POINT m_minsz = POINT {0, 0};
      POINT m_maxsz = POINT {0, 0};
      DragDrop* drop;
      HWND window;
      std::map<int, std::string> menuMap;
      void resize (HWND window);
#endif

      Window (App&, WindowOptions);

      static ScreenSize getScreenSize ();

      void about ();
      void eval (const String&);
      void show ();
      void hide ();
      void kill ();
      void exit (int code);
      void close (int code);
      void navigate (const String&, const String&);
      String getTitle ();
      void setTitle (const String&);
      ScreenSize getSize ();
      void setSize (int, int, int);
      void setContextMenu (const String&, const String&);
      void closeContextMenu (const String&);
      void closeContextMenu ();
#if defined(__linux__) && !defined(__ANDROID__)
      void closeContextMenu (GtkWidget *, const String&);
#endif
      void setBackgroundColor (int r, int g, int b, float a);
      void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos);
      void setSystemMenu (const String& seq, const String& menu);
      void showInspector ();
      int openExternal (const String& s);
      void openDialog ( // @TODO(jwerle): use `OpenDialogOptions` here instead
        const String&,
        bool,
        bool,
        bool,
        bool,
        const String&,
        const String&,
        const String&
      );

      void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      ) {
        if (seq.find("R") == 0) {
          this->eval(getResolveToRenderProcessJavaScript(seq, state, value));
        }

        this->onMessage(IPC::getResolveToMainProcessMessage(seq, state, value));
      }

      static float getSizeInPixels (String sizeInPercent, int screenSize) {
        if (sizeInPercent.size() > 0) {
          if (sizeInPercent.back() == '%') {
            sizeInPercent.pop_back();
            return screenSize * std::stof(sizeInPercent) / 100;
          }
          return std::stof(sizeInPercent);
        }
        return 0;
      }
  };

  struct WindowManagerOptions {
    String defaultHeight = "0";
    String defaultWidth = "0";
    String defaultMinWidth = "0";
    String defaultMinHeight = "0";
    String defaultMaxWidth = "100%";
    String defaultMaxHeight = "100%";
    bool headless = false;
    bool isTest;
    String argv = "";
    String cwd = "";
    Map appData;
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
  };

  struct WindowPropertiesFlags {
    bool showTitle = false;
    bool showSize = false;
    bool showStatus = false;
  };

  class WindowManager  {
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

      class ManagedWindow : public Window {
        public:
          WindowStatus status;
          WindowManager &manager;

          ManagedWindow (
            WindowManager &manager,
            App &app,
            WindowOptions opts
          ) : Window(app, opts) , manager(manager) { }

          ~ManagedWindow () {}

          void show (const String &seq) {
            auto index = std::to_string(this->opts.index);
            manager.log("Showing Window#" + index + " (seq=" + seq + ")");
            status = WindowStatus::WINDOW_SHOWING;
            Window::show();
            status = WindowStatus::WINDOW_SHOWN;
          }

          void hide (const String &seq) {
            if (
              status > WindowStatus::WINDOW_HIDDEN &&
              status < WindowStatus::WINDOW_EXITING
            ) {
              auto index = std::to_string(this->opts.index);
              manager.log("Hiding Window#" + index + " (seq=" + seq + ")");
              status = WindowStatus::WINDOW_HIDING;
              Window::hide();
              status = WindowStatus::WINDOW_HIDDEN;
            }
          }

          void close (int code) {
            if (status < WindowStatus::WINDOW_CLOSING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Closing Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_CLOSING;
              Window::close(code);
              status = WindowStatus::WINDOW_CLOSED;
            }
          }

          void exit (int code) {
            if (status < WindowStatus::WINDOW_EXITING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Exiting Window#" + index + " (code=" + std::to_string(code) + ")");
              status = WindowStatus::WINDOW_EXITING;
              Window::exit(code);
              status = WindowStatus::WINDOW_EXITED;
              gc();
            }
          }

          void kill () {
            if (status < WindowStatus::WINDOW_KILLING) {
              auto index = std::to_string(this->opts.index);
              manager.log("Killing Window#" + index);
              status = WindowStatus::WINDOW_KILLING;
              Window::kill();
              status = WindowStatus::WINDOW_KILLED;
              gc();
            }
          }

          void gc () {
            manager.destroyWindow(reinterpret_cast<Window*>(this));
          }

          JSON::Object json () {
            auto index = this->opts.index;
            auto size = this->getSize();

            return JSON::Object::Entries {
              { "index", index },
              { "title", this->getTitle() },
              { "width", size.width },
              { "height", size.height },
              { "status", this->status }
            };
          }
      };

      std::chrono::system_clock::time_point lastDebugLogLine;

      App &app;
      bool destroyed = false;
      std::vector<bool> inits;
      std::vector<ManagedWindow*> windows;
      std::recursive_mutex mutex;
      WindowManagerOptions options;

      WindowManager (App &app) :
        app(app),
        inits(SSC_MAX_WINDOWS),
        windows(SSC_MAX_WINDOWS)
    {
      if (isDebugEnabled()) {
        lastDebugLogLine = std::chrono::system_clock::now();
      }
    }

      ~WindowManager () {
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

      void configure (WindowManagerOptions configuration) {
        if (destroyed) return;
        this->options.defaultHeight = configuration.defaultHeight;
        this->options.defaultWidth = configuration.defaultWidth;
        this->options.defaultMinWidth = configuration.defaultMinWidth;
        this->options.defaultMinHeight = configuration.defaultMinHeight;
        this->options.defaultMaxWidth = configuration.defaultMaxWidth;
        this->options.defaultMaxHeight = configuration.defaultMaxHeight;
        this->options.onMessage = configuration.onMessage;
        this->options.appData = configuration.appData;
        this->options.onExit = configuration.onExit;
        this->options.headless = configuration.headless;
        this->options.isTest = configuration.isTest;
        this->options.argv = configuration.argv;
        this->options.cwd = configuration.cwd;
      }

      void inline log (const String line) {
        if (destroyed || !isDebugEnabled()) return;
        using namespace std::chrono;

        auto now = system_clock::now();
        auto delta = duration_cast<milliseconds>(now - lastDebugLogLine).count();

        std::cout << "• " << line;
        std::cout << " \033[0;32m+" << delta << "ms\033[0m";
        std::cout << std::endl;

        lastDebugLogLine = now;
      }

      ManagedWindow* getWindow (int index, WindowStatus status) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (this->destroyed) return nullptr;
        if (
          getWindowStatus(index) > WindowStatus::WINDOW_NONE &&
          getWindowStatus(index) < status
        ) {
          return windows[index];
        }

        return nullptr;
      }

      ManagedWindow* getWindow (int index) {
        return getWindow(index, WindowStatus::WINDOW_EXITING);
      }

      ManagedWindow* getOrCreateWindow (int index) {
        return getOrCreateWindow(index, WindowOptions {});
      }

      ManagedWindow* getOrCreateWindow (int index, WindowOptions opts) {
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

      void destroyWindow (ManagedWindow* window) {
        if (destroyed) return;
        if (window != nullptr) {
          return destroyWindow(reinterpret_cast<Window*>(window));
        }
      }

      void destroyWindow (Window* window) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return;
        if (window != nullptr && windows[window->index] != nullptr) {
          auto metadata = reinterpret_cast<ManagedWindow*>(window);

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

      ManagedWindow* createWindow (WindowOptions opts) {
        std::lock_guard<std::recursive_mutex> guard(this->mutex);
        if (destroyed) return nullptr;
        StringStream env;

        if (inits[opts.index] && windows[opts.index] != nullptr) {
          return windows[opts.index];
        }

        if (opts.appData.size() > 0) {
          for (auto const &envKey : parseStringList(opts.appData["build_env"])) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << String(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
            );
          }
        } else {
          for (auto const &envKey : parseStringList(this->options.appData["build_env"])) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << String(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
            );
          }
        }

        auto screen = Window::getScreenSize();

        float width = opts.width <= 0
          ? Window::getSizeInPixels(this->options.defaultWidth, screen.width)
          : opts.width;
        float height = opts.height <= 0
          ? Window::getSizeInPixels(this->options.defaultHeight, screen.height)
          : opts.height;
        float minWidth = opts.minWidth <= 0
          ? Window::getSizeInPixels(this->options.defaultMinWidth, screen.width)
          : opts.minWidth;
        float minHeight = opts.minHeight <= 0
          ? Window::getSizeInPixels(this->options.defaultMinHeight, screen.height)
          : opts.minHeight;
        float maxWidth = opts.maxWidth <= 0
          ? Window::getSizeInPixels(this->options.defaultMaxWidth, screen.width)
          : opts.maxWidth;
        float maxHeight = opts.maxHeight <= 0
          ? Window::getSizeInPixels(this->options.defaultMaxHeight, screen.height)
          : opts.maxHeight;

        WindowOptions windowOptions = {
          .resizable = opts.resizable,
          .frameless = opts.frameless,
          .utility = opts.utility,
          .canExit = opts.canExit,
          .width = width,
          .height = height,
          .minWidth = minWidth,
          .minHeight = minHeight,
          .maxWidth = maxWidth,
          .maxHeight = maxHeight,
          .index = opts.index,
          .debug = isDebugEnabled() || opts.debug,
          .isTest = this->options.isTest,
          .headless = this->options.headless || opts.headless || opts.appData["build_headless"] == "true",

          .cwd = this->options.cwd,
          .title = opts.title.size() > 0 ? opts.title : "",
          .url = opts.url.size() > 0 ? opts.url : "data:text/html,<html>",
          .argv = this->options.argv,
          .preload = opts.preload.size() > 0 ? opts.preload : "",
          .env = env.str(),
          .appData = this->options.appData
        };

        if (isDebugEnabled()) {
          this->log("Creating Window#" + std::to_string(opts.index));
        }

        auto window = new ManagedWindow(*this, app, windowOptions);

        window->status = WindowStatus::WINDOW_CREATED;
        window->onExit = this->options.onExit;
        window->onMessage = this->options.onMessage;

        windows[opts.index] = window;
        inits[opts.index] = true;

        return window;
      }

      ManagedWindow* createDefaultWindow (WindowOptions opts) {
        return createWindow(WindowOptions {
          .resizable = opts.resizable,
          .frameless = opts.frameless,
          .utility = opts.utility,
          .canExit = true,
          .width = opts.width,
          .height = opts.height,
          .index = 0,
      #ifdef PORT
          .port = PORT,
      #endif
          .appData = opts.appData
        });
      }

      JSON::Array json (std::vector<int> indices) {
        auto i = 0;
        JSON::Array result;
        for (auto index : indices) {
          auto window = getWindow(index);
          if (window != nullptr) {
            result[i++] = window->json();
          }
        }
        return result;
      }
  };

#if defined(_WIN32)
  using IEnvHandler = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
  using IConHandler = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
  using INavHandler = ICoreWebView2NavigationCompletedEventHandler;
  using IRecHandler = ICoreWebView2WebMessageReceivedEventHandler;
  using IArgs = ICoreWebView2WebMessageReceivedEventArgs;

  enum WINDOWCOMPOSITIONATTRIB {
    WCA_UNDEFINED = 0,
    WCA_NCRENDERING_ENABLED = 1,
    WCA_NCRENDERING_POLICY = 2,
    WCA_TRANSITIONS_FORCEDISABLED = 3,
    WCA_ALLOW_NCPAINT = 4,
    WCA_CAPTION_BUTTON_BOUNDS = 5,
    WCA_NONCLIENT_RTL_LAYOUT = 6,
    WCA_FORCE_ICONIC_REPRESENTATION = 7,
    WCA_EXTENDED_FRAME_BOUNDS = 8,
    WCA_HAS_ICONIC_BITMAP = 9,
    WCA_THEME_ATTRIBUTES = 10,
    WCA_NCRENDERING_EXILED = 11,
    WCA_NCADORNMENTINFO = 12,
    WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
    WCA_VIDEO_OVERLAY_ACTIVE = 14,
    WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
    WCA_DISALLOW_PEEK = 16,
    WCA_CLOAK = 17,
    WCA_CLOAKED = 18,
    WCA_ACCENT_POLICY = 19,
    WCA_FREEZE_REPRESENTATION = 20,
    WCA_EVER_UNCLOAKED = 21,
    WCA_VISUAL_OWNER = 22,
    WCA_HOLOGRAPHIC = 23,
    WCA_EXCLUDED_FROM_DDA = 24,
    WCA_PASSIVEUPDATEMODE = 25,
    WCA_USEDARKMODECOLORS = 26,
    WCA_LAST = 27
  };

  struct WINDOWCOMPOSITIONATTRIBDATA {
    WINDOWCOMPOSITIONATTRIB Attrib;
    PVOID pvData;
    SIZE_T cbData;
  };
#endif
}
#endif
