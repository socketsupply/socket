#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#include "../app/app.hh"
#include "../ipc/ipc.hh"
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
  NSDraggingSource>
- (NSDragOperation) draggingSession: (NSDraggingSession *) session
sourceOperationMaskForDraggingContext: (NSDraggingContext) context;
@end
#endif

@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
- (void) webview: (SSCBridgedWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;
@end
#endif

namespace SSC {
  class DragDrop;

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

      void about ();
      void eval (const String&);
      void show (const String&);
      void hide (const String&);
      void kill ();
      void exit (int code);
      void close (int code);
      void navigate (const String&, const String&);
      void setTitle (const String&, const String&);
      void setSize (const String&, int, int, int);
      void setContextMenu (const String&, const String&);
      void closeContextMenu (const String&);
      void closeContextMenu ();
#if defined(__linux__) && !defined(__ANDROID__)
      void closeContextMenu (GtkWidget *, const String&);
#endif
      void setBackgroundColor (int r, int g, int b, float a);
      void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos);
      void setSystemMenu (const String& seq, const String& menu);
      ScreenSize getScreenSize ();
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
  };

  struct WindowFactoryOptions {
    int defaultHeight = 0;
    int defaultWidth = 0;
    bool headless = false;
    bool isTest;
    String argv = "";
    String cwd = "";
    Map appData;
    MessageCallback onMessage = [](const String) {};
    ExitCallback onExit = nullptr;
  };

  class WindowFactory  {
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
          WindowFactory &factory;

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

          void show (const String &seq) {
            auto index = std::to_string(this->opts.index);
            factory.log("Showing Window#" + index + " (seq=" + seq + ")");
            status = WindowStatus::WINDOW_SHOWING;
            Window::show(seq);
            status = WindowStatus::WINDOW_SHOWN;
          }

          void hide (const String &seq) {
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
      WindowFactoryOptions options;

      WindowFactory (App &app) :
        app(app),
        inits(SSC_MAX_WINDOWS),
        windows(SSC_MAX_WINDOWS)
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

      void inline log (const String line) {
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
        StringStream env;

        if (inits[opts.index]) {
          return reinterpret_cast<Window*>(windows[opts.index]);
        }

        if (opts.appData.size() > 0) {
          for (auto const &envKey : split(opts.appData["env"], ',')) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << String(
              cleanKey + "=" + encodeURIComponent(envValue) + "&"
            );
          }
        } else {
          for (auto const &envKey : split(this->options.appData["env"], ',')) {
            auto cleanKey = trim(envKey);
            auto envValue = getEnv(cleanKey.c_str());

            env << String(
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

#if defined(_WIN32)
  using IEnvHandler = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
  using IConHandler = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
  using INavHandler = ICoreWebView2NavigationCompletedEventHandler;
  using IRecHandler = ICoreWebView2WebMessageReceivedEventHandler;
  using IArgs = ICoreWebView2WebMessageReceivedEventArgs;

	// constexpr COLORREF darkBkColor = 0x383838;
	// constexpr COLORREF darkTextColor = 0xFFFFFF;
	// static HBRUSH hbrBkgnd = nullptr;

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

  using RefreshImmersiveColorPolicyState = VOID(WINAPI*)();
  using SetWindowCompositionAttribute = BOOL(WINAPI *)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
  using ShouldSystemUseDarkMode = BOOL(WINAPI*)();
  using AllowDarkModeForApp = BOOL(WINAPI*)(BOOL allow);

  extern RefreshImmersiveColorPolicyState refreshImmersiveColorPolicyState;
  extern SetWindowCompositionAttribute setWindowCompositionAttribute;
  extern ShouldSystemUseDarkMode shouldSystemUseDarkMode;
  extern AllowDarkModeForApp allowDarkModeForApp;

  auto bgBrush = CreateSolidBrush(RGB(0, 0, 0));
#endif
}
#endif
