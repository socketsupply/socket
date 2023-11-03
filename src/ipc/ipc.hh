#ifndef SSC_IPC_H
#define SSC_IPC_H

#include "../core/core.hh"

// only available on desktop
#if !defined(__ANDROID__) && (defined(_WIN32) || defined(__linux__) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR))
#include "../core/file_system_watcher.hh"
#endif

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

@interface SSCIPCNetworkStatusObserver : NSObject
@property (nonatomic, assign) dispatch_queue_t monitorQueue;
@property (nonatomic, assign) SSC::IPC::Router* router;
@property (nonatomic, assign) nw_path_monitor_t monitor;
- (id) init;
- (void) start;
@end

@class SSCLocationObserver;
@interface SSCLocationManagerDelegate : NSObject<CLLocationManagerDelegate>
@property (nonatomic, strong) SSCLocationObserver* locationObserver;

- (id) initWithLocationObserver: (SSCLocationObserver*) locationObserver;

- (void) locationManager: (CLLocationManager*) locationManager
        didFailWithError: (NSError*) error;

- (void) locationManager: (CLLocationManager*) locationManager
      didUpdateLocations: (NSArray<CLLocation*>*) locations;

- (void)            locationManager: (CLLocationManager*) locationManager
  didFinishDeferredUpdatesWithError: (NSError*) error;

- (void) locationManagerDidPauseLocationUpdates: (CLLocationManager*) locationManager;
- (void) locationManagerDidResumeLocationUpdates: (CLLocationManager*) locationManager;
- (void) locationManager: (CLLocationManager*) locationManager
                didVisit: (CLVisit*) visit;

-       (void) locationManager: (CLLocationManager*) locationManager
  didChangeAuthorizationStatus: (CLAuthorizationStatus) status;
- (void) locationManagerDidChangeAuthorization: (CLLocationManager*) locationManager;
@end

@interface SSCLocationPositionWatcher : NSObject
@property (nonatomic, assign) NSInteger identifier;
@property (nonatomic, assign) void(^completion)(CLLocation*);
+ (SSCLocationPositionWatcher*) positionWatcherWithIdentifier: (NSInteger) identifier
                                                   completion: (void (^)(CLLocation*)) completion;
@end

@interface SSCLocationObserver : NSObject
@property (nonatomic, retain) CLLocationManager* locationManager;
@property (nonatomic, retain) SSCLocationManagerDelegate* delegate;
@property (atomic, retain) NSMutableArray* activationCompletions;
@property (atomic, retain) NSMutableArray* locationRequestCompletions;
@property (atomic, retain) NSMutableArray* locationWatchers;
@property (nonatomic) SSC::IPC::Router* router;
@property (atomic, assign) BOOL isAuthorized;
- (BOOL) attemptActivation;
- (BOOL) attemptActivationWithCompletion: (void (^)(BOOL)) completion;
- (BOOL) getCurrentPositionWithCompletion: (void (^)(NSError*, CLLocation*)) completion;
- (int) watchPositionForIdentifier: (NSInteger) identifier
                        completion: (void (^)(NSError*, CLLocation*)) completion;
- (BOOL) clearWatch: (NSInteger) identifier;
@end

@interface SSCUserNotificationCenterDelegate : NSObject<UNUserNotificationCenterDelegate>
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler;

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler;
@end
#endif

namespace SSC::IPC {
  struct MessageBuffer {
    size_t size = 0;
    char* bytes = nullptr;
    MessageBuffer(char* bytes, size_t size)
        : size(size), bytes(bytes) { }
  #ifdef _WIN32
    ICoreWebView2SharedBuffer* shared_buf = nullptr;
    MessageBuffer(ICoreWebView2SharedBuffer* buf, size_t size)
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

  struct MessageCancellation {
    void (*handler)(void*) = nullptr;
    void *data = nullptr;
  };

  class Message {
    public:
      using Seq = String;
      MessageBuffer buffer;
      String value = "";
      String name = "";
      String uri = "";
      int index = -1;
      Seq seq = "";
      Map args;
      bool isHTTP = false;
      std::shared_ptr<MessageCancellation> cancel;

      Message () = default;
      Message (const Message& message);
      Message (const String& source, bool decodeValues);
      Message (const String& source);
      Message (const String& source, bool decodeValues, char *bytes, size_t size);
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
      Message::Seq seq = "-1";
      uint64_t id = 0;
      String source = "";
      JSON::Any value = nullptr;
      JSON::Any data = nullptr;
      JSON::Any err = nullptr;
      Headers headers;
      Post post;

      Result () = default;
      Result (const Result&) = default;
      Result (const JSON::Any);
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
      using MessageCallback = std::function<void(const Message&, Router*, ReplyCallback)>;
      using BufferMap = std::map<String, MessageBuffer>;

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
        String path = "";
        bool redirect = false;
      };

      struct WebViewNavigatorMount {
        WebViewURLPathResolution resolution; // absolute URL resolution
        String path; // root path on host file system
        String route; // root path in webview navigator
      };

      static WebViewURLPathResolution resolveURLPathForWebView (String inputPath, const String& basePath);
      static WebViewNavigatorMount resolveNavigatorMountForWebView (const String& path);

    private:
      Table preserved;

    public:
      EvaluateJavaScriptCallback evaluateJavaScriptFunction = nullptr;
      std::function<void(DispatchCallback)> dispatchFunction = nullptr;
      BufferMap buffers;
      bool isReady = false;
      Mutex mutex;
      Table table;
      Listeners listeners;
      Core *core = nullptr;
      Bridge *bridge = nullptr;
    #if defined(__APPLE__)
      SSCIPCNetworkStatusObserver* networkStatusObserver = nullptr;
      SSCLocationObserver* locationObserver = nullptr;
      SSCIPCSchemeHandler* schemeHandler = nullptr;
      SSCIPCSchemeTasks* schemeTasks = nullptr;
      NSTimer* notificationPollTimer = nullptr;
    #endif

      Router ();
      Router (const Router &) = delete;
      ~Router ();

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

  class Bridge {
    public:
      Router router;
      Bluetooth bluetooth;
      Core *core = nullptr;
    #if !defined(__ANDROID__) && (defined(_WIN32) || defined(__linux__) || (defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR))
      FileSystemWatcher* fileSystemWatcher = nullptr;
    #endif

    #if defined(__ANDROID__)
      bool isAndroidEmulator = false;
    #endif

      Bridge (Core *core);
      ~Bridge ();
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
