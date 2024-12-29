#ifndef SOCKET_RUNTIME_CORE_SERVICES_NOTIFICATIONS_H
#define SOCKET_RUNTIME_CORE_SERVICES_NOTIFICATIONS_H

#include "../../core.hh"

#include "permissions.hh"

namespace ssc::runtime::core::services {
  class Notifications;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCUserNotificationCenterDelegate : NSObject<UNUserNotificationCenterDelegate>
@property (nonatomic) ssc::runtime::core::services::Notifications* notifications;
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler;

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler;
@end
#endif

namespace ssc::runtime::core::services {
  class Notifications : public core::Service {
    public:
      using PermissionChangeObserver = Observer<JSON::Object>;
      using PermissionChangeObservers = Observers<PermissionChangeObserver>;
      using NotificationResponseObserver = Observer<JSON::Object>;
      using NotificationResponseObservers = Observers<NotificationResponseObserver>;
      using NotificationPresentedObserver = Observer<JSON::Object>;
      using NotificationPresentedObservers = Observers<NotificationPresentedObserver>;

      struct Notification {
        String identifier;
        String tag;
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
        String channel;
        String category;
        String vibrate;
      };

      struct ShowResult {
        String error = "";
        Notification notification;
      };

      using ShowCallback = Function<void(const ShowResult)>;
      using ListCallback = Function<void(const Vector<Notification>)>;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCUserNotificationCenterDelegate* userNotificationCenterDelegate = nullptr;
      NSTimer* userNotificationCenterPollTimer = nullptr;
      UNAuthorizationStatus __block currentUserNotificationAuthorizationStatus;
    #endif

      Permissions permissions;
      Mutex mutex;

      PermissionChangeObservers permissionChangeObservers;
      NotificationResponseObservers notificationResponseObservers;
      NotificationPresentedObservers notificationPresentedObservers;

      Notifications (const Options&);
      ~Notifications ();

      bool start ();
      bool stop ();

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
