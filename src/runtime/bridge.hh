#ifndef SOCKET_RUNTIME_BRIDGE_H
#define SOCKET_RUNTIME_BRIDGE_H

#include "ipc.hh"
#include "json.hh"
#include "bytes.hh"
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
      using DispatchHandler = Function<void(const context::DispatchCallback)>;

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

      DispatchHandler dispatchHandler = nullptr;

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
      bool route (const String&, SharedPointer<unsigned char[]>, size_t) override;
      bool route (const String&, SharedPointer<unsigned char[]>, size_t, const ipc::Router::ResultCallback) override;
      Runtime* getRuntime () override;
      const Runtime* getRuntime () const override;

      bool route (const String&, const bytes::Buffer&);
      bool route (const String&, const bytes::Buffer&, const ipc::Router::ResultCallback);

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
      struct BridgeOptions {
        Map<String, String> userConfig;
      };

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

      SharedPointer<Bridge> get (int, const BridgeOptions&);
      bool has (int) const;
      bool remove (int);
  };
}
#endif
