#ifndef SSC_IPC_H
#define SSC_IPC_H

#include "../core/core.hh"

namespace SSC::IPC {
  class Bridge;
  class Router;

 /**
  * IPC message parser.
  * Parses URIs like: `ipc://id?p1=v1&p2=v2&...\0`
  * @TODO possibly harden data validation
  */
  class Message {
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
      Map args;

      Message () = default;
      Message (const Message& message);
      Message (const String& source);
      Message (const String& source, char *bytes, size_t size);
      String get (const String& key) const;
      String get (const String& key, const String& fallback) const;
      String str () const { return this->uri; }
      const char * c_str () const { return this->uri.c_str(); }
  };

  class Result {
    public:
      using Data = JSON::Any;
      using Err = JSON::Any;

      Message message;
      Message::Seq seq;
      String source = "";
      JSON::Any json = nullptr;
      Post post;
      Data data = nullptr;
      Err err = nullptr;

      Result ();
      Result (Message::Seq, const Message&);
      Result (Message::Seq, const Message&, JSON::Any);
      Result (Message::Seq, const Message&, JSON::Any, Post);
      String str () const;
  };

  class Router {
    public:
      using EvaluateJavaScriptCallback = std::function<void(const String)>;
      using DispatchCallback = std::function<void()>;
      using ReplyCallback = std::function<void(const Result&)>;
      using ResultCallback = std::function<void(Result)>;
      using MessageCallback = std::function<void(const Message, Router*, ReplyCallback)>;
      using Table = std::map<String, MessageCallback>;

      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      std::function<void(DispatchCallback)> dispatchFunction = nullptr;
      Table table;
      Core *core = nullptr;

      Router ();
      Router (Core *core);

      void map (const String& name, MessageCallback callback);
      void unmap (const String& name);
      bool dispatch (DispatchCallback callback);
      bool emit (const String &name, const String& data);
      bool evaluateJavaScript (const String javaScript);
      bool invoke (const Message& message);
      bool invoke (const Message& message, ResultCallback callback);
      bool invoke (const String msg, char *bytes, size_t size);
      bool invoke (const String msg, char *bytes, size_t size, ResultCallback callback);
      bool send (const Message::Seq& seq, const String& data);
      bool send (const Message::Seq& seq, const String& data, const Post& post);
  };

  class Bridge {
    public:
      Router router;
      Core *core = nullptr;

      Bridge () {}
      Bridge (Core *core);
      bool route (const String msg, char *bytes, size_t size);
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
@property (nonatomic) SSC::IPC::Router* router;
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
