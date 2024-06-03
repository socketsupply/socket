#ifndef SOCKET_RUNTIME_CORE_CORE_H
#define SOCKET_RUNTIME_CORE_CORE_H

#include "../platform/platform.hh"

#include "bluetooth.hh"
#include "codec.hh"
#include "config.hh"
#include "debug.hh"
#include "env.hh"
#include "file_system_watcher.hh"
#include "headers.hh"
#include "ini.hh"
#include "io.hh"
#include "ip.hh"
#include "json.hh"
#include "module.hh"
#include "socket.hh"
#include "post.hh"
#include "resource.hh"
#include "url.hh"
#include "version.hh"
#include "webview.hh"

#include "modules/ai.hh"
#include "modules/child_process.hh"
#include "modules/dns.hh"
#include "modules/fs.hh"
#include "modules/geolocation.hh"
#include "modules/network_status.hh"
#include "modules/notifications.hh"
#include "modules/os.hh"
#include "modules/platform.hh"
#include "modules/timers.hh"
#include "modules/udp.hh"

namespace SSC {
  constexpr int EVENT_LOOP_POLL_TIMEOUT = 32; // in milliseconds

  // forward
  class Core;
  class Process;

  using EventLoopDispatchCallback = Function<void()>;

  class Core {
    public:
      #if !SOCKET_RUNTIME_PLATFORM_IOS
        using ChildProcess = CoreChildProcess;
      #endif
      using DNS = CoreDNS;
      using FS = CoreFS;
      using Geolocation = CoreGeolocation;
      using NetworkStatus = CoreNetworkStatus;
      using Notifications = CoreNotifications;
      using OS = CoreOS;
      using Platform = CorePlatform;
      using Timers = CoreTimers;
      using UDP = CoreUDP;
      using AI = CoreAI;

      struct SharedPointerBuffer {
        SharedPointer<char[]> pointer;
        unsigned int ttl = 0;
      };

    #if !SOCKET_RUNTIME_PLATFORM_IOS
      ChildProcess childProcess;
    #endif
      DNS dns;
      FS fs;
      Geolocation geolocation;
      NetworkStatus networkStatus;
      Notifications notifications;
      OS os;
      Platform platform;
      Timers timers;
      UDP udp;
      AI ai;

      Vector<SharedPointerBuffer> sharedPointerBuffers;
      Posts posts;

      Mutex mutex;
      Mutex loopMutex;
      Mutex postsMutex;
      Mutex timersMutex;

      Atomic<bool> didLoopInit = false;
      Atomic<bool> didTimersInit = false;
      Atomic<bool> didTimersStart = false;
      Atomic<bool> isLoopRunning = false;
      Atomic<bool> shuttingDown = false;

      uv_loop_t eventLoop;
      uv_async_t eventLoopAsync;
      Queue<EventLoopDispatchCallback> eventLoopDispatchQueue;

      #if SOCKET_RUNTIME_PLATFORM_APPLE
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
        Thread *eventLoopThread = nullptr;
      #endif

      Core () :
        #if !SOCKET_RUNTIME_PLATFORM_IOS
          childProcess(this),
        #endif
        ai(this),
        dns(this),
        fs(this),
        geolocation(this),
        networkStatus(this),
        notifications(this),
        os(this),
        platform(this),
        timers(this),
        udp(this)
      {
        initEventLoop();
      }

      ~Core () {
        this->shutdown();
      }

      // called when the application is shutting down
      void shutdown ();

      void retainSharedPointerBuffer (SharedPointer<char[]> pointer, unsigned int ttl);
      void releaseSharedPointerBuffer (SharedPointer<char[]> pointer);

      // core module post data management
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
      const CoreTimers::ID setTimeout (uint64_t timeout, const CoreTimers::TimeoutCallback& callback);
      const CoreTimers::ID setInterval (uint64_t interval, const CoreTimers::IntervalCallback& callback);
      const CoreTimers::ID setImmediate (const CoreTimers::ImmediateCallback& callback);
      bool clearTimeout (const CoreTimers::ID id);
      bool clearInterval (const CoreTimers::ID id);
      bool clearImmediate (const CoreTimers::ID id);

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

#endif // SOCKET_RUNTIME_CORE_CORE_H
