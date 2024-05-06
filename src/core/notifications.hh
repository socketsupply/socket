#ifndef SSC_CORE_NOTIFICATIONS_H
#define SSC_CORE_NOTIFICATIONS_H

#include "module.hh"
#include "platform.hh"
#include "../window/webview.hh"

namespace SSC {
  class Notifications;
}

#if SSC_PLATFORM_APPLE
@interface SSCUserNotificationCenterDelegate : NSObject<UNUserNotificationCenterDelegate>
@property (nonatomic) SSC::Notifications* notifications;
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler;

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler;
@end
#endif

namespace SSC {
  class Notifications : public Module {
    public:
      using PermissionChangeObserver = Module::Observer<JSON::Object>;
      using PermissionChangeObservers = Module::Observers<PermissionChangeObserver>;
      using NotificationResponseObserver = Module::Observer<JSON::Object>;
      using NotificationResponseObservers = Module::Observers<NotificationResponseObserver>;
      using NotificationPresentedObserver = Module::Observer<JSON::Object>;
      using NotificationPresentedObservers = Module::Observers<NotificationPresentedObserver>;

      struct Notification {
        String identifier;
        const JSON::Object json () const;
      };

      struct ShowOptions {
        String id;
        String title;
        String tag;
        String lang;
        bool silent = false;
        String icon;
        String image;
        String body;
      };

      struct ShowResult {
        String error = "";
        Notification notification;
      };

      using ShowCallback = Function<void(const ShowResult&)>;
      using ListCallback = Function<void(const Vector<Notification>&)>;

    #if SSC_PLATFORM_APPLE
      SSCUserNotificationCenterDelegate* userNotificationCenterDelegate = nullptr;
      NSTimer* userNotificationCenterPollTimer = nullptr;
      UNAuthorizationStatus __block currentUserNotificationAuthorizationStatus;
    #endif

      Mutex mutex;

      PermissionChangeObservers permissionChangeObservers;
      NotificationResponseObservers notificationResponseObservers;
      NotificationPresentedObservers notificationPresentedObservers;

      Notifications (Core* core);
      ~Notifications ();

      void configureWebView (WebView* webview);

      bool removePermissionChangeObserver (const PermissionChangeObserver& observer);
      bool addPermissionChangeObserver (
        const PermissionChangeObserver& observer,
        const PermissionChangeObserver::Callback callback
      );

      bool removeNotificationResponseObserver (const NotificationResponseObserver& observer);
      bool addNotificationResponseObserver (
        const NotificationResponseObserver& observer,
        const NotificationResponseObserver::Callback callback
      );

      bool removeNotificationPresentedObserver (const NotificationPresentedObserver& observer);
      bool addNotificationPresentedObserver (
        const NotificationPresentedObserver& observer,
        const NotificationPresentedObserver::Callback callback
      );

      bool show (const ShowOptions& options, const ShowCallback callback);
      bool close (const Notification& notification);
      void list (const ListCallback callback) const;
  };
}
#endif
