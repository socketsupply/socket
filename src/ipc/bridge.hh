#ifndef SSC_IPC_BRIDGE_H
#define SSC_IPC_BRIDGE_H

#include "../core/core.hh"
#include "../window/webview.hh"
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

      const NetworkStatus::Observer networkStatusObserver;
      const Geolocation::PermissionChangeObserver geolocationPermissionChangeObserver;
      const Notifications::PermissionChangeObserver notificationsPermissionChangeObserver;
      const Notifications::NotificationResponseObserver notificationResponseObserver;
      const Notifications::NotificationPresentedObserver notificationPresentedObserver;

      EvaluateJavaScriptFunction evaluateJavaScriptFunction = nullptr;
      NavigateFunction navigateFunction = nullptr;
      DispatchFunction dispatchFunction = nullptr;

      Bluetooth bluetooth;
      Navigator navigator;
      SchemeHandlers schemeHandlers;
      Router router;
      Map userConfig = getUserConfig();

      SharedPointer<Core> core = nullptr;
      String preload = "";
      uint64_t id = 0;

    #if SSC_PLATFORM_ANDROID
      bool isAndroidEmulator = false;
    #endif

      Bridge () = delete;
      Bridge (const Bridge&) = delete;
      Bridge (Bridge&&) = delete;
      Bridge (SharedPointer<Core>core, Map userConfig = getUserConfig());
      ~Bridge ();

      void init ();
      void configureWebView (WebView* webview);
      void configureSchemeHandlers (const SchemeHandlers::Configuration& configuration);

      bool route (const String& uri, SharedPointer<char *> bytes, size_t size);
      bool route (
        const String& uri,
        SharedPointer<char *> bytes,
        size_t size,
        Router::ResultCallback
      );

      const Vector<String>& getAllowedNodeCoreModules () const;
      bool evaluateJavaScript (const String& source);
      bool dispatch (const DispatchCallback& callback);
      bool navigate (const String& url);
      bool emit (const String& name, const String& data);
      bool emit (const String& name, const JSON::Any& json);
      bool send (const Message::Seq& seq, const String& data, const Post& post);
      bool send (const Message::Seq& seq, const JSON::Any& json, const Post& post);
  };
}
#endif
