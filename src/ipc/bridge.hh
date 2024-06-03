#ifndef SOCKET_RUNTIME_IPC_BRIDGE_H
#define SOCKET_RUNTIME_IPC_BRIDGE_H

#include "../core/core.hh"
#include "../core/webview.hh"

#include "preload.hh"
#include "navigator.hh"
#include "router.hh"
#include "scheme_handlers.hh"

namespace SSC::IPC {
  class Bridge {
    public:
      using EvaluateJavaScriptFunction = Function<void(const String)>;
      using NavigateFunction = Function<void(const String&)>;
      using DispatchCallback = Function<void()>;
      using DispatchFunction = Function<void(DispatchCallback)>;

      static Vector<Bridge*> getInstances();

      const CoreNetworkStatus::Observer networkStatusObserver;
      const CoreGeolocation::PermissionChangeObserver geolocationPermissionChangeObserver;
      const CoreNotifications::PermissionChangeObserver notificationsPermissionChangeObserver;
      const CoreNotifications::NotificationResponseObserver notificationResponseObserver;
      const CoreNotifications::NotificationPresentedObserver notificationPresentedObserver;

      EvaluateJavaScriptFunction evaluateJavaScriptFunction = nullptr;
      NavigateFunction navigateFunction = nullptr;
      DispatchFunction dispatchFunction = nullptr;

      Bluetooth bluetooth;
      Navigator navigator;
      SchemeHandlers schemeHandlers;
      Preload preload;
      Router router;
      Map userConfig;

      SharedPointer<Core> core = nullptr;
      uint64_t id = 0;

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      bool isAndroidEmulator = false;
    #endif

      Bridge () = delete;
      Bridge (const Bridge&) = delete;
      Bridge (Bridge&&) = delete;
      Bridge (SharedPointer<Core>core, Map userConfig);
      ~Bridge ();

      Bridge& operator = (const Bridge&) = delete;
      Bridge& operator = (Bridge&&) = delete;

      void init ();
      void configureWebView (WebView* webview);
      void configureSchemeHandlers (const SchemeHandlers::Configuration& configuration);
      void configureNavigatorMounts ();

      bool route (const String& uri, SharedPointer<char[]> bytes, size_t size);
      bool route (
        const String& uri,
        SharedPointer<char[]> bytes,
        size_t size,
        Router::ResultCallback
      );

      bool evaluateJavaScript (const String& source);
      bool dispatch (const DispatchCallback& callback);
      bool navigate (const String& url);
      bool emit (const String& name, const String& data = "");
      bool emit (const String& name, const JSON::Any& json = {});
      bool send (const Message::Seq& seq, const String& data, const Post& post = {});
      bool send (const Message::Seq& seq, const JSON::Any& json, const Post& post = {});
  };
}
#endif
