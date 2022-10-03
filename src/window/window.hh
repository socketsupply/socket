#ifndef SSC_WINDOW_WINDOW_H
#define SSC_WINDOW_WINDOW_H

#include "../app/app.hh"
#include "options.hh"

#ifndef SSC_MAX_WINDOWS
#define SSC_MAX_WINDOWS 32
#endif

namespace SSC {
  class Window;

  enum {
    WINDOW_HINT_NONE = 0,  // Width and height are default size
    WINDOW_HINT_MIN = 1,   // Width and height are minimum bounds
    WINDOW_HINT_MAX = 2,   // Width and height are maximum bounds
    WINDOW_HINT_FIXED = 3  // Window size can not be changed by a user
  };

  class IWindow {
    public:
      int index = 0;
      void* bridge;
      WindowOptions opts;
      MessageCallback onMessage = [](const String) {};
      ExitCallback onExit = nullptr;
      virtual void resolvePromise (
        const String& seq,
        const String& state,
        const String& value
      ) = 0;

      virtual void eval (const String&) = 0;
      virtual void show (const String&) = 0;
      virtual void hide (const String&) = 0;
      virtual void close (int code) = 0;
      virtual void exit (int code) = 0;
      virtual void kill () = 0;
      virtual void navigate (const String&, const String&) = 0;
      virtual ScreenSize getScreenSize () = 0;
      virtual void setSize (const String&, int, int, int) = 0;
      virtual void setTitle (const String&, const String&) = 0;
      virtual void setContextMenu (const String&, const String&) = 0;
      virtual void setSystemMenu (const String&, const String&) = 0;
      virtual void setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) = 0;
      virtual void setBackgroundColor (int r, int g, int b, float a) = 0;
      virtual void showInspector () = 0;
      virtual void openDialog (
        const String&,
        bool,
        bool,
        bool,
        bool,
        const String&,
        const String&,
        const String&
      ) = 0;
  };

  class Window : public IWindow {
    public:
      App app;
      WindowOptions opts;

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
          this->eval(resolveToRenderProcess(seq, state, value));
        }

        this->onMessage(resolveToMainProcess(seq, state, value));
      }
  };
}
#endif
