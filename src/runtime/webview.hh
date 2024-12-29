#ifndef SOCKET_RUNTIME_WEBVIEW_H
#define SOCKET_RUNTIME_WEBVIEW_H

#include "webview/scheme_handlers.hh"
#include "webview/navigator.hh"
#include "webview/preload.hh"
#include "webview/webview.hh"
#include "webview/client.hh"
#include "ipc.hh"

namespace ssc::runtime::webview {
  class IBridge : public ipc::IBridge {
    public:
      Map<String, String> userConfig;
      SchemeHandlers schemeHandlers;
      Navigator navigator;

      IBridge (
        context::Dispatcher& dispatcher,
        context::RuntimeContext& context,
        const Client& client,
        Map<String, String> userConfig
      )
        : ipc::IBridge(dispatcher, context, client),
          schemeHandlers(this),
          userConfig(userConfig),
          navigator(this)
      {}

      virtual bool navigate (const String& url) = 0;
  };
}
#endif
