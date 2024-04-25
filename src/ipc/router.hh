#ifndef SSC_IPC_ROUTER_H
#define SSC_IPC_ROUTER_H

#include "../core/core.hh"
#include "scheme_handlers.hh"
#include "message.hh"
#include "result.hh"

namespace SSC::IPC {
  class Bridge;
  class Router {
    public:
      using EvaluateJavaScriptCallback = std::function<void(const String)>;
      using DispatchCallback = std::function<void()>;
      using ReplyCallback = std::function<void(const Result&)>;
      using ResultCallback = std::function<void(Result)>;
      using MessageCallback = std::function<void(const Message&, Router*, ReplyCallback)>;
      using BufferMap = std::map<String, MessageBuffer>;

      struct Location {
        String href;
        String pathname;
        String query;
        Map workers;
      };

      struct MessageCallbackContext {
        bool async = true;
        MessageCallback callback;
      };

      struct MessageCallbackListenerContext {
        uint64_t token;
        MessageCallback callback;
      };

      using Table = std::map<String, MessageCallbackContext>;
      using Listeners = std::map<String, std::vector<MessageCallbackListenerContext>>;

      struct WebViewURLPathResolution {
        String pathname = "";
        bool redirect = false;
      };

      struct WebViewNavigatorMount {
        WebViewURLPathResolution resolution; // absolute URL resolution
        String path; // root path on host file system
        String route; // root path in webview navigator
      };

      struct WebViewURLComponents {
        String originalURL;
        String scheme = "";
        String authority = "";
        String pathname;
        String query;
        String fragment;
      };

      static WebViewURLComponents parseURLComponents (const String& url);
      static WebViewURLPathResolution resolveURLPathForWebView (String inputPath, const String& basePath);
      static WebViewNavigatorMount resolveNavigatorMountForWebView (const String& path);

    #if defined(__APPLE__)
      static Mutex notificationMapMutex;
      static std::map<String, Router*> notificationMap;
    #endif

    private:
      Table preserved;

    public:
      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      Function<void(DispatchCallback)> dispatchFunction = nullptr;

      Listeners listeners;
      BufferMap buffers;
      Mutex mutex;
      Table table;

      Location location;
      SchemeHandlers schemeHandlers;

      Core *core = nullptr;
      Bridge *bridge = nullptr;

      Router ();
      Router (const Router &) = delete;
      ~Router ();

      void init ();
      void init (Bridge* bridge);
      void configureHandlers (const SchemeHandlers::Configuration& configuration);

      MessageBuffer getMappedBuffer (int index, const Message::Seq seq);
      bool hasMappedBuffer (int index, const Message::Seq seq);
      void removeMappedBuffer (int index, const Message::Seq seq);
      void setMappedBuffer(int index, const Message::Seq seq, MessageBuffer msg_buf);

      void preserveCurrentTable ();

      uint64_t listen (const String& name, MessageCallback callback);
      bool unlisten (const String& name, uint64_t token);
      void map (const String& name, MessageCallback callback);
      void map (const String& name, bool async, MessageCallback callback);
      void unmap (const String& name);
      bool dispatch (DispatchCallback callback);
      bool emit (const String& name, const String data);
      bool evaluateJavaScript (const String javaScript);
      bool send (const Message::Seq& seq, const String data, const Post post);
      bool invoke (const String& msg, ResultCallback callback);
      bool invoke (const String& msg, const char *bytes, size_t size);
      bool invoke (
        const String& msg,
        const char *bytes,
        size_t size,
        ResultCallback callback
      );
      bool invoke (
        const Message& msg,
        const char *bytes,
        size_t size,
        ResultCallback callback
      );
  };
}
#endif
