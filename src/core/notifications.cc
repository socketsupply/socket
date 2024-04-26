#include "notifications.hh"
#include "resource.hh"
#include "module.hh"
#include "debug.hh"
#include "url.hh"

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
  const JSON::Object Notifications::Notification::json () const {
    return JSON::Object::Entries {
      {"id", this->identifier}
    };
  }

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

  bool Notifications::show (const ShowOptions& options, const ShowCallback callback) {
  #if SSC_PLATFORM_APPLE
    if (options.id.size() == 0) {
      callback(ShowResult { "Missing 'id' in Notifications::ShowOptions" });
      return false;
    }

    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
      // XXX(@jwerle): release this?
    auto attachments = [NSMutableArray new];
      // XXX(@jwerle): release this?
    auto userInfo = [NSMutableDictionary new];
      // XXX(@jwerle): release this?
    auto content = [UNMutableNotificationContent new];
    auto __block id = options.id;

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
      NSError* error = nullptr;
      NSURL* iconURL = nullptr;

      const auto url = URL(options.icon);

      if (options.icon.starts_with("socket://")) {
        const auto path = FileResource::getResourcePath(url.pathname);
        iconURL = [NSURL fileURLWithPath: @(path.string().c_str())];
      } else {
        iconURL = [NSURL fileURLWithPath: @(url.href.c_str())];
      }

      const auto types = [UTType
            typesWithTag: iconURL.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      // XXX(@jwerle): release this?
      auto options = [NSMutableDictionary new];

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
        auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        callback(ShowResult { message });
        return false;
      }

      [attachments addObject: attachment];
    } else {
    // FIXME(): this define never is true
    #if SSC_PLATFORM_SANDBOXED
    // using an asset from the resources directory will require a code signed application
      NSError* error = nullptr;
      const auto path = FileResource::getResourcePath(String("icon.png"));
      const auto iconURL = [NSURL fileURLWithPath: @(path.string().c_str())];
      const auto types = [UTType
            typesWithTag: iconURL.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      // XXX(@jwerle): release this?
      auto options = [NSMutableDictionary new];
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
        return false;
      }

      [attachments addObject: attachment];
    #endif
    }

    if (options.image.size() > 0) {
      NSError* error = nullptr;
      NSURL* imageURL = nullptr;

      const auto url = URL(options.image);

      if (options.image.starts_with("socket://")) {
        const auto path = FileResource::getResourcePath(url.pathname);
        imageURL = [NSURL fileURLWithPath: @(path.string().c_str())];
      } else {
        imageURL = [NSURL fileURLWithPath: @(url.href.c_str())];
      }

      auto types = [UTType
            typesWithTag: imageURL.pathExtension
                tagClass: UTTagClassFilenameExtension
        conformingToType: nullptr
      ];

      auto options = [NSMutableDictionary new];

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
        auto message = String(
           error.localizedDescription.UTF8String != nullptr
             ? error.localizedDescription.UTF8String
             : "An unknown error occurred"
        );

        callback(ShowResult { message });
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

    [notificationCenter addNotificationRequest: request withCompletionHandler: ^(NSError* error) {
      if (error != nullptr) {
        auto message = String(
          error.localizedDescription.UTF8String != nullptr
            ? error.localizedDescription.UTF8String
            : "An unknown error occurred"
        );

        callback(ShowResult { message });
        return;
      }

      callback(ShowResult { "", id });
    }];
  #endif
    return false;
  }

  bool Notifications::close (const Notification& notification) {
  #if SSC_PLATFORM_APPLE
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
  #endif

    return false;
  }

  void Notifications::list (const ListCallback callback) const {
    auto notificationCenter = [UNUserNotificationCenter currentNotificationCenter];
    [notificationCenter getDeliveredNotificationsWithCompletionHandler: ^(NSArray<UNNotification*> *notifications) {
      Vector<Notification> entries;

      for (UNNotification* notification in notifications) {
        entries.push_back(Notification { notification.request.identifier.UTF8String });
      }

      callback(entries);
    }];
  }

#if SSC_PLATFORM_LINUX
  void Notifications::configureWebView (WebKitWebView* webview) {
    Lock lock(this->mutex);

    static bool areWebContextSignalsConnected = false;

    g_signal_connect(
      G_OBJECT(webview),
      "show-notification",
      G_CALLBACK(+[](
        WebKitWebView* webview,
        WebKitNotification* notification,
        gpointer userData
      ) -> bool {
        static auto windowManager = App::instance()->getWindowManager();

        if (windowManager == nullptr) {
          return false;
        }

        for (auto& window : windowManager->windows) {
          if (
            window != nullptr &&
            window->bridge != nullptr &&
            WEBKIT_WEB_VIEW(window->webview) == webview
           ) {
            auto userConfig = window->bridge->userConfig;
            return userConfig["permissions_allow_notifications"] != "false";
          }
        }

        return false;
      }),
      this
    );

    if (!areWebContextSignalsConnected) {
      auto webContext = webkit_web_context_get_default();

      areWebContextSignalsConnected = true;

      g_signal_connect(
        G_OBJECT(webContext),
        "initialize-notification-permissions",
        G_CALLBACK(+[](
          WebKitWebContext* webContext,
          gpointer userData
        ) {
          static auto userConfig = SSC::getUserConfig();
          static const auto bundleIdentifier = userConfig["meta_bundle_identifier"];

          const auto uri = "socket://" + bundleIdentifier;
          const auto origin = webkit_security_origin_new_for_uri(uri.c_str());

          GList* allowed = nullptr;
          GList* disallowed = nullptr;

          webkit_security_origin_ref(origin);

          if (origin && allowed && disallowed) {
            if (userConfig["permissions_allow_notifications"] == "false") {
              disallowed = g_list_append(disallowed, (gpointer) origin);
            } else {
              allowed = g_list_append(allowed, (gpointer) origin);
            }

            if (allowed && disallowed) {
              webkit_web_context_initialize_notification_permissions(
                webContext,
                allowed,
                disallowed
              );
            }
          }

          if (allowed) {
            g_list_free(allowed);
          }

          if (disallowed) {
            g_list_free(disallowed);
          }

          if (origin) {
            webkit_security_origin_unref(origin);
          }
        }),
        this
      );
    }
  }
#elif SSC_PLATFORM_WINDOWS
  void Notifications::configureWebView (ICoreWebView2* webview) {
  }
#endif
}
