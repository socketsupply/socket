#ifndef SSC_CORE_CORE_H
#define SSC_CORE_CORE_H

#include "../common.hh"
#include <uv.h>

#if defined(__APPLE__)
#import <Webkit/Webkit.h>
#import <Network/Network.h>
#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>
#import <UserNotifications/UserNotifications.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#import <UIKit/UIKit.h>
#else
#import <Cocoa/Cocoa.h>
#endif
#elif defined(__linux__) && !defined(__ANDROID__)
#include <JavaScriptCore/JavaScript.h>
#include <webkit2/webkit2.h>
#include <gtk/gtk.h>
#elif defined(_WIN32)
#include <WebView2.h>
#include <WebView2Experimental.h>
#include <WebView2EnvironmentOptions.h>
#include <WebView2ExperimentalEnvironmentOptions.h>
#include <shellapi.h>

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "WebView2LoaderStatic.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "userenv.lib")
#pragma comment(lib, "uv_a.lib")
#endif

#include "json.hh"
#include "runtime-preload.hh"

#if defined(__APPLE__)
@interface SSCBluetoothController : NSObject<
  CBCentralManagerDelegate,
  CBPeripheralManagerDelegate,
  CBPeripheralDelegate
>
@property (strong, nonatomic) CBCentralManager* centralManager;
@property (strong, nonatomic) CBPeripheralManager* peripheralManager;
@property (strong, nonatomic) CBPeripheral* bluetoothPeripheral;
@property (strong, nonatomic) NSMutableArray* peripherals;
@property (strong, nonatomic) NSMutableDictionary* services;
@property (strong, nonatomic) NSMutableDictionary* characteristics;
@property (strong, nonatomic) NSMutableDictionary* serviceMap;
- (void) startAdvertising;
- (void) startScanning;
- (id) init;
@end
#endif

namespace SSC {
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds

  // forward
  class Core;

  class Headers {
    public:
      class Value {
        public:
          String string;
          Value () = default;
          Value (String value) { this->string = value; }
          Value (const char* value) { this->string = value; }
          Value (const Value& value) { this->string = value.string; }
          Value (bool value) { this->string = value ? "true" : "false"; }
          Value (int value) { this->string = std::to_string(value); }
          Value (float value) { this->string = std::to_string(value); }
          Value (int64_t value) { this->string = std::to_string(value); }
          Value (uint64_t value) { this->string = std::to_string(value); }
          Value (double_t value) { this->string = std::to_string(value); }
#if defined(__APPLE__)
          Value (ssize_t value) { this->string = std::to_string(value); }
#endif
          String str () const { return this->string; }
      };

      class Header {
        public:
          String key;
          Value value;
          Header (const Header& header);
          Header (const String& key, const Value& value);
      };

      using Entries = Vector<Header>;
      Entries entries;
      Headers () = default;
      Headers (const Headers& headers);
      Headers (const Vector<std::map<String, Value>>& entries);
      Headers (const Entries& entries);
      size_t size () const;
      String str () const;
  };

  struct Post {
    uint64_t id = 0;
    uint64_t ttl = 0;
    char* body = nullptr;
    size_t length = 0;
    String headers = "";
  };

  using Posts = std::map<uint64_t, Post>;
  using EventLoopDispatchCallback = std::function<void()>;

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
  };

  typedef enum {
    PEER_TYPE_NONE = 0,
    PEER_TYPE_TCP = 1 << 1,
    PEER_TYPE_UDP = 1 << 2,
    PEER_TYPE_MAX = 0xF
  } peer_type_t;

  typedef enum {
    PEER_FLAG_NONE = 0,
    PEER_FLAG_EPHEMERAL = 1 << 1
  } peer_flag_t;

  typedef enum {
    PEER_STATE_NONE = 0,
    // general states
    PEER_STATE_CLOSED = 1 << 1,
    // udp states (10)
    PEER_STATE_UDP_BOUND = 1 << 10,
    PEER_STATE_UDP_CONNECTED = 1 << 11,
    PEER_STATE_UDP_RECV_STARTED = 1 << 12,
    PEER_STATE_UDP_PAUSED = 1 << 13,
    // tcp states (20)
    PEER_STATE_TCP_BOUND = 1 << 20,
    PEER_STATE_TCP_CONNECTED = 1 << 21,
    PEER_STATE_TCP_PAUSED = 1 << 13,
    PEER_STATE_MAX = 1 << 0xF
  } peer_state_t;

  struct LocalPeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getsockname (uv_udp_t *socket, struct sockaddr *addr);
    int getsockname (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  struct RemotePeerInfo {
    struct sockaddr_storage addr;
    String address = "";
    String family = "";
    int port = 0;
    int err = 0;

    int getpeername (uv_udp_t *socket, struct sockaddr *addr);
    int getpeername (uv_tcp_t *socket, struct sockaddr *addr);
    void init (uv_udp_t *socket);
    void init (uv_tcp_t *socket);
    void init (const struct sockaddr_storage *addr);
  };

  /**
   * A generic structure for a bound or connected peer.
   */
  class Peer {
    public:
      struct RequestContext {
        using Callback = std::function<void(int, Post)>;
        Callback cb;
        Peer *peer = nullptr;
        RequestContext (Callback cb) { this->cb = cb; }
      };

      using UDPReceiveCallback = std::function<void(
        ssize_t,
        const uv_buf_t*,
        const struct sockaddr*
      )>;

      // uv handles
      union {
        uv_udp_t udp;
        uv_tcp_t tcp; // XXX: FIXME
      } handle;

      // sockaddr
      struct sockaddr_in addr;

      // callbacks
      UDPReceiveCallback receiveCallback;
      std::vector<std::function<void()>> onclose;

      // instance state
      uint64_t id = 0;
      std::recursive_mutex mutex;
      Core *core;

      struct {
        struct {
          bool reuseAddr = false;
          bool ipv6Only = false; // @TODO
        } udp;
      } options;

      // peer state
      LocalPeerInfo local;
      RemotePeerInfo remote;
      peer_type_t type = PEER_TYPE_NONE;
      peer_flag_t flags = PEER_FLAG_NONE;
      peer_state_t state = PEER_STATE_NONE;

      /**
      * Private `Peer` class constructor
      */
      Peer (Core *core, peer_type_t peerType, uint64_t peerId, bool isEphemeral);
      ~Peer ();

      int init ();
      int initRemotePeerInfo ();
      int initLocalPeerInfo ();
      void addState (peer_state_t value);
      void removeState (peer_state_t value);
      bool hasState (peer_state_t value);
      const RemotePeerInfo* getRemotePeerInfo ();
      const LocalPeerInfo* getLocalPeerInfo ();
      bool isUDP ();
      bool isTCP ();
      bool isEphemeral ();
      bool isBound ();
      bool isActive ();
      bool isClosing ();
      bool isClosed ();
      bool isConnected ();
      bool isPaused ();
      int bind ();
      int bind (String address, int port);
      int bind (String address, int port, bool reuseAddr);
      int rebind ();
      int connect (String address, int port);
      int disconnect ();
      void send (
        char *buf,
        size_t size,
        int port,
        const String address,
        Peer::RequestContext::Callback cb
      );
      int recvstart ();
      int recvstart (UDPReceiveCallback onrecv);
      int recvstop ();
      int resume ();
      int pause ();
      void close ();
      void close (std::function<void()> onclose);
  };

  static inline String addrToIPv4 (struct sockaddr_in* sin) {
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, buf, INET_ADDRSTRLEN);
    return String(buf);
  }

  static inline String addrToIPv6 (struct sockaddr_in6* sin) {
    char buf[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &sin->sin6_addr, buf, INET6_ADDRSTRLEN);
    return String(buf);
  }

  static inline void parseAddress (struct sockaddr *name, int* port, char* address) {
    struct sockaddr_in *name_in = (struct sockaddr_in *) name;
    *port = ntohs(name_in->sin_port);
    uv_ip4_name(name_in, address, 17);
  }

  class Bluetooth {
    public:
      using SendFunction = std::function<void(const String, JSON::Any, Post)>;
      using EmitFunction = std::function<void(const String, JSON::Any)>;
      using Callback = std::function<void(String, JSON::Any)>;

      Core *core = nullptr;
      #if defined(__APPLE__)
      SSCBluetoothController* controller= nullptr;
      #endif

      SendFunction sendFunction;
      EmitFunction emitFunction;

      Bluetooth ();
      ~Bluetooth ();
      bool send (const String& seq, JSON::Any json, Post post);
      bool send (const String& seq, JSON::Any json);
      bool emit (const String& seq, JSON::Any json);
      void startScanning ();
      void publishCharacteristic (
        const String& seq,
        char* bytes,
        size_t size,
        const String& serviceId,
        const String& characteristicId,
        Callback callback
      );
      void subscribeCharacteristic (
        const String& seq,
        const String& serviceId,
        const String& characteristicId,
        Callback callback
      );
      void startService (
        const String& seq,
        const String& serviceId,
        Callback callback
      );
  };

  class Core {
    public:
      class Module {
        public:
          using Callback = std::function<void(String, JSON::Any, Post)>;
          struct RequestContext {
            String seq;
            Module::Callback cb;
            RequestContext () = default;
            RequestContext (String seq, Module::Callback cb) {
              this->seq = seq;
              this->cb = cb;
            }
          };

          Core *core = nullptr;
          Module (Core* core) {
            this->core = core;
          }
      };

      class Diagnostics : public Module {
        public:
          Diagnostics (auto core) : Module(core) {}
      };

      class DNS : public Module {
        public:
          DNS (auto core) : Module(core) {}
          struct LookupOptions {
            String hostname;
            int family;
            // TODO: support these options
            // - hints
            // - all
            // -verbatim
          };
          void lookup (
            const String seq,
            LookupOptions options,
            Module::Callback cb
          );
      };

      class FS : public Module {
        public:
          FS (auto core) : Module(core) {}

          struct Descriptor {
            uint64_t id;
            std::atomic<bool> retained = false;
            std::atomic<bool> stale = false;
            Mutex mutex;
            uv_dir_t *dir = nullptr;
            uv_file fd = 0;
            Core *core;

            Descriptor (Core *core, uint64_t id);
            bool isDirectory ();
            bool isFile ();
            bool isRetained ();
            bool isStale ();
          };

          struct RequestContext : Module::RequestContext {
            uint64_t id;
            Descriptor *desc = nullptr;
            uv_fs_t req;
            uv_buf_t iov[16];
            // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
            uv_dirent_t dirents[256];
            int offset = 0;
            int result = 0;

            RequestContext () = default;
            RequestContext (Descriptor *desc)
              : RequestContext(desc, "", nullptr) {}
            RequestContext (String seq, Callback cb)
              : RequestContext(nullptr, seq, cb) {}
            RequestContext (Descriptor *desc, String seq, Callback cb) {
              this->id = SSC::rand64();
              this->cb = cb;
              this->seq = seq;
              this->desc = desc;
              this->req.data = (void *) this;
            }

            ~RequestContext () {
              uv_fs_req_cleanup(&this->req);
            }

            void setBuffer (int index, size_t len, char *base);
            void freeBuffer (int index);
            char* getBuffer (int index);
            size_t getBufferSize (int index);
          };

          std::map<uint64_t, Descriptor*> descriptors;
          Mutex mutex;

          Descriptor * getDescriptor (uint64_t id);
          void removeDescriptor (uint64_t id);
          bool hasDescriptor (uint64_t id);

          void constants (const String seq, Module::Callback cb);
          void access (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void chmod (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void close (const String seq, uint64_t id, Module::Callback cb);
          void copyFile (
            const String seq,
            const String src,
            const String dst,
            int mode,
            Module::Callback cb
          );
          void closedir (const String seq, uint64_t id, Module::Callback cb);
          void closeOpenDescriptor (
            const String seq,
            uint64_t id,
            Module::Callback cb
          );
          void closeOpenDescriptors (const String seq, Module::Callback cb);
          void closeOpenDescriptors (
            const String seq,
            bool preserveRetained,
            Module::Callback cb
          );
          void fstat (const String seq, uint64_t id, Module::Callback cb);
          void getOpenDescriptors (const String seq, Module::Callback cb);
          void lstat (const String seq, const String path, Module::Callback cb);
          void mkdir (
            const String seq,
            const String path,
            int mode,
            Module::Callback cb
          );
          void open (
            const String seq,
            uint64_t id,
            const String path,
            int flags,
            int mode,
            Module::Callback cb
          );
          void opendir (
            const String seq,
            uint64_t id,
            const String path,
            Module::Callback cb
          );
          void read (
            const String seq,
            uint64_t id,
            size_t len,
            size_t offset,
            Module::Callback cb
          );
          void readdir (
            const String seq,
            uint64_t id,
            size_t entries,
            Module::Callback cb
          );
          void retainOpenDescriptor (
            const String seq,
            uint64_t id,
            Module::Callback cb
          );
          void rename (
            const String seq,
            const String src,
            const String dst,
            Module::Callback cb
          );
          void rmdir (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void stat (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void unlink (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void write (
            const String seq,
            uint64_t id,
            char *bytes,
            size_t size,
            size_t offset,
            Module::Callback cb
          );
      };

      class OS : public Module {
        public:
          static const int RECV_BUFFER = 1;
          static const int SEND_BUFFER = 0;

          OS (auto core) : Module(core) {}
          void bufferSize (
            const String seq,
            uint64_t peerId,
            size_t size,
            int buffer,
            Module::Callback cb
          );
          void cpus  (
            const String seq,
            Module::Callback cb
          );
          void networkInterfaces (const String seq, Module::Callback cb) const;
          void rusage (
            const String seq,
            Module::Callback cb
          );
          void uname (
            const String seq,
            Module::Callback cb
          );
          void uptime (
            const String seq,
            Module::Callback cb
          );
          void hrtime (
            const String seq,
            Module::Callback cb
          );
          void availableMemory (
            const String seq,
            Module::Callback cb
          );
      };

      class Platform : public Module {
        public:
          Platform (auto core) : Module(core) {}
          void event (
            const String seq,
            const String event,
            const String data,
            Module::Callback cb
          );
          void notify (
            const String seq,
            const String title,
            const String body,
            Module::Callback cb
          );
          void openExternal (
            const String seq,
            const String value,
            Module::Callback cb
          );
      };

      class UDP : public Module {
        public:
          UDP (auto core) : Module(core) {}

          struct BindOptions {
            String address;
            int port;
            bool reuseAddr = false;
          };

          struct ConnectOptions {
            String address;
            int port;
          };

          struct SendOptions {
            String address = "";
            int port = 0;
            char *bytes = nullptr;
            size_t size = 0;
            bool ephemeral = false;
          };

          void bind (
            const String seq,
            uint64_t id,
            BindOptions options,
            Module::Callback cb
          );
          void close (const String seq, uint64_t id, Module::Callback cb);
          void connect (
            const String seq,
            uint64_t id,
            ConnectOptions options,
            Module::Callback cb
          );
          void disconnect (const String seq, uint64_t id, Module::Callback cb);
          void getPeerName (const String seq, uint64_t id, Module::Callback cb);
          void getSockName (const String seq, uint64_t id, Module::Callback cb);
          void getState (const String seq, uint64_t id,  Module::Callback cb);
          void readStart (const String seq, uint64_t id, Module::Callback cb);
          void readStop (const String seq, uint64_t id, Module::Callback cb);
          void send (
            const String seq,
            uint64_t id,
            SendOptions options,
            Module::Callback cb
          );
      };

      Diagnostics diagnostics;
      DNS dns;
      FS fs;
      OS os;
      Platform platform;
      UDP udp;

      std::shared_ptr<Posts> posts;
      std::map<uint64_t, Peer*> peers;

      std::recursive_mutex loopMutex;
      std::recursive_mutex peersMutex;
      std::recursive_mutex postsMutex;
      std::recursive_mutex timersMutex;

      std::atomic<bool> didLoopInit = false;
      std::atomic<bool> didTimersInit = false;
      std::atomic<bool> didTimersStart = false;

      std::atomic<bool> isLoopRunning = false;

      uv_loop_t eventLoop;
      uv_async_t eventLoopAsync;
      std::queue<EventLoopDispatchCallback> eventLoopDispatchQueue;

#if defined(__APPLE__)
      dispatch_queue_attr_t eventLoopQueueAttrs = dispatch_queue_attr_make_with_qos_class(
        DISPATCH_QUEUE_SERIAL,
        QOS_CLASS_DEFAULT,
        -1
      );

      dispatch_queue_t eventLoopQueue = dispatch_queue_create(
        "socket.runtime.core.loop.queue",
        eventLoopQueueAttrs
      );
#else
      std::thread *eventLoopThread = nullptr;
#endif

      Core () :
        diagnostics(this),
        dns(this),
        fs(this),
        os(this),
        platform(this),
        udp(this)
      {
        this->posts = std::shared_ptr<Posts>(new Posts());
        initEventLoop();
      }

      void resumeAllPeers ();
      void pauseAllPeers ();
      bool hasPeer (uint64_t id);
      void removePeer (uint64_t id);
      void removePeer (uint64_t id, bool autoClose);
      Peer* getPeer (uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id);
      Peer* createPeer (peer_type_t type, uint64_t id, bool isEphemeral);

      Post getPost (uint64_t id);
      bool hasPost (uint64_t id);
      void removePost (uint64_t id);
      void removeAllPosts ();
      void expirePosts ();
      void putPost (uint64_t id, Post p);
      String createPost (String seq, String params, Post post);

      // timers
      void initTimers ();
      void startTimers ();
      void stopTimers ();

      // loop
      uv_loop_t* getEventLoop ();
      int getEventLoopTimeout ();
      bool isLoopAlive ();
      void initEventLoop ();
      void runEventLoop ();
      void stopEventLoop ();
      void dispatchEventLoop (EventLoopDispatchCallback dispatch);
      void signalDispatchEventLoop ();
      void sleepEventLoop (int64_t ms);
      void sleepEventLoop ();
  };

  String createJavaScript (const String& name, const String& source);

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value,
    const String& target,
    const JSON::Object& options
  );

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  );

  String getResolveMenuSelectionJavaScript (
    const String& seq,
    const String& title,
    const String& parent
  );

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  );
} // SSC

#endif // SSC_CORE_CORE_H
