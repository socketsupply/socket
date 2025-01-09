#include "../../filesystem.hh"
#include "../../debug.hh"
#include "../../core.hh"
#include "../../url.hh"

#include "notifications.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
@implementation SSCUserNotificationCenterDelegate
-  (void) userNotificationCenter: (UNUserNotificationCenter*) center
  didReceiveNotificationResponse: (UNNotificationResponse*) response
           withCompletionHandler: (void (^)(void)) completionHandler
{
  using namespace ssc::runtime;

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
  using namespace ssc::runtime;
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

namespace ssc::runtime::core::services {
  const JSON::Object Notifications::Notification::json () const {
    return JSON::Object::Entries {
      {"id", this->identifier}
    };
  }

  Notifications::Notifications (const Options& options)
    : core::Service(options),
      permissions(options),
      permissionChangeObservers(),
      notificationResponseObservers(),
      notificationPresentedObservers()
  {}

  Notifications::~Notifications () {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      if (!this->enabled) return;

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

  bool Notifications::start () {
    if (!this->enabled) {
      return false;
    }

    this->stop();
   #if SOCKET_RUNTIME_PLATFORM_APPLE
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

    return true;
  }

  bool Notifications::stop () {
    if (!this->enabled) {
      return false;
    }

   #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->userNotificationCenterPollTimer) {
      [this->userNotificationCenterPollTimer invalidate];
      this->userNotificationCenterPollTimer = nullptr;
    }
   #endif

    return true;
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

  bool Notifications::show (const ShowOptions& options, const ShowCallback callback) {
    if (options.id.size() == 0) {
      callback(ShowResult { "Missing 'id' in Notifications::ShowOptions" });
      return false;
    }

    if (!this->permissions.hasRuntimePermission("notifications")) {
      callback(ShowResult { "Runtime permission is disabled for 'notifications'" });
      return false;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    auto attachments = [NSMutableArray array];
    auto userInfo = [NSMutableDictionary dictionary];
    auto content = [UNMutableNotificationContent new];

    NSError* error = nullptr;

    auto id = options.id;

    if (options.tag.size() > 0) {
      userInfo[@"tag"]  = @(options.tag.c_str());
      content.threadIdentifier = @(options.tag.c_str());
    }

    if (options.lang.size() > 0) {
      userInfo[@"lang"]  = @(options.lang.c_str());
    }

    if (options.silent == false) {
      content.sound = [UNNotificationSound defaultSound];
    }

    if (options.icon.size() > 0) {
      NSURL* iconURL = nullptr;

      const auto url = URL(options.icon);

      if (options.icon.starts_with("socket://")) {
        const auto path = filesystem::Resource::getResourcePath(url.pathname);
        iconURL = [NSURL fileURLWithPath: @(path.string().c_str())];
      } else {
        iconURL = [NSURL fileURLWithPath: @(url.href().c_str())];
      }

      const auto types = [UTType
        typesWithTag: iconURL.pathExtension
        tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary dictionary];

      if (types.count > 0) {
        options[UNNotificationAttachmentOptionsTypeHintKey] = types.firstObject.preferredMIMEType;
      };

      auto attachment = [UNNotificationAttachment
        attachmentWithIdentifier: @("")
        URL: iconURL
        options: options
        error: &error
      ];

      if (error != nullptr) {
        const auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        callback(ShowResult { message });
      #if !__has_feature(objc_arc)
        [content release];
      #endif
        return false;
      }

      [attachments addObject: attachment];
    } else {
      // using an asset from the resources directory will require a code signed application
      const auto path = filesystem::Resource::getResourcePath(String("icon.png"));

      if (filesystem::Resource(path).exists()) {
        const auto iconURL = [NSURL fileURLWithPath: @(path.string().c_str())];
        const auto types = [UTType
          typesWithTag: iconURL.pathExtension
          tagClass: UTTagClassFilenameExtension
          conformingToType: nullptr
        ];

        auto options = [NSMutableDictionary dictionary];
        auto attachment = [UNNotificationAttachment
          attachmentWithIdentifier: @("")
          URL: iconURL
          options: options
          error: &error
        ];

        if (error != nullptr) {
          auto message = String(
            error.localizedDescription.UTF8String != nullptr
              ? error.localizedDescription.UTF8String
              : "An unknown error occurred"
          );

          callback(ShowResult { message });
        #if !__has_feature(objc_arc)
          [content release];
        #endif
          return false;
        }

        [attachments addObject: attachment];
      }
    }

    if (options.image.size() > 0) {
      NSError* error = nullptr;
      NSURL* imageURL = nullptr;

      const auto url = URL(options.image);

      if (options.image.starts_with("socket://")) {
        const auto path = filesystem::Resource::getResourcePath(url.pathname);
        imageURL = [NSURL fileURLWithPath: @(path.string().c_str())];
      } else {
        imageURL = [NSURL fileURLWithPath: @(url.href().c_str())];
      }

      const auto types = [UTType
        typesWithTag: imageURL.pathExtension
        tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary dictionary];

      if (types.count > 0) {
        options[UNNotificationAttachmentOptionsTypeHintKey] = types.firstObject.preferredMIMEType;
      };

      auto attachment = [UNNotificationAttachment
        attachmentWithIdentifier: @("")
        URL: imageURL
        options: options
        error: &error
      ];

      if (error != nullptr) {
        const auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        callback(ShowResult { message });
      #if !__has_feature(objc_arc)
        [content release];
      #endif
        return false;
      }

      [attachments addObject: attachment];
    }

    content.attachments = attachments;
    content.userInfo = userInfo;
    content.title = @(options.title.c_str());
    content.body = @(options.body.c_str());

    auto request = [UNNotificationRequest
      requestWithIdentifier: @(id.c_str())
      content: content
      trigger: nil
    ];

    auto cb = callback;
    [notificationCenter addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
      if (error != nullptr) {
        const auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        this->loop.dispatch([=] () {
          cb(ShowResult { message });
        });
      #if !__has_feature(objc_arc)
        [content release];
      #endif
        return;
      }

      this->loop.dispatch([=] () {
        cb(ShowResult { "", id });
      });
    }];

    return true;
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    const auto attachment = Android::JNIEnvironmentAttachment(this->dispatcher.jvm);
    // `activity.showNotification(
    //    id,
    //    title,
    //    body,
    //    tag,
    //    channel,
    //    category,
    //    silent,
    //    iconURL,
    //    imageURL,
    //    vibratePattern
    // )`
    const auto success = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      this->dispatcher.activity,
      "showNotification",
      "("
        "Ljava/lang/String;" // id
        "Ljava/lang/String;" // title
        "Ljava/lang/String;" // body
        "Ljava/lang/String;" // tag
        "Ljava/lang/String;" // channel
        "Ljava/lang/String;" // category
        "Z" // silent
        "Ljava/lang/String;" // iconURL
        "Ljava/lang/String;" // imageURL
        "Ljava/lang/String;" // vibratePattern
      ")Z",
      attachment.env->NewStringUTF(options.id.c_str()),
      attachment.env->NewStringUTF(options.title.c_str()),
      attachment.env->NewStringUTF(options.body.c_str()),
      attachment.env->NewStringUTF(options.tag.c_str()),
      attachment.env->NewStringUTF(options.channel.c_str()),
      attachment.env->NewStringUTF(options.category.c_str()),
      options.silent,
      attachment.env->NewStringUTF(options.icon.c_str()),
      attachment.env->NewStringUTF(options.image.c_str()),
      attachment.env->NewStringUTF(options.vibrate.c_str())
    );

    this->loop.dispatch([=, this] () {
      callback(ShowResult { "", options.id });
      this->notificationPresentedObservers.dispatch(JSON::Object::Entries {
        {"id", options.id}
      });
    });
    return success;
  #endif
    return false;
  }

  bool Notifications::close (const Notification& notification) {
    if (!this->permissions.hasRuntimePermission("notifications")) {
      return false;
    }
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    const auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    const auto identifiers = @[@(notification.identifier.c_str())];
    const auto json = JSON::Object::Entries {
      {"id", notification.identifier},
      {"action", "dismiss"}
    };

    [notificationCenter removePendingNotificationRequestsWithIdentifiers: identifiers];
    [notificationCenter removeDeliveredNotificationsWithIdentifiers: identifiers];

    this->notificationResponseObservers.dispatch(json);
    return true;
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    const auto attachment = Android::JNIEnvironmentAttachment(this->dispatcher.jvm);
    // `activity.showNotification(
    //    id,
    //    tag,
    // )`
    const auto success = CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Boolean,
      this->dispatcher.activity,
      "closeNotification",
      "("
        "Ljava/lang/String;" // id
        "Ljava/lang/String;" // tag
      ")Z",
      attachment.env->NewStringUTF(notification.identifier.c_str()),
      attachment.env->NewStringUTF(notification.tag.c_str())
    );

    this->loop.dispatch([=, this] () {
      const auto json = JSON::Object::Entries {
        {"id", notification.identifier},
        {"action", "dismiss"}
      };

      this->notificationResponseObservers.dispatch(json);
    });
    return success;
  #endif

    return false;
  }

  void Notifications::list (const ListCallback callback) const {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    [notificationCenter getDeliveredNotificationsWithCompletionHandler: ^(NSArray<UNNotification*> *notifications) {
      Vector<Notification> entries;

      for (UNNotification* notification in notifications) {
        entries.push_back(Notification { notification.request.identifier.UTF8String });
      }

      callback(entries);
    }];
  #else
    Vector<Notification> entries;
    callback(entries);
  #endif
  }
}
