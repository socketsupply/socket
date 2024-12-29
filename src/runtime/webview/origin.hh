#ifndef SOCKET_RUNTIME_WEBVIEW_ORIGIN_H
#define SOCKET_RUNTIME_WEBVIEW_ORIGIN_H

#include "../platform.hh"

namespace ssc::runtime::webview {
  struct Origin {
    using ID = uint64_t;
    String name;
  };
}
#endif
