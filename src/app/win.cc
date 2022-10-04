#include "../window/window.hh"
#include "../window/factory.hh"
#include "app.hh"

#include <uxtheme.h>
#pragma comment(lib, "UxTheme.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdi32.lib")

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")
#pragma comment(lib, "uv_a.lib")

namespace SSC {
  inline void alert (const SSC::WString &ws) {
    MessageBoxA(nullptr, SSC::WStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  inline void alert (const SSC::String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  std::atomic<bool> App::isReady {false};

  App::App (void* h): hInstance((_In_ HINSTANCE) h) {
    #if DEBUG == 1
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    #endif

    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);

    SSC::setWindowCompositionAttribute = reinterpret_cast<SSC::SetWindowCompositionAttribute>(GetProcAddress(
      GetModuleHandleW(L"user32.dll"),
      "SetWindowCompositionAttribute")
    );

    if (hUxtheme) {
      SSC::refreshImmersiveColorPolicyState =
        (SSC::RefreshImmersiveColorPolicyState) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104));

      SSC::shouldSystemUseDarkMode =
        (SSC::ShouldSystemUseDarkMode) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(138));

      SSC::allowDarkModeForApp =
        (SSC::AllowDarkModeForApp) GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
    }

    allowDarkModeForApp(shouldSystemUseDarkMode());
    refreshImmersiveColorPolicyState();

    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    auto iconPath = fs::path { getCwd("") / fs::path { "index.ico" } };

    HICON icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CXSMICON),
      LR_LOADFROMFILE
    );

    auto *szWindowClass = L"DesktopApp";
    auto *szTitle = L"Socket SDK";

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = bgBrush;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = TEXT("DesktopApp");
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    wcex.lpfnWndProc = Window::WndProc;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  };
}
