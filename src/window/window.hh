#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#include <iostream>

#include "../ipc/ipc.hh"
#include "../app/app.hh"
#include "../core/env.hh"
#include "../core/config.hh"

#include "hotkey.hh"
#include "options.hh"

#ifndef SSC_MAX_WINDOWS
#define SSC_MAX_WINDOWS 32
#endif

#define SSC_SERVICE_WORKER_CONTAINER_WINDOW_INDEX SSC_MAX_WINDOWS + 1

#ifndef SSC_MAX_WINDOWS_RESERVED
#define SSC_MAX_WINDOWS_RESERVED 16
#endif

#if SSC_PLATFORM_WINDOWS
#define WM_HANDLE_DEEP_LINK WM_APP + 1
#define WM_SOCKET_TRAY WM_APP + 2
#endif

namespace SSC {
  // forward
  class Dialog;
  class Window;
}

#if SSC_PLATFORM_APPLE
@class SSCBridgedWebView;
@class SSCWindow;

@interface SSCWindowDelegate :
#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
  NSObject
#else
  NSObject <NSWindowDelegate, WKScriptMessageHandler>
#endif
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end

@interface SSCWindow :
#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
  UIWindow
#else
  NSWindow
#endif

@property (nonatomic) SSC::Window* window;
@property (nonatomic, assign) SSCBridgedWebView *webview;

#if SSC_PLATFORM_MACOS
@property (nonatomic, retain) NSView *titleBarView;
@property (nonatomic) NSPoint windowControlOffsets;
#endif
@end

@interface WKOpenPanelParameters (WKPrivate)
- (NSArray<NSString*>*) _acceptedMIMETypes;
- (NSArray<NSString*>*) _acceptedFileExtensions;
- (NSArray<NSString*>*) _allowedFileExtensions;
@end

@interface SSCBridgedWebView :
#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
  WKWebView<WKUIDelegate>
#else
  WKWebView<
    WKUIDelegate,
    NSDraggingDestination,
    NSFilePromiseProviderDelegate,
    NSDraggingSource
  >

  @property (nonatomic) NSPoint initialWindowPos;
  @property (nonatomic) CGFloat contentHeight;
  @property (nonatomic) CGFloat radius;
  @property (nonatomic) CGFloat margin;
  @property (nonatomic) BOOL shouldDrag;

  -   (NSDragOperation) draggingSession: (NSDraggingSession *) session
  sourceOperationMaskForDraggingContext: (NSDraggingContext) context;

  -             (void) webView: (WKWebView*) webView
    runOpenPanelWithParameters: (WKOpenPanelParameters*) parameters
              initiatedByFrame: (WKFrameInfo*) frame
             completionHandler: (void (^)(NSArray<NSURL*>*)) completionHandler;
#endif

#if SSC_PLATFORM_MACOS || (SSC_PLATFORM_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)

  -                                      (void) webView: (WKWebView*) webView
   requestDeviceOrientationAndMotionPermissionForOrigin: (WKSecurityOrigin*) origin
                                       initiatedByFrame: (WKFrameInfo*) frame
                                        decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler;

  -                        (void) webView: (WKWebView*) webView
   requestMediaCapturePermissionForOrigin: (WKSecurityOrigin*) origin
                         initiatedByFrame: (WKFrameInfo*) frame
                                     type: (WKMediaCaptureType) type
                          decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler;
#endif

  -                     (void) webView: (WKWebView*) webView
    runJavaScriptAlertPanelWithMessage: (NSString*) message
                      initiatedByFrame: (WKFrameInfo*) frame
                     completionHandler: (void (^)(void)) completionHandler;

  -                       (void) webView: (WKWebView*) webView
    runJavaScriptConfirmPanelWithMessage: (NSString*) message
                        initiatedByFrame: (WKFrameInfo*) frame
                       completionHandler: (void (^)(BOOL result)) completionHandler;
@end

#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
@interface SSCUIPickerDelegate : NSObject<
  UIDocumentPickerDelegate,

  // TODO(@jwerle): use 'PHPickerViewControllerDelegate' instead
  UIImagePickerControllerDelegate,
  UINavigationControllerDelegate
>
  @property (nonatomic) SSC::Dialog* dialog;

  // UIDocumentPickerDelegate
  -  (void) documentPicker: (UIDocumentPickerViewController*) controller
    didPickDocumentsAtURLs: (NSArray<NSURL*>*) urls;
  - (void) documentPickerWasCancelled: (UIDocumentPickerViewController*) controller;

  // UIImagePickerControllerDelegate
  -  (void) imagePickerController: (UIImagePickerController*) picker
    didFinishPickingMediaWithInfo: (NSDictionary<UIImagePickerControllerInfoKey, id>*) info;
  - (void) imagePickerControllerDidCancel: (UIImagePickerController*) picker;
@end
#endif

#endif

namespace SSC {
#if SSC_PLATFORM_WINDOWS
  class DragDrop;
#endif

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  struct ScreenSize {
    int height = 0;
    int width = 0;
  };

  class Window {
    public:
      HotKeyContext hotkey;
      WindowOptions opts;
      App& app;

      MessageCallback onMessage = [](const String) {};
      ExitCallback onExit = nullptr;
      IPC::Bridge *bridge = nullptr;
      int index = 0;
      int width = 0;
      int height = 0;
      bool exiting = false;

    #if SSC_PLATFORM_APPLE
    #if SSC_PLATFORM_MACOS
      SSCWindow* window = nullptr;
    #endif
      SSCBridgedWebView* webview;
      SSCWindowDelegate* windowDelegate = nullptr;
      WKProcessPool* processPool = nullptr;
    #elif SSC_PLATFORM_LINUX
      GtkSelectionData *selectionData = nullptr;
      GtkAccelGroup *accelGroup = nullptr;
      GtkWidget *webview = nullptr;
      GtkWidget *window = nullptr;
      GtkWidget *menubar = nullptr;
      GtkWidget *menutray = nullptr;
      GtkWidget *vbox = nullptr;
      GtkWidget *popup = nullptr;
      int popupId;
      double dragLastX = 0;
      double dragLastY = 0;
      bool shouldDrag;
      Vector<String> draggablePayload;
      bool isDragInvokedInsideWindow;
      GdkPoint initialLocation;
    #elif SSC_PLATFORM_WINDOWS
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      bool usingCustomEdgeRuntimeDirectory = false;
      ICoreWebView2Controller *controller = nullptr;
      ICoreWebView2 *webview = nullptr;
      HMENU menubar;
      HMENU menutray;
      DWORD mainThread = GetCurrentThreadId();

      double dragLastX = 0;
      double dragLastY = 0;
      bool shouldDrag;
      DragDrop* drop;

      POINT minimumSize = POINT {0, 0};
      POINT maximumSize = POINT {0, 0};

      POINT initialCursorPos = POINT {0, 0};
      RECT initialWindowPos = RECT {0, 0, 0, 0};

      HWND window;
      std::map<int, String> menuMap;
      std::map<int, String> menuTrayMap;
      Path modulePath;

      void resize (HWND window);
    #endif

      Window (App&, WindowOptions);
      ~Window ();

      static ScreenSize getScreenSize ();

      void about ();
      void eval (const String&);
      void show ();
      void hide ();
      void kill ();
      void exit (int code);
      void close (int code);
      void minimize();
      void maximize();
      void restore();
      void navigate (const String&, const String&);
      const String getTitle () const;
      void setTitle (const String&);
      ScreenSize getSize ();
      const ScreenSize getSize () const;
      void setSize (int, int, int);
      void setContextMenu (const String&, const String&);
      void closeContextMenu (const String&);
      void closeContextMenu ();
    #if SSC_PLATFORM_LINUX
      void closeContextMenu (GtkWidget *, const String&);
    #endif
      void setBackgroundColor (int r, int g, int b, float a);
      void setBackgroundColor (const String& rgba);
      String getBackgroundColor ();
      void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos);
      void setSystemMenu (const String& seq, const String& dsl);
      void setMenu (const String& seq, const String& dsl, const bool& isTrayMenu);
      void setTrayMenu (const String& seq, const String& dsl);
      void showInspector ();

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

      void resolvePromise (
        const String& seq,
        const String& state,
        const JSON::Any& json
      ) {
        auto result = SSC::IPC::Result(json);
        return resolvePromise(seq, state, result.str());
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
    String aspectRatio = "";
    String defaultHeight = "0";
    String defaultWidth = "0";
    String defaultMinWidth = "0";
    String defaultMinHeight = "0";
    String defaultMaxWidth = "100%";
    String defaultMaxHeight = "100%";
    bool headless = false;
    bool isTest = false;
    bool canExit = false;
    String argv = "";
    String cwd = "";
    Map userConfig;
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
          );

          ~ManagedWindow ();

          void show (const String &seq);
          void hide (const String &seq);
          void close (int code);
          void exit (int code);
          void kill ();
          void gc ();
          JSON::Object json () const;
      };

      std::chrono::system_clock::time_point lastDebugLogLine;

      App &app;
      bool destroyed = false;
      Vector<SharedPointer<ManagedWindow>> windows;
      Mutex mutex;
      WindowManagerOptions options;

      WindowManager (App &app);
      ~WindowManager ();

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

      void destroy ();
      void configure (const WindowManagerOptions& configuration);

      SharedPointer<ManagedWindow> getWindow (int index, const WindowStatus status);
      SharedPointer<ManagedWindow> getWindow (int index);
      SharedPointer<ManagedWindow> getOrCreateWindow (int index);
      SharedPointer<ManagedWindow> getOrCreateWindow (int index, const WindowOptions& options);
      WindowStatus getWindowStatus (int index);

      void destroyWindow (int index);

      SharedPointer<ManagedWindow> createWindow (const WindowOptions& options);
      SharedPointer<ManagedWindow> createDefaultWindow (const WindowOptions& options);

      JSON::Array json (const Vector<int>& indices);
  };

  class Dialog {
    public:
      struct FileSystemPickerOptions {
        enum class Type { Open, Save };
        bool directories = false;
        bool multiple = false;
        bool files = false;
        Type type = Type::Open;
        String contentTypes;
        String defaultName;
        String defaultPath;
        String title;
      };

    #if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS_SIMULATOR
      SSCUIPickerDelegate* uiPickerDelegate = nullptr;
      Vector<String> delegatedResults;
      std::mutex delegateMutex;
    #endif

      Dialog ();
      ~Dialog ();

      String showSaveFilePicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showOpenFilePicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showDirectoryPicker (
        const FileSystemPickerOptions& options
      );

      Vector<String> showFileSystemPicker (
        const FileSystemPickerOptions& options
      );
  };

#if SSC_PLATFORM_WINDOWS
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
