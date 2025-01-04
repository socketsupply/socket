#ifndef SOCKET_RUNTIME_DESKTOP_EXTENSION_H
#define SOCKET_RUNTIME_DESKTOP_EXTENSION_H

#include "../runtime/platform.hh"

namespace ssc::desktop {
  struct WebExtensionContext {
    struct ConfigData {
      char* bytes = nullptr;
      size_t size = 0;
    };

    ConfigData config;
  };
}

#endif
