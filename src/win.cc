#define UNICODE
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"shell32.lib")
#pragma comment(lib,"version.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"WebView2LoaderStatic.lib")
#include "win64/WebView2.h"
#include <tchar.h>
#include <wrl.h>

// https://go.microsoft.com/fwlink/p/?LinkId=2124703

#include <windows.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <algorithm>

std::wstring ExePath() {
  TCHAR buffer[MAX_PATH] = { 0 };
  DWORD len = GetModuleFileName(NULL, buffer, MAX_PATH);
  std::wstring ws;
  ws.assign(&buffer[0], &buffer[len]);
  std::wstring::size_type pos = ws.find_last_of(L"\\/");
  return ws.substr(0, pos);
}
 
static ICoreWebView2Controller *m_webviewController;
static ICoreWebView2 *m_webview;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int CALLBACK WinMain(
  _In_ HINSTANCE hInstance,
  _In_ HINSTANCE hPrevInstance,
  _In_ LPSTR lpCmdLine,
  _In_ int nCmdShow)
{
  auto *szWindowClass = L"DesktopApp";
  auto *szTitle = L"TEST";
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
  wcex.lpszClassName = szWindowClass;
  wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

  if (!RegisterClassEx(&wcex)) {
    MessageBox(nullptr, nullptr, TEXT("SHIT!"), MB_OK | MB_ICONSTOP);
    return 1;
  }

  HWND hWnd = CreateWindow(
    szWindowClass,
    szTitle,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT, CW_USEDEFAULT,
    1200, 900,
    NULL,
    NULL,
    hInstance,
    NULL);

  if (!hWnd) {
    MessageBox(nullptr, nullptr, TEXT("CRAP!"), MB_OK | MB_ICONSTOP);
    return 1;
  }

  ShowWindow(hWnd, nCmdShow);
  UpdateWindow(hWnd);

  auto hr = CreateCoreWebView2EnvironmentWithOptions(
    nullptr,
    nullptr,
    nullptr,
    Microsoft::WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
      [=](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

        env->CreateCoreWebView2Controller(
          hWnd,
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
            GetClientRect(hWnd, &bounds);
            m_webviewController->put_Bounds(bounds);

            EventRegistrationToken token;

            m_webview->add_WebMessageReceived(
              Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [=](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs * args) -> HRESULT {
                  PWSTR bytes;
                  args->TryGetWebMessageAsString(&bytes);

                  std::wstringstream ss;
                  ss << bytes;

                  // HANDLE event_log = RegisterEventSource(NULL, L"OPKIT");
                  // ReportEvent(event_log, EVENTLOG_SUCCESS, 0, 0, NULL, 1, 0, (LPCWSTR* const) x.c_str(), NULL);

                  MessageBox(
                    hWnd,
                    (LPCWSTR const) ss.str().c_str(),
                    nullptr,
                    MB_OK
                  );

                  // Frameless loading mode.
                  SetWindowLongPtr(hWnd, GWL_STYLE, WS_CAPTION);
                  ShowWindow(hWnd, SW_SHOW);

                  // webview->PostWebMessageAsString(bytes);
                  CoTaskMemFree(bytes);
                  return S_OK;
                }
              ).Get(),
              &token
            );

            std::wstringstream url;
            url << "file://" << ExePath() << "/index.html";
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
          hWnd,
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
          hWnd,
          TEXT("User data folder cannot be created because a file with the same name already exists."),
          nullptr,
          MB_OK
        );
        break;
      }

      case E_ACCESSDENIED: {
        MessageBox(
          hWnd,
          TEXT("Unable to create user data folder, Access Denied."),
          nullptr,
          MB_OK
        );
        break;
      }

      case E_FAIL: {
        MessageBox(
          hWnd,
          TEXT("Edge runtime unable to start"),
          nullptr,
          MB_OK
        );
        break;
      }

      default: {
        MessageBox(
          hWnd,
          TEXT("Failed to create WebView2 environment"),
          nullptr,
          MB_OK
        );
      }
    }
  }

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
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