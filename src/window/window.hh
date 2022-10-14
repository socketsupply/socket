#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#include "../app/app.hh"
#include "../ipc/ipc.hh"
#include "options.hh"

#ifndef SSC_MAX_WINDOWS
#define SSC_MAX_WINDOWS 32
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

#if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
      NSWindow* window;
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

        this->onMessage(getResolveToMainProcessMessage(seq, state, value));
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
