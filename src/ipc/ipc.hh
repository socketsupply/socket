#ifndef SSC_IPC_H
#define SSC_IPC_H

#include "../core/core.hh"

namespace SSC::IPC {
  class Router;
  class Bridge;
}

// create a proxy module so imports of the module of concern are imported
// exactly once at the canonical URL (file:///...) in contrast to module
// URLs (socket:...)

static constexpr auto moduleTemplate =
R"S(
import module from '{{url}}'
export * from '{{url}}'
export default module
)S";

#if defined(__APPLE__)
namespace SSC::IPC {
  using Task = id<WKURLSchemeTask>;
  using Tasks = std::map<String, Task>;
}

@class SSCBridgedWebView;
@interface SSCIPCSchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) SSC::IPC::Router* router;
-     (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (SSC::IPC::Task) task;
-    (void) webView: (SSCBridgedWebView*) webview
  stopURLSchemeTask: (SSC::IPC::Task) task;
@end

@interface SSCIPCSchemeTasks : NSObject {
  std::unique_ptr<SSC::IPC::Tasks> tasks;
  SSC::Mutex mutex;
}
- (SSC::IPC::Task) get: (SSC::String) id;
- (void) remove: (SSC::String) id;
- (bool) has: (SSC::String) id;
- (void) put: (SSC::String) id task: (SSC::IPC::Task) task;
@end

#if defined(__APPLE__)
@interface SSCIPCNetworkStatusObserver : NSObject
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property (nonatomic) SSC::IPC::Router* router;
@property (retain) nw_path_monitor_t monitor;
- (id) init;
@end
#endif
#endif

namespace SSC::IPC {
  struct MessageBuffer {
    size_t size = 0;
    char* bytes = nullptr;
    MessageBuffer(char* bytes, size_t size)
        : size(size), bytes(bytes) { }
  #ifdef _WIN32
    ICoreWebView2ExperimentalSharedBuffer* shared_buf = nullptr;
    MessageBuffer(ICoreWebView2ExperimentalSharedBuffer* buf, size_t size)
        : size(size), shared_buf(buf) {
      BYTE* b = reinterpret_cast<BYTE*>(bytes);
      HRESULT r = buf->get_Buffer(&b);
      if (r != S_OK) {
        // TODO(trevnorris): Handle this
      }
    }
  #endif
    MessageBuffer() = default;
  };

  class Message {
    public:
      using Seq = String;
      MessageBuffer buffer;
      String value = "";
      String name = "";
      String seq = "";
      String uri = "";
      int index = -1;
      Map args;

      Message () = default;
      Message (const Message& message);
      Message (const String& source);
      Message (const String& source, char *bytes, size_t size);
      bool has (const String& key) const;
      String get (const String& key) const;
      String get (const String& key, const String& fallback) const;
      String str () const { return this->uri; }
      const char * c_str () const { return this->uri.c_str(); }
  };

  class Result {
    public:
      class Err {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Err () = default;
          Err (const Message&, JSON::Any);
      };

      class Data {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Post post;

          Data () = default;
          Data (const Message&, JSON::Any);
          Data (const Message&, JSON::Any, Post);
      };

      Message message;
      Message::Seq seq;
      String source = "";
      JSON::Any value = nullptr;
      JSON::Any data = nullptr;
      JSON::Any err = nullptr;
      Post post;

      Result () = default;
      Result (const Err error);
      Result (const Data data);
      Result (const Message::Seq&, const Message&);
      Result (const Message::Seq&, const Message&, JSON::Any);
      Result (const Message::Seq&, const Message&, JSON::Any, Post);
      String str () const;
      JSON::Any json () const;
  };

  class Router {
    public:
      using EvaluateJavaScriptCallback = std::function<void(const String)>;
      using DispatchCallback = std::function<void()>;
      using ReplyCallback = std::function<void(const Result&)>;
      using ResultCallback = std::function<void(Result)>;
      using MessageCallback = std::function<void(const Message, Router*, ReplyCallback)>;
      using BufferMap = std::map<String, MessageBuffer>;

      struct MessageCallbackContext {
        bool async = true;
        MessageCallback callback;
      };

      using Table = std::map<String, MessageCallbackContext>;

      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      std::function<void(DispatchCallback)> dispatchFunction = nullptr;
      BufferMap buffers;
      bool isReady = false;
      Mutex mutex;
      Table table;
      Core *core = nullptr;
      Bridge *bridge = nullptr;
#if defined(__APPLE__)
      SSCIPCNetworkStatusObserver* networkStatusObserver = nullptr;
      SSCIPCSchemeHandler* schemeHandler = nullptr;
      SSCIPCSchemeTasks* schemeTasks = nullptr;
#endif

      Router ();
      Router (const Router &) = delete;
      ~Router ();

      MessageBuffer getMappedBuffer (int index, const Message::Seq seq);
      bool hasMappedBuffer (int index, const Message::Seq seq);
      void removeMappedBuffer (int index, const Message::Seq seq);
      void setMappedBuffer(int index, const Message::Seq seq, MessageBuffer msg_buf);

      void map (const String& name, MessageCallback callback);
      void map (const String& name, bool async, MessageCallback callback);
      void unmap (const String& name);
      bool dispatch (DispatchCallback callback);
      bool emit (const String& name, const String& data);
      bool evaluateJavaScript (const String javaScript);
      bool send (const Message::Seq& seq, const String& data, const Post post);
      bool invoke (const String& msg, ResultCallback callback);
      bool invoke (const String& msg, const char *bytes, size_t size);
      bool invoke (
        const String& msg,
        const char *bytes,
        size_t size,
        ResultCallback callback
      );
  };

  class Bridge {
    public:
      Router router;
      Bluetooth bluetooth;
      Core *core = nullptr;

      Bridge (Core *core);
      bool route (const String& msg, const char *bytes, size_t size);
      bool route (
        const String& msg,
        const char *bytes,
        size_t size,
        Router::ResultCallback
      );
  };

  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }
} // SSC::IPC
#endif
