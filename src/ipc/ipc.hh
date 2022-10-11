#ifndef SSC_IPC_H
#define SSC_IPC_H

#include "../core/core.hh"
#include "json.hh"

namespace SSC::IPC {
  class Bridge;
  class Router;

 /**
  * IPC message parser.
  * Parses URIs like: `ipc://id?p1=v1&p2=v2&...\0`
  * @TODO possibly harden data validation
  */
  class Message {
    Map args;
    public:
      using Seq = String;
      struct MessageBody {
        char *bytes = nullptr;
        unsigned int size = 0;
      };

      MessageBody body;
      String value = "";
      String name = "";
      String seq = "";
      String uri = "";
      int index = -1;

      Message (const String& source);
      Message (const String& source, char *bytes, size_t size);
      String get (const String& key) const;
      String get (const String& key, const String& fallback) const;
      inline const char * c_str () const {
        return this->uri.c_str();
      }
  };

  class Router {
    public:
      using EvaluateJavaScriptCallback = std::function<void(const String)>;
      using DispatchCallback = std::function<void()>;
      using MessageCallback = std::function<void(const Message, Router* router)>;
      using Table = std::map<String, MessageCallback>;

      struct Implementation {
        EvaluateJavaScriptCallback evaluateJavaScript = nullptr;
        std::function<void(DispatchCallback)> dispatch = nullptr;
      };

      Implementation implementation;
      Table table;
      Core *core = nullptr;

      Router () {}
      Router (Core *core);

      void bind (const String& name, MessageCallback callback);
      bool dispatch (DispatchCallback callback);
      bool emit (const String &name, const String& data);
      bool evaluateJavaScript (const String javaScript);
      bool invoke (const String& name, const Message& message);
      bool push (const JSON::Any data);
      bool push (const String& data);
      bool push (const Post &post);
      bool push (const JSON::Any data, const Post &post);
      bool push (const String& data, const Post &post);
      bool reply (const Message& message, const JSON::Any data, const Post &post);
      bool reply (const Message& message, const JSON::Any data);
      bool reply (const Message& message, const String& data);
      bool reply (const Message& message, const String& data, const Post &post);
      bool route (const String msg);
      bool route (const String msg, char *bytes, size_t size);
      bool route (const Message& message);
      bool send (const Message::Seq& seq, const JSON::Any data);
      bool send (const Message::Seq& seq, const JSON::Any data, const Post& post);
      bool send (const Message::Seq& seq, const String& data);
      bool send (const Message::Seq& seq, const String& data, const Post& post);
      void setImplementation (Implementation implementation);
      void unbind (const String& name);
  };

  class Bridge {
    public:
      Router router;
      Core *core = nullptr;

      Bridge () {}
      Bridge (Core *core);
  };
} // SSC::IPC

#if defined(__APPLE__)
@class SSCBridgedWebView;
@class SSCBluetoothDelegate;

@interface SSCIPCBridge : NSObject
@property (strong, nonatomic) NSObject<OS_dispatch_queue>* monitorQueue;
@property (strong, nonatomic) SSCBridgedWebView* webview;
@property (strong, nonatomic) SSCBluetoothDelegate* bluetooth;
@property (nonatomic) SSC::Core* core;
@property (nonatomic) SSC::IPC::Router* router; // internal
@property (assign) nw_path_monitor_t monitor;
- (bool) route: (SSC::String) msg
           buf: (char*) buf
       bufsize: (size_t) bufsize;

- (void) emit: (SSC::String) name
          msg: (SSC::String) msg;

- (void) send: (SSC::IPC::Message::Seq) seq
          msg: (SSC::String) msg
         post: (SSC::Post) post;

- (void) initNetworkStatusObserver;
- (void) setWebview: (SSCBridgedWebView*) webview;
- (void) setCore: (SSC::Core*) core;
- (void) dispatch: (void (^)(void)) callback;
@end

@interface SSCIPCBridgeSchemeHandler : NSObject<WKURLSchemeHandler>
@property (strong, nonatomic) SSCIPCBridge* bridge;
- (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (id <WKURLSchemeTask>) task;

- (void) webView: (SSCBridgedWebView*) webview
  stopURLSchemeTask: (id <WKURLSchemeTask>) task;
@end
#endif

#endif
