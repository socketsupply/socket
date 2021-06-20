#include "common.hh"
#include "win64/WebView2.h"

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

using HEnv = ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler;
using HCon = ICoreWebView2CreateCoreWebView2ControllerCompletedHandler;
using HRec = ICoreWebView2WebMessageReceivedEventHandler;
using IArgs = ICoreWebView2WebMessageReceivedEventArgs;
using CoreEnv = ICoreWebView2Environment;
using CoreCon = ICoreWebView2Controller;

static ICoreWebView2Controller *webviewController = nullptr;
static ICoreWebView2 *webview = nullptr;

namespace Opkit {
  class App: public IApp {
    public:
      _In_ HINSTANCE hInstance;
      HANDLE mainThread = GetCurrentThread();
      static std::atomic<bool> isReady;

      App(void* h);
      MSG msg;
      int run();
      void exit();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };

  std::atomic<bool> App::isReady {false};

  class Window : public IWindow {
    public:
      HWND window = nullptr;
      POINT p;
      App app;
      Window(App&, WindowOptions);

      static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

      void eval(const std::string&);
      void show();
      void hide();
      void resize();
      void navigate(const std::string&);
      void setSize(int, int);
      void setTitle(const std::string&);
      void setContextMenu(std::string, std::string);
      void setSystemMenu(std::string);
      std::string openDialog(bool, bool, bool, std::string, std::string);
      int openExternal(std::string s);

      void createWebview();
  };

  App::App(void* h): hInstance((_In_ HINSTANCE) h) {
    // this fixes bad default quality DPI.
    ::SetProcessDPIAware();

    auto *szWindowClass = L"DesktopApp";
    auto *szTitle = L"Opkit";
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Window::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) {
      MessageBox(nullptr, nullptr, TEXT("Failed to start"), MB_OK | MB_ICONSTOP);
    }
  };

  void Window::createWebview () {}

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

    SetWindowPos(window, nullptr, 90000, 90000, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    SetWindowLongPtr(window, GWLP_USERDATA, (LONG_PTR) &webview);

    auto hr = CreateCoreWebView2EnvironmentWithOptions(
      nullptr,
      nullptr,
      nullptr,
      Microsoft::WRL::Callback<HEnv>(
        [=](HRESULT result, CoreEnv* env) -> HRESULT {

          env->CreateCoreWebView2Controller(
            window,
            Microsoft::WRL::Callback<HCon>(
              [=](HRESULT result, CoreCon* controller) -> HRESULT {
                hide();
                if (controller != nullptr) {
                  webviewController = controller;
                  webviewController->get_CoreWebView2(&webview);
                }

                webviewController->AddRef();

                ICoreWebView2Settings* Settings;
                webview->get_Settings(&Settings);
                Settings->put_IsScriptEnabled(TRUE);
                Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                Settings->put_IsWebMessageEnabled(TRUE);
                Settings->put_AreDevToolsEnabled(TRUE);
                Settings->put_IsZoomControlEnabled(FALSE);
      
                // Settings->put_IsBuiltInErrorPageEnabled(FALSE);

                RECT bounds;
                GetClientRect(window, &bounds);
                webviewController->put_Bounds(bounds);

                this->app.isReady = true;

                EventRegistrationToken token;
                webview->add_WebMessageReceived(
                  Microsoft::WRL::Callback<HRec>(
                    [=](ICoreWebView2* webview, IArgs * args) -> HRESULT {
                      PWSTR bytes;
                      args->TryGetWebMessageAsString(&bytes);

                      // HANDLE event_log = RegisterEventSource(NULL, L"OPKIT");
                      // ReportEvent(event_log, EVENTLOG_SUCCESS, 0, 0, NULL, 1, 0, (LPCWSTR* const) x.c_str(), NULL);

                      // Frameless loading mode.
                      // SetWindowLongPtr(window_primary, GWL_STYLE, WS_CAPTION);
                      // ShowWindow(window_primary, SW_SHOW);

                      // webview->PostWebMessageAsString(bytes);
                      CoTaskMemFree(bytes);
                      return S_OK;
                    }
                  ).Get(),
                  &token
                );

                return S_OK;
              }
            ).Get()
          );

          return S_OK;
        }
      ).Get()
    );

    if (!SUCCEEDED(hr)) {
      switch (hr) {
        case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): {
          MessageBox(
            window,
            TEXT(
              "Couldn't find Edge installation. "
              "Do you have a version installed that's compatible with this "
              "WebView2 SDK version?"
            ),
            nullptr,
            MB_OK
          );
          break;
        }

        case HRESULT_FROM_WIN32(ERROR_FILE_EXISTS): {
          MessageBox(
            window,
            TEXT("User data folder cannot be created because a file with the same name already exists."),
            nullptr,
            MB_OK
          );
          break;
        }

        case E_ACCESSDENIED: {
          MessageBox(
            window,
            TEXT("Unable to create user data folder, Access Denied."),
            nullptr,
            MB_OK
          );
          break;
        }

        case E_FAIL: {
          MessageBox(
            window,
            TEXT("Edge runtime unable to start"),
            nullptr,
            MB_OK
          );
          break;
        }

        default: {
          MessageBox(
            window,
            TEXT("Failed to create WebView2 environment"),
            nullptr,
            MB_OK
          );
          break;
        }
      }
    }
  }

  int App::run () {
    bool loop = GetMessage(&msg, nullptr, 0, 0) > 0;

    if (loop) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    return !loop;
  }

  void App::exit() {
    PostQuitMessage(WM_QUIT);
  }

  void App::dispatch(std::function<void()> cb) {
    auto r = std::async (std::launch::async, [&] {
      // wait for the webview to be ready before
      // responding to requests from the main process.
      while (!this->isReady) {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
      }
      cb();
    });
  }

  std::string App::getCwd(const std::string& _) {
    wchar_t filename[MAX_PATH];
    GetModuleFileNameW(NULL, filename, MAX_PATH);
    auto path = fs::path { filename }.remove_filename();
    return pathToString(path);
  }

  void Window::show () {
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    RECT r,r1;
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

  void Window::eval(const std::string& js) {
    if (webview == nullptr) {
      return;
    }

    webview->ExecuteScript(
      StringToWString(js).c_str(),
      nullptr
    );
  }

  void Window::navigate (const std::string& s) {
    show();
    webview->Navigate(StringToWString(s).c_str());
  }

  void Window::setTitle(const std::string& title) {}

  void Window::setSize(int width, int height) {}

  void Window::setSystemMenu(std::string) {
    // TODO implement
  }

  void Window::setContextMenu(std::string seq, std::string value) {
    // TODO implement
  }

  int Window::openExternal(std::string url) {
    return 0;
  }

  std::string Window::openDialog(bool isSave, bool allowDirs, bool allowFiles, std::string, std::string) {
    return std::string("");
  }

  void Window::resize() {
    RECT rc;
    GetClientRect(window, &rc);
    webviewController->put_Bounds(rc);
  }

  LRESULT CALLBACK Window::WndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam) {
    
    Window* w = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
      case WM_SIZE:
        // WM_SIZE will first fire before the webview finishes loading
        // init() calls resize(), so this call is only for size changes
        // after fully loading.
        if (w != nullptr && w->initDone) {
            w->resize();
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
      case WM_CLOSE:
          DestroyWindow(hwnd);
          break;
      case 666:
        alert("666");
        break;
      case WM_DESTROY:
          w->app.exit();
          break;
      default:
          return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    return 0;
  }
} // namespace Opkit