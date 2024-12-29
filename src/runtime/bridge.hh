#ifndef SOCKET_RUNTIME_BRIDGE_H
#define SOCKET_RUNTIME_BRIDGE_H

#include "ipc.hh"
#include "json.hh"
#include "window.hh"
#include "context.hh"
#include "webview.hh"
#include "queued_response.hh"

#include "core/services.hh"

namespace ssc::runtime::bridge {
  using Client = ipc::Client;

  /**
   * The `Bridge` class represents a bi-directional interface between the
   * runtime services and a window.
   */
  class Bridge : public window::IBridge {
    public:
      using ID = uint64_t;

      struct Options {
        Client client;
        context::RuntimeContext& context;
        context::Dispatcher& dispatcher;
        webview::Preload::Options preload;
        Map<String, String> userConfig;
      };

      const core::services::NetworkStatus::Observer networkStatusObserver;
      const core::services::Geolocation::PermissionChangeObserver geolocationPermissionChangeObserver;
      const core::services::Notifications::PermissionChangeObserver notificationsPermissionChangeObserver;
      const core::services::Notifications::NotificationResponseObserver notificationResponseObserver;
      const core::services::Notifications::NotificationPresentedObserver notificationPresentedObserver;

      using window::IBridge::IBridge;
      Bridge (const Options&);
      ~Bridge ();

      void init () override;

      // `ipc::IBridge`
      bool active () const override;
      bool emit (const String&, const String& = "") override;
      bool emit (const String&, const JSON::Any& = {}) override;
      bool send (const ipc::Message::Seq&, const String&, const QueuedResponse& = {}) override;
      bool send (const ipc::Message::Seq&, const JSON::Any&, const QueuedResponse& = {}) override;
      bool route (const String&, SharedPointer<char[]>, size_t) override;
      bool route (const String&, SharedPointer<char[]>, size_t, const ipc::Router::ResultCallback) override;

      // `window::IBridge`
      void configureWebView (webview::WebView* webview) override;
      void configureSchemeHandlers (const webview::SchemeHandlers::Configuration&) override;
      void configureNavigatorMounts () override;
      bool evaluateJavaScript (const String&) override;
      bool dispatch (const context::DispatchCallback) override;
      // `webview::IBridge`
      bool navigate (const String& url) override;
  };

  class Manager {
    public:
      Vector<SharedPointer<Bridge>> entries;
      context::RuntimeContext& context;
      Mutex mutex;

      Manager (context::RuntimeContext&);
      Manager () = delete;
      Manager (const Manager&) = delete;
      Manager (Manager&&) = delete;
      virtual ~Manager();

      Manager& operator = (const Manager&) = delete;
      Manager& operator = (Manager&&) = delete;

      SharedPointer<Bridge> get (int);
      bool has (int) const;
      bool remove (int);
  };

  /*
  class Bridge {
    public:
      using EvaluateJavaScriptFunction = Function<void(const String)>;
      using NavigateFunction = Function<void(const String&)>;
      using DispatchCallback = Function<void()>;
      using DispatchFunction = Function<void(DispatchCallback)>;

      struct Options : public SSC::Options {
        Map<String, String> userConfig = {};
        Preload::Options preload;
        uint64_t id = 0;
        int index = 0;
        Options (
          int index,
          const Map<String, String> userConfig = {},
          const Preload::Options preload = {},
          uint64_t id = 0
        );
      };

      static Vector<Bridge*> getInstances();

      const CoreNetworkStatus::Observer networkStatusObserver;
      const CoreGeolocation::PermissionChangeObserver geolocationPermissionChangeObserver;
      const CoreNotifications::PermissionChangeObserver notificationsPermissionChangeObserver;
      const CoreNotifications::NotificationResponseObserver notificationResponseObserver;
      const CoreNotifications::NotificationPresentedObserver notificationPresentedObserver;

      EvaluateJavaScriptFunction evaluateJavaScriptFunction = nullptr;
      NavigateFunction navigateFunction = nullptr;
      DispatchFunction dispatchFunction = nullptr;

      Client client = {};
      Map<String, String> userConfig;
      Navigator navigator;
      Router router;
      SchemeHandlers schemeHandlers;

      uint64_t id = 0;
      int index = 0;

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      bool isAndroidEmulator = false;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
      WebKitWebContext* webContext = nullptr;
    #endif

      Bridge (SharedPointer<Core>core, const Options& options);
      Bridge () = delete;
      Bridge (const Bridge&) = delete;
      Bridge (Bridge&&) = delete;
      virtual ~Bridge ();

      Bridge& operator = (const Bridge&) = delete;
      Bridge& operator = (Bridge&&) = delete;

      void init ();
      void configureWebView (WebView* webview);
      void configureSchemeHandlers (
        const SchemeHandlers::Configuration& configuration
      );
      void configureNavigatorMounts ();

      bool route (
        const String& uri,
        SharedPointer<char[]> bytes,
        size_t size
      );

      bool route (
        const String& uri,
        SharedPointer<char[]> bytes,
        size_t size,
        Router::ResultCallback
      );

      bool evaluateJavaScript (const String& source);
      bool dispatch (const DispatchCallback callback);
      bool navigate (const String& url);
      bool emit (const String& name, const String& data = "");
      bool emit (const String& name, const JSON::Any& json = {});
      bool send (
        const Message::Seq& seq,
        const String& data,
        const QueuedResponse& post = {}
      );
      bool send (
        const Message::Seq& seq,
        const JSON::Any& json,
        const QueuedResponse& post = {}
      );
  };
  */
}
#endif
