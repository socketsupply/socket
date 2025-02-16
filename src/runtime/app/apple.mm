#include "../app.hh"

using namespace ssc::runtime;
using namespace ssc::runtime::app;
using ssc::runtime::window::Window;
using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::config::getDevHost;
using ssc::runtime::config::getDevPort;
using ssc::runtime::string::parseStringList;
using ssc::runtime::string::split;
using ssc::runtime::string::trim;
using ssc::runtime::msleep;

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.app.queue",
  qos
);

@implementation SSCApplicationDelegate
#if SOCKET_RUNTIME_PLATFORM_MACOS
- (void) applicationDidFinishLaunching: (NSNotification*) notification {
  self.statusItem = [NSStatusBar.systemStatusBar statusItemWithLength: NSVariableStatusItemLength];
}

- (void) applicationWillBecomeActive: (NSNotification*) notification {
  dispatch_async(queue, ^{
    self.app->resume();
  });
}

- (void) applicationWillResignActive: (NSNotification*) notification {
  dispatch_async(queue, ^{
    // self.app->pause();
  });
}

- (void) menuWillOpen: (NSMenu*) menu {
  auto app = self.app;
  auto window = app->runtime.windowManager.getWindow(0);
  if (!window) return;
  auto w = window->window;
  if (!w) return;

  [w makeKeyAndOrderFront: nil];
  [NSApp activateIgnoringOtherApps: YES];

  if (app != nullptr) {
    for (auto window : self.app->runtime.windowManager.windows) {
      if (window != nullptr) window->bridge->emit("tray", JSON::Object {});
    }
  }
}

- (void) application: (NSApplication*) application openURLs: (NSArray<NSURL*>*) urls {
  auto app = self.app;
  if (app != nullptr) {
    for (NSURL* url in urls) {
      for (auto& window : self.app->runtime.windowManager.windows) {
        if (window != nullptr) {
          window->handleApplicationURL(url.absoluteString.UTF8String);
        }
      }
    }
  }
}

- (BOOL) application: (NSApplication*) application
continueUserActivity: (NSUserActivity*) userActivity
  restorationHandler: (void (^)(NSArray*)) restorationHandler
{
  return [self
                         application: application
    willContinueUserActivityWithType: userActivity.activityType
  ];
}

              - (BOOL) application: (NSApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType
{
  static auto userConfig = getUserConfig();
  const auto webpageURL = application.userActivity.webpageURL;

  if (userActivityType == nullptr) {
    return NO;
  }

  if (webpageURL == nullptr) {
    return NO;
  }

  if (webpageURL.host == nullptr) {
    return NO;
  }

  const auto activityType = String(userActivityType.UTF8String);

  if (activityType != String(NSUserActivityTypeBrowsingWeb.UTF8String)) {
    return NO;
  }

  const auto host = String(webpageURL.host.UTF8String);
  const auto links = parseStringList(userConfig["meta_application_links"], ' ');

  if (links.size() == 0) {
    return NO;
  }

  bool exists = false;

  for (const auto& link : links) {
    const auto parts = split(link, '?');
    if (host == parts[0]) {
      exists = true;
      break;
    }
  }

  if (!exists) {
    return NO;
  }

  const auto url = String(webpageURL.absoluteString.UTF8String);
  bool emitted = false;

  for (auto& window : self.app->runtime.windowManager.windows) {
    if (window != nullptr) {
      window->handleApplicationURL(url);
      emitted = true;
    }
  }

  if (!emitted) {
    return NO;
  }

  return YES;
}

                 - (void) application: (NSApplication*) application
didFailToContinueUserActivityWithType: (NSString*) userActivityType
                                error: (NSError*) error {
  debug("application:didFailToContinueUserActivityWithType:error: %@", error);
}

#elif SOCKET_RUNTIME_PLATFORM_IOS
-           (BOOL) application: (UIApplication*) application
 didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
  auto const notificationCenter = NSNotificationCenter.defaultCenter;
  const auto frame = UIScreen.mainScreen.bounds;

  Vector<String> argv;
  bool isTest = false;

  self.app = App::sharedApplication();
  self.app->delegate = self;

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardDidShow:)
           name: UIKeyboardDidShowNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardDidHide:)
           name: UIKeyboardDidHideNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillShow:)
           name: UIKeyboardWillShowNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillHide:)
           name: UIKeyboardWillHideNotification
         object: nil
  ];

  [notificationCenter
    addObserver: self
       selector: @selector(keyboardWillChange:)
           name: UIKeyboardWillChangeFrameNotification
         object: nil
  ];

  for (const auto& arg : split(self.app->runtime.userConfig["ssc_argv"], ',')) {
    if (arg.find("--test") == 0) {
      isTest = true;
    }

    argv.push_back("'" + trim(arg) + "'");
  }

  auto windowManagerOptions = window::ManagerOptions {};

  for (const auto& arg : split(self.app->runtime.userConfig["ssc_argv"], ',')) {
    if (arg.find("--test") == 0) {
      windowManagerOptions.features.useTestScript = true;
    }

    windowManagerOptions.argv.push_back("'" + trim(arg) + "'");
  }


  windowManagerOptions.userConfig = self.app->runtime.userConfig;

  self.app->runtime.windowManager.configure(windowManagerOptions);

  static const auto port = getDevPort();
  static const auto host = getDevHost();

  auto defaultWindow = self.app->runtime.windowManager.createDefaultWindow(Window::Options {
     .shouldExitApplicationOnClose = true
  });

  defaultWindow->setTitle(self.app->runtime.userConfig["meta_title"]);

  self.app->runtime.serviceWorkerManager.init("socket://" + self.app->runtime.userConfig["meta_bundle_identifier"], {
    .userConfig = self.app->runtime.userConfig
  });

  if (isDebugEnabled() && port > 0 && host.size() > 0) {
    defaultWindow->navigate(host + ":" + std::to_string(port));
  } else if (self.app->runtime.userConfig["webview_root"].size() != 0) {
    defaultWindow->navigate(
      "socket://" + self.app->runtime.userConfig["meta_bundle_identifier"] + self.app->runtime.userConfig["webview_root"]
    );
  } else {
    defaultWindow->navigate(
      "socket://" + self.app->runtime.userConfig["meta_bundle_identifier"] + "/index.html"
    );
  }

  self.app->dispatch([defaultWindow](){
    msleep(64);
    defaultWindow->show();
  });

  return YES;
}

- (void) applicationDidEnterBackground: (UIApplication*) application {
  for (const auto& window : self.app->runtime.windowManager.windows) {
    if (window != nullptr) {
      window->eval("window.blur()");
    }
  }
}

- (void) applicationWillEnterForeground: (UIApplication*) application {
  for (const auto& window : self.app->runtime.windowManager.windows) {
    if (window != nullptr) {
      if (!window->webview.isHidden) {
        window->eval("window.focus()");
      }
    }
  }

  for (const auto& window : self.app->runtime.windowManager.windows) {
    if (window != nullptr) {
      // XXX(@jwerle): maybe bluetooth should be a singleton instance with observers
      window->bridge->bluetooth.startScanning();
    }
  }
}

- (void) applicationWillTerminate: (UIApplication*) application {
  dispatch_async(queue, ^{
    // TODO(@jwerle): what should we do here?
    // self.app->stop();
  });
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self.app->resume();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    // XXX(@jwerle): on iOS, we do not pause the runtime
    // self.app->pause();
  });
}

-  (BOOL) application: (UIApplication*) application
 continueUserActivity: (NSUserActivity*) userActivity
   restorationHandler: (void (^)(NSArray<id<UIUserActivityRestoring>>*)) restorationHandler
{
  return [self
                         application: application
    willContinueUserActivityWithType: userActivity.activityType
  ];
}

              - (BOOL) application: (UIApplication*) application
  willContinueUserActivityWithType: (NSString*) userActivityType
{
  static auto userConfig = getUserConfig();
  const auto webpageURL = application.userActivity.webpageURL;

  if (userActivityType == nullptr) {
    return NO;
  }

  if (webpageURL == nullptr) {
    return NO;
  }

  if (webpageURL.host == nullptr) {
    return NO;
  }

  const auto activityType = String(userActivityType.UTF8String);

  if (activityType != String(NSUserActivityTypeBrowsingWeb.UTF8String)) {
    return NO;
  }

  const auto host = String(webpageURL.host.UTF8String);
  const auto links = parseStringList(userConfig["meta_application_links"], ' ');

  if (links.size() == 0) {
    return NO;
  }

  bool exists = false;

  for (const auto& link : links) {
    const auto parts = split(link, '?');
    if (host == parts[0]) {
      exists = true;
      break;
    }
  }

  if (!exists) {
    return NO;
  }

  bool emitted = false;
  for (const auto& window : self.app->runtime.windowManager.windows) {
    if (window != nullptr) {
      window->handleApplicationURL(webpageURL.absoluteString.UTF8String);
      emitted = true;
    }
  }

  return emitted;
}

- (void) keyboardWillHide: (NSNotification*) notification {
  const auto info = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto height = rect.size.height;

  const auto window = self.app->runtime.windowManager.getWindow(0);
  window->webview.scrollView.scrollEnabled = YES;

  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window) {
      window->bridge->emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "will-hide"},
          {"height", height}
        }}
      });
    }
  }
}

- (void) keyboardDidHide: (NSNotification*) notification {
  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window) {
      window->bridge->emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "did-hide"}
        }}
      });
    }
  }
}

- (void)keyboardWillShow: (NSNotification*) notification {
  const auto info = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue*) [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto height = rect.size.height;

  const auto window = self.app->runtime.windowManager.getWindow(0);
  window->webview.scrollView.scrollEnabled = NO;

  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge->emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
        {"event", "will-show"},
        {"height", height}
        }}
      });
    }
  }
}

- (void) keyboardDidShow: (NSNotification*) notification {
  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge->emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "did-show"}
        }}
      });
    }
  }
}

- (void) keyboardWillChange: (NSNotification*) notification {
  const auto keyboardInfo = notification.userInfo;
  const auto keyboardFrameBegin = (NSValue* )[keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  const auto rect = [keyboardFrameBegin CGRectValue];
  const auto width = rect.size.width;
  const auto height = rect.size.height;

  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window && !window->window.isHidden) {
      window->bridge->emit("keyboard", JSON::Object::Entries {
        {"value", JSON::Object::Entries {
          {"event", "will-change"},
          {"width", width},
          {"height", height},
        }}
      });
    }
  }
}

- (BOOL) application: (UIApplication*) app
             openURL: (NSURL*) url
             options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*) options
{
  for (const auto window : self.app->runtime.windowManager.windows) {
    if (window) {
      window->handleApplicationURL(url.absoluteString.UTF8String);
      return YES;
    }
  }

  return NO;
}
#endif
@end
