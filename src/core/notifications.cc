#include "notifications.hh"
#include "module.hh"

#if SSC_PLATFORM_APPLE
@implementation SSCUserNotificationCenterDelegate
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler
{
  using namespace SSC;

  const auto id = String(response.notification.request.identifier.UTF8String);
  const auto action = (
    [response.actionIdentifier isEqualToString: UNNotificationDefaultActionIdentifier]
      ? "default"
      : "dismiss"
  );

  const auto json = JSON::Object::Entries {
    {"id", id},
    {"action", action}
  };

  self.notifications->notificationResponseObservers.dispatch(json);

  completionHandler();
}

- (void) userNotificationCenter: (UNUserNotificationCenter*) center
        willPresentNotification: (UNNotification*) notification
          withCompletionHandler: (void (^)(UNNotificationPresentationOptions options)) completionHandler
{
  using namespace SSC;
  UNNotificationPresentationOptions options = UNNotificationPresentationOptionList;
  const auto __block id = String(notification.request.identifier.UTF8String);

  if (notification.request.content.sound != nullptr) {
    options |= UNNotificationPresentationOptionSound;
  }

  if (notification.request.content.attachments != nullptr) {
    if (notification.request.content.attachments.count > 0) {
      options |= UNNotificationPresentationOptionBanner;
    }
  }

  self.notifications->notificationPresentedObservers.dispatch(JSON::Object::Entries {
    {"id", id}
  });

  // look for dismissed notification
  auto timer = [NSTimer timerWithTimeInterval: 2 repeats: YES block: ^(NSTimer* timer) {
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    // if notification that was presented is not in the delivered notifications then
    // then notify that the notification was "dismissed"
    [notificationCenter getDeliveredNotificationsWithCompletionHandler: ^(NSArray<UNNotification*> *notifications) {
      for (UNNotification* notification in notifications) {
        if (String(notification.request.identifier.UTF8String) == id) {
          return;
        }
      }

      [timer invalidate];

      self.notifications->notificationResponseObservers.dispatch(JSON::Object::Entries {
        {"id", id},
        {"action", "dismiss"}
      });
    }];
  }];

  [NSRunLoop.mainRunLoop
    addTimer: timer
     forMode: NSDefaultRunLoopMode
  ];
}
@end
#endif

namespace SSC {
  Notifications::Notifications (Core* core)
    : Module(core),
      permissionChangeObservers(),
      notificationResponseObservers(),
      notificationPresentedObservers()
  {
  #if SSC_PLATFORM_APPLE
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];

    this->userNotificationCenterDelegate = [SSCUserNotificationCenterDelegate new];
    this->userNotificationCenterDelegate.notifications = this;

    if (!notificationCenter.delegate) {
      notificationCenter.delegate = this->userNotificationCenterDelegate;
    }

    [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
      this->currentUserNotificationAuthorizationStatus = settings.authorizationStatus;
      this->userNotificationCenterPollTimer = [NSTimer timerWithTimeInterval: 2 repeats: YES block: ^(NSTimer* timer) {
        // look for authorization status changes
        [notificationCenter getNotificationSettingsWithCompletionHandler: ^(UNNotificationSettings *settings) {
          JSON::Object json;
          if (this->currentUserNotificationAuthorizationStatus != settings.authorizationStatus) {
            this->currentUserNotificationAuthorizationStatus = settings.authorizationStatus;

            if (settings.authorizationStatus == UNAuthorizationStatusDenied) {
              json = JSON::Object::Entries {{"state", "denied"}};
            } else if (settings.authorizationStatus == UNAuthorizationStatusNotDetermined) {
              json = JSON::Object::Entries {{"state", "prompt"}};
            } else {
              json = JSON::Object::Entries {{"state", "granted"}};
            }

            this->permissionChangeObservers.dispatch(json);
          }
        }];
      }];

      [NSRunLoop.mainRunLoop
        addTimer: this->userNotificationCenterPollTimer
          forMode: NSDefaultRunLoopMode
      ];
    }];
  #endif
  }

  Notifications::~Notifications () {
  #if SSC_PLATFORM_APPLE
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];

    if (notificationCenter.delegate == this->userNotificationCenterDelegate) {
      notificationCenter.delegate = nullptr;
    }

    [this->userNotificationCenterPollTimer invalidate];

  #if !__has_feature(objc_arc)
    [this->userNotificationCenterDelegate release];
  #endif

    this->userNotificationCenterDelegate = nullptr;
    this->userNotificationCenterPollTimer = nullptr;
  #endif
  }

  bool Notifications::addPermissionChangeObserver (
    const PermissionChangeObserver& observer,
    const PermissionChangeObserver::Callback callback
  ) {
    return this->permissionChangeObservers.add(observer, callback);
  }

  bool Notifications::removePermissionChangeObserver (const PermissionChangeObserver& observer) {
    return this->permissionChangeObservers.remove(observer);
  }

  bool Notifications::addNotificationResponseObserver (
    const NotificationResponseObserver& observer,
    const NotificationResponseObserver::Callback callback
  ) {
    return this->notificationResponseObservers.add(observer, callback);
  }

  bool Notifications::removeNotificationResponseObserver (const NotificationResponseObserver& observer) {
    return this->notificationResponseObservers.remove(observer);
  }

  bool Notifications::addNotificationPresentedObserver (
    const NotificationPresentedObserver& observer,
    const NotificationPresentedObserver::Callback callback
  ) {
    return this->notificationPresentedObservers.add(observer, callback);
  }

  bool Notifications::removeNotificationPresentedObserver (const NotificationPresentedObserver& observer) {
    return this->notificationPresentedObservers.remove(observer);
  }
}
