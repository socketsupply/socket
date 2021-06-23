#include "common.hh"
#include "win64/WebView2.h"
#include "win64/options.h"

#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"WebView2LoaderStatic.lib")

inline void alert (const std::wstring &ws) {
  MessageBoxA(nullptr, Opkit::WStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

inline void alert (const std::string &s) {
  MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

inline void alert (const char* s) {
  MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}
 
namespace Opkit {
  using IEnvHandler = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
  using IConHandler = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
  using INavHandler = ICoreWebView2NavigationCompletedEventHandler;

  class App: public IApp {
    public:
      MSG msg;
      _In_ HINSTANCE hInstance;
      DWORD mainThread = GetCurrentThreadId();

      App(void* h);
      int run();
      void exit();
      void kill();
      void dispatch(std::function<void()>);
      std::string getCwd(const std::string&);
  };

  class Window : public IWindow {
    HWND window;
    DWORD mainThread = GetCurrentThreadId();

    public:
      Window(App&, WindowOptions);
      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
      ICoreWebView2 *webview = nullptr;
      ICoreWebView2Controller *controller = nullptr;

      App app;

      void resize (HWND window);
      POINT m_minsz = POINT{0, 0};
      POINT m_maxsz = POINT{0, 0};
      bool initDone = false;

      void eval(const std::string&);
      void show();
      void exit();
      void kill();
      void hide();
      void navigate(const std::string&);
      void setSize(int, int, int);
      void setTitle(const std::string&);
      void setContextMenu(std::string, std::string);
      void setSystemMenu(std::string);
      std::string openDialog(bool, bool, bool, std::string, std::string);
      int openExternal(std::string);
  };

  App::App(void* h): hInstance((_In_ HINSTANCE) h) {
    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    /* HICON icon = (HICON) LoadImage(
      hInstance,
      IDI_APPLICATION,
      IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CYSMICON),
      LR_DEFAULTCOLOR
    ); */

    auto *szWindowClass = L"DesktopApp";
    auto *szTitle = L"Opkit";
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
    wcex.lpfnWndProc = Window::WndProc;

    if (!RegisterClassEx(&wcex)) {
      alert("App failed to register");
    }
  };

  Window::Window (App& app, WindowOptions opts) : app(app) {
    window = CreateWindow(
      TEXT("DesktopApp"), TEXT("Opkit"),
      WS_OVERLAPPEDWINDOW,
      100000,
      100000,
      1024, 780,
      NULL, NULL,
      app.hInstance, NULL
    );

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) this);

    std::string preload(
      "window.external = {\n"
      "  invoke: arg => window.chrome.webview.postMessage(arg)\n"
      "};\n"
      "" + opts.preload + "\n"
    );

    auto res = CreateCoreWebView2EnvironmentWithOptions(
      nullptr,
      nullptr,
      nullptr,
      Microsoft::WRL::Callback<IEnvHandler>(
        [&, preload](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
          env->CreateCoreWebView2Controller(
            window,
            Microsoft::WRL::Callback<IConHandler>(
              [&, preload](HRESULT result, ICoreWebView2Controller* c) -> HRESULT {

                if (c != nullptr) {
                  controller = c;
                  controller->get_CoreWebView2(&webview);

                  RECT bounds;
                  GetClientRect(window, &bounds);
                  controller->put_Bounds(bounds);
                  controller->AddRef();
                }

                ICoreWebView2Settings* Settings;
                webview->get_Settings(&Settings);
                Settings->put_IsScriptEnabled(TRUE);
                Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                Settings->put_IsWebMessageEnabled(TRUE);
                Settings->put_AreDevToolsEnabled(TRUE);
                Settings->put_IsZoomControlEnabled(FALSE);

                webview->AddScriptToExecuteOnDocumentCreated(
                  StringToWString(preload).c_str(),
                  Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                    [&](HRESULT error, PCWSTR id) -> HRESULT {
                      return S_OK;
                    }
                  ).Get()
                );

                return S_OK;
              }
            ).Get()
          );

          return S_OK;
        }
      ).Get()
    );

    if (!SUCCEEDED(res)) {
      alert("Unable to create webview");
    }
  }

  int App::run () {
    MSG msg;
    BOOL res = GetMessage(&msg, nullptr, 0, 0);

    if (msg.hwnd) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    if (msg.message == WM_APP) {
      auto f = (std::function<void()> *)(msg.lParam);
      (*f)();
      delete f;
    }

    if (msg.message == WM_QUIT) {
      if (shouldExit) return 1;
    }

    return 0;
  }

  void App::exit () {
    if (onExit != nullptr) onExit();
  }

  void App::kill () {
    shouldExit = true;
    PostQuitMessage(WM_QUIT);
  }

  void App::dispatch (std::function<void()> cb) {
    PostThreadMessage(
      mainThread,
      WM_APP,
      0,
      (LPARAM) new std::function<void()>(cb)
    );
  }

  std::string App::getCwd (const std::string& _) {
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    auto path = fs::path { filename }.remove_filename();
    return pathToString(path);
  }

  void Window::kill () {
    if (controller != nullptr) controller->Close();
    DestroyWindow(window);
  }

  void Window::exit () {
    if (onExit == nullptr) {
      PostQuitMessage(WM_QUIT);
      return;
    }

    onExit();
  }

  void Window::show () {
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    RECT r, r1;
    GetWindowRect(window, &r);
    GetWindowRect(GetDesktopWindow(), &r1);

    MoveWindow(window, (
      (r1.right-r1.left) - (r.right-r.left)) / 2,
      ((r1.bottom-r1.top) - (r.bottom-r.top)) / 2,
      (r.right-r.left),
      (r.bottom-r.top),
      0
    );
  }

  void Window::hide () {
    ShowWindow(window, SW_HIDE);
    UpdateWindow(window);
  }

  void Window::resize (HWND window) {
    if (controller == nullptr) {
      return;
    }

    RECT bounds;
    GetClientRect(window, &bounds);
    controller->put_Bounds(bounds);
  }

  void Window::eval (const std::string& s) {
    if (webview == nullptr) {
      return;
    }

    webview->ExecuteScript(
      StringToWString(s).c_str(),
      nullptr
    );
  }

  void Window::navigate (const std::string& s) {
    webview->Navigate(StringToWString(s).c_str());
  }

  void Window::setTitle (const std::string& title) {
    SetWindowText(window, title.c_str());
  }

  void Window::setSize (int width, int height, int hints) {
    auto style = GetWindowLong(window, GWL_STYLE);

    if (hints == WINDOW_HINT_FIXED) {
      style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    } else {
      style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
    }

    SetWindowLong(window, GWL_STYLE, style);

    if (hints == WINDOW_HINT_MAX) {
      m_maxsz.x = width;
      m_maxsz.y = height;
    } else if (hints == WINDOW_HINT_MIN) {
      m_minsz.x = width;
      m_minsz.y = height;
    } else {
      RECT r;
      r.left = r.top = 0;
      r.right = width;
      r.bottom = height;

      AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW, 0);

      SetWindowPos(
        window,
        NULL,
        r.left, r.top, r.right - r.left, r.bottom - r.top,
        SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_FRAMECHANGED
      );

      resize(window);
    }
  }

  void Window::setSystemMenu (std::string) {
    // TODO implement
  }

  void Window::setContextMenu (std::string seq, std::string value) {
    // TODO implement
  }

  int Window::openExternal (std::string url) {
    return 0;
  }

  std::string Window::openDialog (bool isSave, bool allowDirs, bool allowFiles, std::string, std::string) {
    return std::string("");
  }

  LRESULT CALLBACK Window::WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {
    
    Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (message) {
      case WM_SIZE: {
        if (w == nullptr || w->webview == nullptr) {
          break;
        }

        RECT bounds;
        GetClientRect(hWnd, &bounds);
        w->controller->put_Bounds(bounds);
        break;
      }

      case WM_DESTROY: {
        if (w == nullptr) {
          PostQuitMessage(WM_QUIT);
          break;
        }

        w->exit();
        break;
      }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return 0;
  }

} // namespace Opkit