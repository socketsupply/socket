#include "../javascript.hh"
#include "../config.hh"
#include "../window.hh"
#include "../string.hh"
#include "../cwd.hh"
#include "../app.hh"

using ssc::runtime::javascript::getResolveMenuSelectionJavaScript;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::trim;
using ssc::runtime::string::split;

namespace ssc::runtime::app {
  static Atomic<bool> isConsoleVisible = false;
  static FILE* console = nullptr;

  static inline void alert (const WString &ws) {
    MessageBoxA(nullptr, convertWStringToString(ws).c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const String &s) {
    MessageBoxA(nullptr, s.c_str(), _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static inline void alert (const char* s) {
    MessageBoxA(nullptr, s, _TEXT("Alert"), MB_OK | MB_ICONSTOP);
  }

  static void showWindowsConsole () {
    if (!isConsoleVisible) {
      isConsoleVisible = true;
      AllocConsole();
      freopen_s(&console, "CONOUT$", "w", stdout);
    }
  }

  static void hideWindowsConsole () {
    if (isConsoleVisible) {
      isConsoleVisible = false;
      fclose(console);
      FreeConsole();
    }
  }

  // message is defined in WinUser.h
  // https://raw.githubusercontent.com/tpn/winsdk-10/master/Include/10.0.10240.0/um/WinUser.h
  LRESULT CALLBACK onWindowProcMessage (
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
  ) {
    auto app = App::sharedApplication();

    if (app == nullptr) {
      return 0;
    }

    auto window = reinterpret_cast<window::Manager::ManagedWindow*>(
      GetWindowLongPtr(hWnd, GWLP_USERDATA)
    );

    // invalidate `window` pointer that potentially is leaked
    if (window != nullptr && app->runtime.windowManager.getWindow(window->index).get() != window) {
      window = nullptr;
    }

    auto userConfig = window != nullptr
      ? dynamic_cast<Window*>(window)->bridge.userConfig
      : getUserConfig();

    if (message == WM_COPYDATA) {
      auto copyData = reinterpret_cast<PCOPYDATASTRUCT>(lParam);
      message = (UINT) copyData->dwData;
      wParam = (WPARAM) copyData->cbData;
      lParam = (LPARAM) copyData->lpData;
    }

    switch (message) {
      case WM_SIZE: {
        if (window == nullptr || window->controller == nullptr) {
          break;
        }

        RECT bounds;
        GetClientRect(hWnd, &bounds);
        window->size.height = bounds.bottom - bounds.top;
        window->size.width = bounds.right - bounds.left;
        window->controller->put_Bounds(bounds);
        break;
      }

      case WM_SOCKET_TRAY: {
        // XXX(@jwerle, @heapwolf): is this a correct for an `isAgent` predicate?
        auto isAgent = userConfig.count("tray_icon") != 0;

        if (window != nullptr && lParam == WM_LBUTTONDOWN) {
          SetForegroundWindow(hWnd);
          if (isAgent) {
            POINT point;
            GetCursorPos(&point);
            TrackPopupMenu(
              window->menutray,
              TPM_BOTTOMALIGN | TPM_LEFTALIGN,
              point.x,
              point.y,
              0,
              hWnd,
              NULL
            );
          }

          PostMessage(hWnd, WM_NULL, 0, 0);

          // broadcast an event to all the windows that the tray icon was clicked
          for (auto window : app->runtime.windowManager.windows) {
            if (window != nullptr) {
              window->bridge.emit("tray", JSON::Object {});
            }
          }
        }

        // XXX: falls through to `WM_COMMAND` below
      }

      case WM_COMMAND: {
        if (window == nullptr) {
          break;
        }

        if (window->menuMap.contains(wParam)) {
          String meta(window->menuMap[wParam]);
          auto parts = split(meta, '\t');

          if (parts.size() > 1) {
            auto title = parts[0];
            auto parent = parts[1];

            if (title.find("About") == 0) {
              dynamic_cast<Window*>(window)->about();
              break;
            }

            if (title.find("Quit") == 0) {
              window->exit(0);
              break;
            }

            window->eval(getResolveMenuSelectionJavaScript("0", title, parent, "system"));
          }
        } else if (window->menuTrayMap.contains(wParam)) {
          String meta(window->menuTrayMap[wParam]);
          auto parts = split(meta, ':');

          if (parts.size() > 0) {
            auto title = trim(parts[0]);
            auto tag = parts.size() > 1 ? trim(parts[1]) : "";
            window->eval(getResolveMenuSelectionJavaScript("0", title, tag, "tray"));
          }
        }

        break;
      }

      case WM_SETTINGCHANGE: {
        // TODO(heapwolf): Dark mode
        break;
      }

      case WM_CREATE: {
        // TODO(heapwolf): Dark mode
        SetWindowTheme(hWnd, L"Explorer", NULL);
        SetMenu(hWnd, CreateMenu());
        break;
      }

      case WM_CLOSE: {
        if (!window || !window->options.closable) {
          break;
        }

        auto index = window->index;
        const JSON::Object json = JSON::Object::Entries {
          {"data", index}
        };

        for (auto window : app->runtime.windowManager.windows) {
          if (window != nullptr && window->index != index) {
            window->eval(getEmitToRenderProcessJavaScript("windowclosed", json.str()));
          }
        }

        app->runtime.windowManager.destroyWindow(index);
        break;
      }

      case WM_HOTKEY: {
        if (window != nullptr) {
          window->hotkey.onHotKeyBindingCallback((HotKeyBinding::ID) wParam);
        }
        break;
      }

      case WM_HANDLE_DEEP_LINK: {
        const auto url = String(reinterpret_cast<const char*>(lParam), wParam);

        for (auto window : app->runtime.windowManager.windows) {
          if (window != nullptr) {
            window->handleApplicationURL(url);
          }
        }
        break;
      }

      case WM_GETMINMAXINFO: {
        const auto screen = window::Window::getScreenSize();
        auto info = reinterpret_cast<LPMINMAXINFO>(lParam);

        info->ptMinTrackSize.x = window::Window::getSizeInPixels(
          app->runtime.windowManager.options.defaultMinWidth,
          screen.width
        );

        info->ptMinTrackSize.y = window::Window::getSizeInPixels(
          app->runtime.windowManager.options.defaultMinHeight,
          screen.height
        );

        info->ptMaxTrackSize.x = window::Window::getSizeInPixels(
          app->runtime.windowManager.options.defaultMaxWidth,
          screen.width
        );

        info->ptMaxTrackSize.y = window::Window::getSizeInPixels(
          app->runtime.windowManager.options.defaultMaxHeight,
          screen.height
        );
        break;
      }

      //case WM_WINDOWPOSCHANGING: { break; }

      default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
  }

  void registerWindowClass (App* app) {
    auto userconfig = app->runtime.userConfig;
    // this fixes bad default quality DPI.
    SetProcessDPIAware();

    if (userConfig["win_logo"].size() == 0 && userConfig["win_icon"].size() > 0) {
      userConfig["win_logo"] = fs::path(userConfig["win_icon"]).filename().string();
    }

    auto iconPath = fs::path { getcwd() / fs::path { userConfig["win_logo"] } };

    HICON icon = (HICON) LoadImageA(
      NULL,
      iconPath.string().c_str(),
      IMAGE_ICON,
      GetSystemMetrics(SM_CXICON),
      GetSystemMetrics(SM_CXICON),
      LR_LOADFROMFILE
    );

    auto windowClassName = userConfig["meta_bundle_identifier"];

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = app->instance;
    wcex.hIcon = LoadIcon(app->instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = windowClassName.c_str();
    wcex.hIconSm = icon; // ico doesn't auto scale, needs 16x16 icon lol fuck you bill
    wcex.hIcon = icon;
    wcex.lpfnWndProc = onWindowProcMessage;

    if (!RegisterClassEx(&wcex)) {
      alert("Application could not launch, possible missing resources.");
    }
  }
}
