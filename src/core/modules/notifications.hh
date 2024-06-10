#ifndef SOCKET_RUNTIME_CORE_MODULE_NOTIFICATIONS_H
#define SOCKET_RUNTIME_CORE_MODULE_NOTIFICATIONS_H

#include "../module.hh"

namespace SSC {
  class CoreNotifications;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCUserNotificationCenterDelegate : NSObject<UNUserNotificationCenterDelegate>
@property (nonatomic) SSC::CoreNotifications* notifications;
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler;

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler;
@end
#endif

namespace SSC {
  class CoreNotifications : public CoreModule {
    public:
      using PermissionChangeObserver = CoreModule::Observer<JSON::Object>;
      using PermissionChangeObservers = CoreModule::Observers<PermissionChangeObserver>;
      using NotificationResponseObserver = CoreModule::Observer<JSON::Object>;
      using NotificationResponseObservers = CoreModule::Observers<NotificationResponseObserver>;
      using NotificationPresentedObserver = CoreModule::Observer<JSON::Object>;
      using NotificationPresentedObservers = CoreModule::Observers<NotificationPresentedObserver>;

      bool isUtility = false;

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

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCUserNotificationCenterDelegate* userNotificationCenterDelegate = nullptr;
      NSTimer* userNotificationCenterPollTimer = nullptr;
      UNAuthorizationStatus __block currentUserNotificationAuthorizationStatus;
    #endif

      Mutex mutex;

      PermissionChangeObservers permissionChangeObservers;
      NotificationResponseObservers notificationResponseObservers;
      NotificationPresentedObservers notificationPresentedObservers;

      CoreNotifications (Core* core, bool isUtility);
      ~CoreNotifications ();

      bool removePermissionChangeObserver (const PermissionChangeObserver& observer);
      bool addPermissionChangeObserver (
        const PermissionChangeObserver& observer,
        const PermissionChangeObserver::Callback& callback
      );

      bool removeNotificationResponseObserver (const NotificationResponseObserver& observer);
      bool addNotificationResponseObserver (
        const NotificationResponseObserver& observer,
        const NotificationResponseObserver::Callback& callback
      );

      bool removeNotificationPresentedObserver (const NotificationPresentedObserver& observer);
      bool addNotificationPresentedObserver (
        const NotificationPresentedObserver& observer,
        const NotificationPresentedObserver::Callback& callback
      );

      bool show (const ShowOptions& options, const ShowCallback& callback);
      bool close (const Notification& notification);
      void list (const ListCallback& callback) const;
  };
}
#endif
