#define UNICODE
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"WebView2LoaderStatic.lib")

#include <tchar.h>
#include <wrl.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <functional>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <stdio.h>

#include "util.h"
#include "win64/WebView2.h"

namespace fs = std::filesystem;

std::string getCwd (const std::string);

inline std::wstring getExec() {
  wchar_t filename[MAX_PATH];
  GetModuleFileNameW(NULL, filename, MAX_PATH);
  return filename;
}

inline void alert (const std::wstring &ws) {
  MessageBoxA(nullptr, Str(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

inline void alert (const std::string &s) {
  MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
}

std::string createNativeDialog(
  int flags,
  const char *_,
  const char *default_path,
  const char *default_name);

static ICoreWebView2Controller *m_webviewController;
static ICoreWebView2 *m_webview;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

namespace Opkit {
  class edge_engine {
    public:

    edge_engine(bool debug, void *hInstance)
      : hInstance((_In_ HINSTANCE) hInstance) {
        
      ::SetProcessDPIAware();
      createWindow();
    }

    void createWindow () {
      auto *szWindowClass = L"DesktopApp";
      auto *szTitle = L"Opkit";
      WNDCLASSEX wcex;

      wcex.cbSize = sizeof(WNDCLASSEX);
      wcex.style = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc = WndProc;
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
        MessageBox(nullptr, nullptr, TEXT("Unable to register window"), MB_OK | MB_ICONSTOP);
        return;
      }

      m_window = CreateWindow(
        TEXT("DesktopApp"),
        TEXT("Opkit"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1200, 900,
        NULL,
        NULL,
        hInstance,
        NULL
      );

      if (!m_window) {
        MessageBox(nullptr, nullptr, TEXT("Unable to create window!"), MB_OK | MB_ICONSTOP);
        return;
      }

      ShowWindow(m_window, SW_SHOW);
      UpdateWindow(m_window);

      createWebview();
    }

    void createWebview () {
      auto hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        nullptr,
        nullptr,
        Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [=](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

            env->CreateCoreWebView2Controller(
              m_window,
              Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
              [=](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                if (controller != nullptr) {
                  m_webviewController = controller;
                  m_webviewController->get_CoreWebView2(&m_webview);
                }

                m_webviewController->AddRef();

                ICoreWebView2Settings* Settings;
                m_webview->get_Settings(&Settings);
                Settings->put_IsScriptEnabled(TRUE);
                Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                Settings->put_IsWebMessageEnabled(TRUE);
                Settings->put_AreDevToolsEnabled(TRUE);
                Settings->put_IsZoomControlEnabled(FALSE);
      
                // Settings->put_IsBuiltInErrorPageEnabled(FALSE);

                RECT bounds;
                GetClientRect(m_window, &bounds);
                m_webviewController->put_Bounds(bounds);

                EventRegistrationToken token;

                m_webview->add_WebMessageReceived(
                  Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                    [=](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
                      PWSTR bytes;
                      args->TryGetWebMessageAsString(&bytes);

                      // HANDLE event_log = RegisterEventSource(NULL, L"OPKIT");
                      // ReportEvent(event_log, EVENTLOG_SUCCESS, 0, 0, NULL, 1, 0, (LPCWSTR* const) x.c_str(), NULL);

                      // Frameless loading mode.
                      SetWindowLongPtr(m_window, GWL_STYLE, WS_CAPTION);
                      ShowWindow(m_window, SW_SHOW);

                      // webview->PostWebMessageAsString(bytes);
                      CoTaskMemFree(bytes);
                      return S_OK;
                    }
                  ).Get(),
                  &token
                );

                fs::path wd = getExec();

                fs::path cwd {
                  fs::path { getExec() }.remove_filename() /
                  "index.html"
                };

                std::wstringstream url;
                url << "file://" << Str(pathToString(cwd));

                alert(url.str());
                m_webview->Navigate(url.str().c_str());

                return S_OK;
              }).Get()
            );

            return S_OK;
          }
        ).Get()
      );

      if (!SUCCEEDED(hr)) {
        switch (hr) {
          case HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND): {
            MessageBox(
              m_window,
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
              m_window,
              TEXT("User data folder cannot be created because a file with the same name already exists."),
              nullptr,
              MB_OK
            );
            break;
          }

          case E_ACCESSDENIED: {
            MessageBox(
              m_window,
              TEXT("Unable to create user data folder, Access Denied."),
              nullptr,
              MB_OK
            );
            break;
          }

          case E_FAIL: {
            MessageBox(
              m_window,
              TEXT("Edge runtime unable to start"),
              nullptr,
              MB_OK
            );
            break;
          }

          default: {
            MessageBox(
              m_window,
              TEXT("Failed to create WebView2 environment"),
              nullptr,
              MB_OK
            );
          }
        }
      }      
    }

    void createContextMenu(std::string seq, std::string menuData) {}

    void about () {}

    void show () {
      ShowWindow(m_window, SW_SHOW);
      UpdateWindow(m_window);
    }

    void hide () {
      ShowWindow(m_window, SW_HIDE);
      UpdateWindow(m_window);
    }

    void menu(std::string menu) {}

    void *window() { return (void*) m_window; }

    void run() {
      while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }

    void inspect() {}

    void terminate() {}

    void dispatch(std::function<void()> f) {}

    void setTitle(const std::string title) {}

    void setSize(int width, int height, int hints) {}

    int openExternal(std::string url) {
      return 0;
    }
    void navigate(const std::string url) {}

    void init(const std::string js) {}

    void eval(const std::string js) {}

    _In_ HINSTANCE hInstance;
    HWND m_window;

    private:
      MSG msg;
      virtual void on_message(const std::string msg) = 0;

      static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
          case WM_SIZE: {
            if (m_webviewController != nullptr) {
              RECT bounds;
              GetClientRect(hWnd, &bounds);
              m_webviewController->put_Bounds(bounds);
            }
            break;
          }

          case WM_DESTROY: {
            PostQuitMessage(0);
            break;
          }

          default: {
            return DefWindowProc(hWnd, message, wParam, lParam);
            break;
          }
        }

        return 0;
      }
  };

  using browser_engine = edge_engine;
} // namespace Opkit