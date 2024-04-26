#ifndef SSC_CORE_CORE_H
#define SSC_CORE_CORE_H

#include "bluetooth.hh"
#include "codec.hh"
#include "config.hh"
#include "debug.hh"
#include "env.hh"
#include "file_system_watcher.hh"
#include "geolocation.hh"
#include "headers.hh"
#include "ini.hh"
#include "io.hh"
#include "ip.hh"
#include "json.hh"
#include "module.hh"
#include "network_status.hh"
#include "notifications.hh"
#include "peer.hh"
#include "platform.hh"
#include "post.hh"
#include "preload.hh"
#include "protocol_handlers.hh"
#include "resource.hh"
#include "service_worker_container.hh"
#include "string.hh"
#include "types.hh"
#include "url.hh"
#include "version.hh"

namespace SSC {
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds

  uint64_t rand64 ();
  void msleep (uint64_t ms);

  // forward
  class Core;
  class Process;

  using EventLoopDispatchCallback = Function<void()>;

  struct Timer {
    uv_timer_t handle;
    bool repeated = false;
    bool started = false;
    uint64_t timeout = 0;
    uint64_t interval = 0;
    uv_timer_cb invoke;
  };

  class Core {
    public:
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
            Atomic<bool> retained = false;
            Atomic<bool> stale = false;
            UniquePointer<FileResource> resource = nullptr;
            Mutex mutex;
            uv_dir_t *dir = nullptr;
            uv_file fd = 0;
            Core *core;

            Descriptor (Core *core, uint64_t id, const String& filename);
            bool isDirectory ();
            bool isFile ();
            bool isRetained ();
            bool isStale ();
          };

          struct RequestContext : Module::RequestContext {
            uint64_t id;
            Descriptor *desc = nullptr;
            uv_fs_t req;
            uv_buf_t buf;
            // 256 which corresponds to DirectoryHandle.MAX_BUFFER_SIZE
            uv_dirent_t dirents[256];
            int offset = 0;
            int result = 0;
            bool recursive;  // A place to stash recursive options when needed

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
              this->recursive = false;
            }

            ~RequestContext () {
              uv_fs_req_cleanup(&this->req);
            }

            void setBuffer (char* base, uint32_t len);
            void freeBuffer ();
            char* getBuffer ();
            uint32_t getBufferSize ();
          };

        #if !SSC_PLATFORM_ANDROID
          std::map<uint64_t, FileSystemWatcher*> watchers;
        #endif

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
          void chown (
            const String seq,
            const String path,
            uv_uid_t uid,
            uv_gid_t gid,
            Module::Callback cb
          );
          void lchown (
            const String seq,
            const String path,
            uv_uid_t uid,
            uv_gid_t gid,
            Module::Callback cb
          );
          void close (const String seq, uint64_t id, Module::Callback cb);
          void copyFile (
            const String seq,
            const String src,
            const String dst,
            int flags,
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
          void fsync (const String seq, uint64_t id, Module::Callback cb);
          void ftruncate (
            const String seq,
            uint64_t id,
            int64_t offset,
            Module::Callback cb
          );
          void getOpenDescriptors (const String seq, Module::Callback cb);
          void lstat (const String seq, const String path, Module::Callback cb);
					void link (
            const String seq,
            const String src,
            const String dest,
            Module::Callback cb
          );
          void symlink (
            const String seq,
            const String src,
            const String dest,
            int flags,
            Module::Callback cb
          );
          void mkdir (
            const String seq,
            const String path,
            int mode,
            bool recursive,
            Module::Callback cb
          );
          void readlink (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void realpath (
            const String seq,
            const String path,
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
          void stopWatch (
            const String seq,
            uint64_t id,
            Module::Callback cb
          );
          void unlink (
            const String seq,
            const String path,
            Module::Callback cb
          );
          void watch (
            const String seq,
            uint64_t id,
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
          void constants (const String seq, Module::Callback cb);
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
          void revealFile (
            const String seq,
            const String value,
            Module::Callback cb
          );

      };

    #if !SSC_PLATFORM_IOS
      class ChildProcess : public Module {
        public:
          using Handles = std::map<uint64_t, Process*>;
          struct SpawnOptions {
            String cwd;
            bool allowStdin = true;
            bool allowStdout = true;
            bool allowStderr = true;
          };

          struct ExecOptions {
            String cwd;
            bool allowStdout = true;
            bool allowStderr = true;
            uint64_t timeout = 0;
            #ifdef _WIN32
              int killSignal = 0; // unused
            #else
              int killSignal = SIGTERM;
            #endif
          };

          Handles handles;
          Mutex mutex;

          ChildProcess (auto core) : Module(core) {}

          void shutdown ();
          void exec (
            const String seq,
            uint64_t id,
            const Vector<String> args,
            const ExecOptions options,
            Module::Callback cb
          );

          void spawn (
            const String seq,
            uint64_t id,
            const Vector<String> args,
            const SpawnOptions options,
            Module::Callback cb
          );

          void kill (
            const String seq,
            uint64_t id,
            int signal,
            Module::Callback cb
          );

          void write (
            const String seq,
            uint64_t id,
            char* buffer,
            size_t size,
            Module::Callback cb
          );
      };
    #endif

      class Timers : public Module {
        public:
          using ID = uint64_t;
          using CancelCallback = Function<void()>;
          using Callback = Function<void(CancelCallback)>;
          using TimeoutCallback = Function<void()>;
          using IntervalCallback = Callback;
          using ImmediateCallback = TimeoutCallback;

          struct Timer {
            Timers* timers = nullptr;
            ID id = 0;
            Callback callback;
            bool repeat = false;
            bool cancelled = false;
            uv_timer_t timer;
          };

          using Handles = std::map<ID, Timer>;

          Handles handles;
          Mutex mutex;

          Timers (auto core): Module (core) {}

          const ID createTimer (uint64_t timeout, uint64_t interval, const Callback callback);
          bool cancelTimer (const ID id);
          const ID setTimeout (uint64_t timeout, const TimeoutCallback callback);
          bool clearTimeout (const ID id);
          const ID setImmediate (const ImmediateCallback callback);
          bool clearImmediate (const ID id);
          const ID setInterval (uint64_t interval, const IntervalCallback callback);
          bool clearInterval (const ID id);
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

    #if !SSC_PLATFORM_IOS
      ChildProcess childProcess;
    #endif
      Diagnostics diagnostics;
      DNS dns;
      FS fs;
      Geolocation geolocation;
      NetworkStatus networkStatus;
      Notifications notifications;
      OS os;
      Platform platform;
      ProtocolHandlers protocolHandlers;
      ServiceWorkerContainer serviceWorker;
      Timers timers;
      UDP udp;

      Posts posts;
      std::map<uint64_t, Peer*> peers;

      Mutex loopMutex;
      Mutex peersMutex;
      Mutex postsMutex;
      Mutex timersMutex;

      Atomic<bool> didLoopInit = false;
      Atomic<bool> didTimersInit = false;
      Atomic<bool> didTimersStart = false;
      Atomic<bool> isLoopRunning = false;
      Atomic<bool> shuttingDown = false;
      Atomic<bool> domReady = false;

      uv_loop_t eventLoop;
      uv_async_t eventLoopAsync;
      Queue<EventLoopDispatchCallback> eventLoopDispatchQueue;

    #if SSC_PLATFORM_APPLE
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
      #if !SSC_PLATFORM_IOS
        childProcess(this),
      #endif
        diagnostics(this),
        dns(this),
        fs(this),
        geolocation(this),
        networkStatus(this),
        notifications(this),
        os(this),
        platform(this),
        protocolHandlers(this),
        timers(this),
        udp(this),
        serviceWorker(this)
      {
        initEventLoop();
      }

      void shutdown ();

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
      bool hasPostBody (const char* body);
      void removePost (uint64_t id);
      void removeAllPosts ();
      void expirePosts ();
      void putPost (uint64_t id, Post p);
      String createPost (String seq, String params, Post post);

      // timers
      void initTimers ();
      void startTimers ();
      void stopTimers ();
      const Timers::ID setTimeout (uint64_t timeout, const Timers::TimeoutCallback callback);
      const Timers::ID setImmediate (const Timers::ImmediateCallback callback);
      const Timers::ID setInterval (uint64_t interval, const Timers::IntervalCallback callback);
      bool clearTimeout (const Timers::ID id);
      bool clearImmediate (const Timers::ID id);
      bool clearInterval (const Timers::ID id);

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
    const String& parent,
    const String type = "system"
  );

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  );

  void setcwd (const String& cwd);
  const String getcwd ();
  const String getcwd_state_value ();
} // SSC

#endif // SSC_CORE_CORE_H
