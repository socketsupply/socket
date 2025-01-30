#ifndef SOCKET_RUNTIME_WINDOW_H
#define SOCKET_RUNTIME_WINDOW_H

#include "javascript.hh"
#include "bluetooth.hh"
#include "context.hh"
#include "webview.hh"
#include "color.hh"
#include "ipc.hh"

#include "window/dialog.hh"
#include "window/hotkey.hh"

#ifndef SOCKET_RUNTIME_MAX_WINDOWS
#define SOCKET_RUNTIME_MAX_WINDOWS 64
#endif

#ifndef SOCKET_RUNTIME_MAX_WINDOWS_RESERVED
#define SOCKET_RUNTIME_MAX_WINDOWS_RESERVED 32
#endif

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
#define WM_HANDLE_DEEP_LINK WM_APP + 1
#define WM_SOCKET_TRAY WM_APP + 2
#endif

namespace ssc::runtime::window {
  // forward
  class Window;
}

namespace ssc::runtime::bridge {
  // forward
  class Bridge;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@class SSCWindow;

@interface SSCWindowDelegate :
#if SOCKET_RUNTIME_PLATFORM_IOS
  NSObject<
    UIScrollViewDelegate,
    WKScriptMessageHandler
  >
#else
  NSObject <
    NSWindowDelegate,
    WKScriptMessageHandler
  >
#endif
@end

@interface SSCWindow :
#if SOCKET_RUNTIME_PLATFORM_IOS
  UIWindow
#else
  NSWindow
#endif

#if SOCKET_RUNTIME_PLATFORM_MACOS
  @property (nonatomic, strong) NSView *titleBarView;
  @property (nonatomic) NSPoint windowControlOffsets;
#endif
  @property (nonatomic, strong) SSCWebView* webview;
@end
#endif

namespace ssc::runtime::window {
#if SOCKET_RUNTIME_PLATFORM_WINDOWS
  class DragDrop;
#endif

  using Client = webview::Client;

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  /**
   * A container for holding an application's screen size
   */
  struct ScreenSize {
    int width = 0;
    int height = 0;
  };

  class IBridge : public webview::IBridge {
    public:
      // handlers
      using NavigateHandler = Function<void(const String&)>;
      using EvaluateJavaScriptHandler = Function<void(const String&)>;

      EvaluateJavaScriptHandler evaluateJavaScriptHandler = nullptr;
      NavigateHandler navigateHandler = nullptr;
      bluetooth::Bluetooth bluetooth;

      using webview::IBridge::IBridge;
      virtual void configureWebView (webview::WebView* webview) = 0;
      virtual void configureSchemeHandlers (const webview::SchemeHandlers::Configuration&) = 0;
      virtual void configureNavigatorMounts () = 0;
      virtual bool evaluateJavaScript (const String&) = 0;
      virtual bool navigate (const String& url) = 0;
      virtual void init () = 0;
  };

  /**
   * `Window` is a base class that implements a variety of APIs for a
   * window on host platforms. Windows contain a WebView that is connected
   * to the core runtime through a window's "IPC Bridge".
   */
  class Window {
    public:
      using EvalCallback = Function<void(const JSON::Any)>;

      /**
       * A container for representing the window position in a
       * Cartesian coordinate system (screen coordinates)
       */
      struct Position {
        float x = 0.0f;
        float y = 0.0f;
      };

      /**
       * A container for representing the size of a window.
       */
      struct Size {
        int width = 0;
        int height = 0;
      };

      /**
       * An enumeration of the "ready state" of a window.
       * These values closel relate to the `globalThis.document.readyState`
       * possible values (loading, interactive, complete)
       */
      enum class ReadyState {
        None,
        Loading,
        Interactive,
        Complete
      };

      /**
       * `Window::Options` is an extended `webview::Preload::Options` container for
       * configuring a new `Window`.
       */
      struct Options : public webview::Preload::Options {
        /**
         * If `true`, the window can be minimized.
         * This option value is only supported on desktop.
         */
        bool minimizable = true;

        /**
         * If `true`, the window can be maximized.
         * This option value is only supported on desktop.
         */
        bool maximizable = true;

        /**
         * If `true`, the window can be resized.
         * This option value is only supported on desktop.
         */
        bool resizable = true;

        /**
         * If `true`, the window can be closed.
         * This option value is only supported on desktop.
         */
        bool closable = true;

        /**
         * If `true`, the window can be "frameless".
         * This option value is only supported on desktop.
         */
        bool frameless = false;

        /**
         * If `true`, the window is considered a "utility" window.
         * This option value is only supported on desktop.
         */
        bool utility = false;

        /**
         * If `true`, the window, when the window is "closed", it can
         * exit the application.
         * This option value is only supported on desktop.
         */
        bool shouldExitApplicationOnClose = false;

        /**
         * If `true`, when handling the `socket:` scheme, the bridge will
         * prefer an applicable service worker before request resources.
         */
        bool shouldPreferServiceWorker = false;

        /**
         * The maximum height in screen pixels the window can be.
         */
        float maxHeight = 0.0;

        /**
         * The minimum height in screen pixels the window can be.
         */
        float minHeight = 0.0;

        /**
         * The absolute height in screen pixels the window can be.
         */
        float height = 0.0;

        /**
         * The maximum width in screen pixels the window can be.
         */
        float maxWidth = 0.0;

        /**
         * The minimum width in screen pixels the window can be.
         */
        float minWidth = 0.0;

        /**
         * The absolute width in screen pixels the window can be.
         */
        float width = 0.0;

        /**
         * The window border/corner radius.
         * This option value is only supported on macOS.
         */
        float radius = 0.0;

        /**
         * Thw window frame margin.
         * This option value is only supported on macOS.
         */
        float margin = 0.0;

        /**
         * A string (split on ':') provides two float values which will
         * set the window's aspect ratio.
         * This option value is only supported on desktop.
         */
        String aspectRatio = "";

        /**
         * A string that describes a style for the window title bar.
         * Valid values are:
         *   - hidden
         *   - hiddenInset
         * This option value is only supported on macOS and Windows.
         */
        String titlebarStyle = "";

        /**
         * A string value (split on 'x') in the form of `XxY` where
         *   - `X` is the value in screen pixels offset from the left of the
         *         window frame
         *   - `Y` is the value in screen pixels offset from the top of the
         *         window frame
         * The values control the offset of the "close", "minimize", and "maximize"
         * button controls for a window.
         * This option value is only supported on macOS.
         */
        String windowControlOffsets = "";

        /**
         * A string value in the form of `rgba(r, g, b, a)` where
         *   - `r` is the "red" channel value, an integer between `0` and `255`
         *   - `g` is the "green" channel value, an integer between `0` and `255`
         *   - `b` is the "blue" channel value, an integer between `0` and `255`
         *   - `a` is the "alpha" channel value, a float between `0` and `1`
         * The values represent the background color of a window when the platform
         * system theme is in "light mode". This also be the "default" theme.
         */
        String backgroundColorLight = "";

        /**
         * A string value in the form of `rgba(r, g, b, a)` where
         *   - `r` is the "red" channel value, an integer between `0` and `255`
         *   - `g` is the "green" channel value, an integer between `0` and `255`
         *   - `b` is the "blue" channel value, an integer between `0` and `255`
         *   - `a` is the "alpha" channel value, a float between `0` and `1`
         * The values represent the background color of a window when the platform
         * system theme is in "dark mode".
         */
        String backgroundColorDark = "";

        /**
         * An optional directory (sandboxing applied) where resources should be
         * loaded from
         */
        String resourcesDirectory = "";

        /**
         * A callback function that is called when a "script message" is received
         * from the WebVew.
         */
        MessageCallback onMessage = [](const String) {};

        /**
         * A callback function that is called when the window wants to exit the
         * application. This function is called _only_ when the
         * `shouldExitApplicationOnClose` option is `true`.
         */
        ExitCallback onExit = nullptr;

        /**
         * The origin for this window.
         */
        webview::Origin origin;

        /**
         * A unique token for a window to ensure only one of this particular
         * type can exist. This value is handled in the IPC routing only.
         */
        String token = std::to_string(crypto::rand64());

        bool createServiceWorker = true;
      };

      /**
       * The current "ready state" of the window
       */
      ReadyState readyState = ReadyState::None;

      /**
       * The options used to create this window.
       */
      const Window::Options options;

      /**
       * The "hot key" context for this window.
       * The "hot key" features are only available on desktop platforms.
       */
      HotKeyContext hotkey;

      /**
       * The IPC bridge that connects the application window's WebView to
       * the runtime and various core modules and functions.
       */
      SharedPointer<bridge::Bridge> bridge = nullptr;

      /**
       * The (x, y) screen coordinate position of the window.
       */
      Position position;

      /**
       * The current mouse (x, y) position when dragging started.
       */
      Position dragStart;

      /**
       * The current mouse (x, y) position while dragging.
       */
      Position dragging;

      /**
       * The size (width, height) of the window.
       */
      Size size;

      /**
       * A callback function that is called when a "script message" is received
       * from the WebVew.
       */
      MessageCallback onMessage = [](const String) {};

      /**
       * A callback function that is called when the window wants to exit the
       * application. This function is called _only_ when
       * `options.shouldExitApplicationOnClose` is `true`.
       */
      ExitCallback onExit = nullptr;

      /**
       * The unique index of the window instance. This value is used by the
       * `Manager` and various standard libary IPC functions for
       * addressing a window a unique manner.
       */
      int index = 0;

      /**
       * This value is `true` when the window has closed an is indicating
       * that the application is exiting
       */
      Atomic<bool> isExiting = false;

      /**
       * A pointer to the platform WebView.
       */
      webview::WebView* webview = nullptr;

      /**
       * A controller for showing system dialogs such as a "file picker"
       */
      Dialog dialog;

      /**
       * The current background color of the window
       */
      color::Color backgroundColor;

    #if SOCKET_RUNTIME_PLATFORM_IOS
      SSCWebViewController* viewController = nullptr;
    #endif

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCWindowDelegate* windowDelegate = nullptr;
      SSCWindow* window = nullptr;
      WKProcessPool* processPool = nullptr;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      GtkSelectionData *selectionData = nullptr;
      GtkAccelGroup *accelGroup = nullptr;

      GtkWidget* vbox = nullptr;
      GtkWidget* window = nullptr;
      GtkWidget* menubar = nullptr;
      GtkWidget* menutray = nullptr;
      GtkWidget* contextMenu = nullptr;

      bool isDarkMode = false;

    #if SOCKET_RUNTIME_DESKTOP_EXTENSION
      void* userContentManager;
      void* policies;
      void* settings;
    #else
      WebKitUserContentManager* userContentManager;
      WebKitWebsitePolicies* policies;
      WebKitSettings* settings;
    #endif

      int contextMenuID;
      double dragLastX = 0;
      double dragLastY = 0;

      bool shouldDrag;
      Vector<String> draggablePayload;
      bool isDragInvokedInsideWindow;
      GdkPoint initialLocation;
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
      ICoreWebView2Controller *controller = nullptr;
      HMENU menubar;
      HMENU menutray;
      DWORD mainThread = GetCurrentThreadId();

      double dragLastX = 0;
      double dragLastY = 0;
      bool shouldDrag;
      SharedPointer<DragDrop> drop;

      POINT minimumSize = POINT {0, 0};
      POINT maximumSize = POINT {0, 0};

      POINT initialCursorPos = POINT {0, 0};
      RECT initialWindowPos = RECT {0, 0, 0, 0};

      HWND window;
      Map<int, String> menuMap;
      Map<int, String> menuTrayMap;

      void resize (HWND window);
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
      String pendingNavigationLocation;
      jobject self = nullptr;
      Map<String, EvalCallback> evaluateJavaScriptCallbacks;
    #endif

      Window (SharedPointer<bridge::Bridge> bridge, const Window::Options&);
      ~Window ();

      static ScreenSize getScreenSize ();

      void about ();
      void eval (const String&, const EvalCallback callback = nullptr);
      void show ();
      void hide ();
      void kill ();
      void exit (int code = 0);
      void close (int code = 0);
      void minimize ();
      void maximize ();
      void restore ();
      void navigate (const String&);
      const String getTitle () const;
      void setTitle (const String&);
      Size getSize ();
      const Size getSize () const;
      void setSize (int width, int height, int hints = 0);
      void setPosition (float, float);
      void setContextMenu (const String&, const String&);
      void closeContextMenu (const String&);
      void closeContextMenu ();
    #if SOCKET_RUNTIME_PLATFORM_LINUX
      void closeContextMenu (GtkWidget *, const String&);
    #endif
      void setBackgroundColor (int r, int g, int b, float a);
      void setBackgroundColor (const String& rgba);
      void setBackgroundColor (const color::Color& color);
      String getBackgroundColor ();
      void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos);
      void setSystemMenu (const String& dsl);
      void setMenu (const String& dsl, const bool& isTrayMenu);
      void setTrayMenu (const String& dsl);
      void showInspector ();

      void handleApplicationURL (const String& url);
      void onReadyStateChange (const ReadyState readyState) {}

      void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      ) {
        if (seq.find("R") == 0) {
          this->eval(javascript::getResolveToRenderProcessJavaScript(seq, state, value));
        }
      }

      void resolvePromise (
        const String& seq,
        const String& state,
        const JSON::Any& json
      ) {
        auto result = ipc::Result(json);
        return resolvePromise(seq, state, result.str());
      }

      static float getSizeInPixels (String sizeInPercent, int screenSize) {
        if (sizeInPercent.size() > 0) {
          if (sizeInPercent.back() == '%') {
            sizeInPercent.pop_back();
            return ((float) screenSize) * std::stof(sizeInPercent) / 100.0f;
          }
          return std::stof(sizeInPercent);
        }
        return 0.0f;
      }
  };

  struct ManagerOptions : Window::Options {
    String defaultHeight = "0";
    String defaultWidth = "0";
    String defaultMinWidth = "0";
    String defaultMinHeight = "0";
    String defaultMaxWidth = "100%";
    String defaultMaxHeight = "100%";
  };

  struct WindowPropertiesFlags {
    bool showTitle = false;
    bool showSize = false;
    bool showStatus = false;
  };

  class Manager  {
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
          Manager &manager;
          Vector<String> pendingApplicationURLs;
          Mutex mutex;

          ManagedWindow (
            Manager &manager,
            SharedPointer<bridge::Bridge> bridge,
            const Window::Options& options
          );

          ~ManagedWindow ();

          void show ();
          void hide ();
          void close (int code = 0);
          void exit (int code = 0);
          void kill ();
          void gc ();
          void navigate (const URL&);
          JSON::Object json () const;
          void handleApplicationURL (const String& url);
          void onReadyStateChange (const ReadyState& readyState);
          bool emit (const String& event, const JSON::Any& json = {});
          void setBackgroundColor (int r, int g, int b, float a);
          void setBackgroundColor (const String& rgba);
          void setBackgroundColor (const color::Color& color);
          String getBackgroundColor ();
      };

      // maps window index to managed window
      Vector<SharedPointer<ManagedWindow>> windows;
      context::RuntimeContext& context;

      ManagerOptions options;
      Atomic<bool> destroyed = false;
      Mutex mutex;

      Manager (context::RuntimeContext&);
      Manager () = delete;
      Manager (const Manager&) = delete;
      ~Manager ();

      void destroy ();
      void configure (const ManagerOptions& configuration);

      int getNextWindowIndex (bool reserved = false) const;
      int getRandomWindowIndex (bool reserved = false) const;

      SharedPointer<ManagedWindow> getWindow (int index, const WindowStatus status);
      SharedPointer<ManagedWindow> getWindow (int index);
      SharedPointer<ManagedWindow> getWindowForClient (const Client&);
      SharedPointer<ManagedWindow> getWindowForBridge (const bridge::Bridge*);
      SharedPointer<ManagedWindow> getWindowForWebView (const webview::WebView*);
      SharedPointer<ManagedWindow> getOrCreateWindow (int index);
      SharedPointer<ManagedWindow> getOrCreateWindow (int index, const Window::Options&);
      WindowStatus getWindowStatus (int index);

      SharedPointer<ManagedWindow> createWindow (const Window::Options&);
      SharedPointer<ManagedWindow> createDefaultWindow (const Window::Options&);

      void destroyWindow (int index);
      JSON::Array json (const Vector<int>& indices);
      bool emit (const String& event, const JSON::Any& json = {});
  };
}
#endif
