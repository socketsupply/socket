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

std::string createNativeDialog(
  int flags,
  const char *_,
  const char *default_path,
  const char *default_name);

namespace Opkit {
  class edge_engine {
    public:

    edge_engine(bool debug, void *window)
      : m_window((_In_ HINSTANCE) window) {
    }

    void createContextMenu(std::string seq, std::string menuData) {}

    void about () {}

    void show () {}

    void hide () {}

    void menu(std::string menu) {}

    void *window() { return (void*) m_window; }

    void run() {}

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

    _In_ HINSTANCE m_window;

    private:
      virtual void on_message(const std::string msg) = 0;
  };

  using browser_engine = edge_engine;
} // namespace Opkit