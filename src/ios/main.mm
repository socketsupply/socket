#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "../window/window.hh"
#include "../cli/cli.hh"

using namespace SSC;

constexpr auto _debug = false;

static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.app.queue",
  qos
);

@interface AppDelegate : UIResponder <
  UIApplicationDelegate,
  WKScriptMessageHandler,
  UIScrollViewDelegate
> {
  SSC::IPC::Bridge* bridge;
  Core* core;
}
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) SSCNavigationDelegate* navDelegate;
@property (strong, nonatomic) SSCBridgedWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) applicationDidEnterBackground: (UIApplication*) application {
  [self.webview evaluateJavaScript: @"window.blur()" completionHandler: nil];
}

- (void) applicationWillEnterForeground: (UIApplication*) application {
  [self.webview evaluateJavaScript: @"window.focus()" completionHandler: nil];
  bridge->bluetooth.startScanning();
}

- (void) applicationWillTerminate: (UIApplication*) application {
  // @TODO(jwerle): what should we do here?
}

- (void) applicationDidBecomeActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self->core->resumeAllPeers();
    self->core->runEventLoop();
  });
}

- (void) applicationWillResignActive: (UIApplication*) application {
  dispatch_async(queue, ^{
    self->core->stopEventLoop();
    self->core->pauseAllPeers();
  });
}

//
// When a message is received try to route it.
// Messages may also be received and routed via the custom scheme handler.
//
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  id body = [scriptMessage body];

  if (![body isKindOfClass:[NSString class]]) {
    return;
  }

  auto msg = IPC::Message([body UTF8String]);

  if (msg.name == "application.exit" || msg.name == "process.exit") {
    auto code = std::stoi(msg.get("value", "0"));

    if (code > 0) {
      CLI::notify(SIGTERM);
    } else {
      CLI::notify(SIGUSR2);
    }
  } else {
    bridge->route([body UTF8String], nullptr, 0);
  }
}

- (void) keyboardWillHide: (NSNotification*) notification {
  NSDictionary *info = [notification userInfo];
  NSValue* keyboardFrameBegin = [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat height = rect.size.height;

  JSON::Object json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-hide"},
      {"height", height}
    }} 
  };

  self.webview.scrollView.scrollEnabled = YES;
  bridge->router.emit("keyboard", json.str());
}

- (void) keyboardDidHide: (NSNotification*) notification {

  JSON::Object json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-hide"}
    }}
  };

  bridge->router.emit("keyboard", json.str());
}

- (void) keyboardWillShow: (NSNotification*) notification {
  NSDictionary *info = [notification userInfo];
  NSValue* keyboardFrameBegin = [info valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat height = rect.size.height;

  JSON::Object json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-show"},
      {"height", height},
    }}
  };

  self.webview.scrollView.scrollEnabled = NO;
  bridge->router.emit("keyboard", json.str());
}

- (void) keyboardDidShow: (NSNotification*) notification {
  JSON::Object json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-show"}
    }}
  };

  bridge->router.emit("keyboard", json.str());
}

- (void) keyboardWillChange: (NSNotification*) notification {
  NSDictionary* keyboardInfo = [notification userInfo];
  NSValue* keyboardFrameBegin = [keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat width = rect.size.width;
  CGFloat height = rect.size.height;

  JSON::Object json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-change"},
      {"width", width},
      {"height", height},
    }}
  };

  bridge->router.emit("keyboard", json.str());
}

- (void) scrollViewDidScroll: (UIScrollView*) scrollView {
  scrollView.bounds = self.webview.bounds;
}

- (BOOL) application: (UIApplication*) app
             openURL: (NSURL*) url
             options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*) options
{
  // TODO can this be escaped or is the url encoded property already?
  JSON::Object json = JSON::Object::Entries {{"url", [url.absoluteString UTF8String]}};
  bridge->router.emit("protocol", json.str());
  return YES;
}

-           (BOOL) application: (UIApplication*) application
 didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
  using namespace SSC;

  core = new Core;
  bridge = new IPC::Bridge(core);
  bridge->router.dispatchFunction = [=] (auto callback) {
    dispatch_async(queue, ^{ callback(); });
  };

  bridge->router.evaluateJavaScriptFunction = [=](auto js) {
    dispatch_async(dispatch_get_main_queue(), ^{
      auto script = [NSString stringWithUTF8String: js.c_str()];
      [self.webview evaluateJavaScript: script completionHandler: nil];
    });
  };

  auto appFrame = [[UIScreen mainScreen] bounds];

  self.window = [[UIWindow alloc] initWithFrame: appFrame];

  UIViewController *viewController = [[UIViewController alloc] init];
  viewController.view.frame = appFrame;
  self.window.rootViewController = viewController;

  auto userConfig = SSC::getUserConfig();

  StringStream env;

  for (auto const &envKey : parseStringList(userConfig["build_env"])) {
    auto cleanKey = trim(envKey);

    if (!Env::has(cleanKey)) {
      continue;
    }

    auto envValue = Env::get(cleanKey.c_str());

    env << String(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  env << String("width=" + std::to_string(appFrame.size.width) + "&");
  env << String("height=" + std::to_string(appFrame.size.height) + "&");

  NSString* resourcePath = [[NSBundle mainBundle] resourcePath];
  NSString* cwd = [resourcePath stringByAppendingPathComponent: @"ui"];
  const auto argv = userConfig["ssc_argv"];

  uv_chdir(cwd.UTF8String);

  WindowOptions opts {
    .debug = isDebugEnabled(),
    .isTest = argv.find("--test") != -1,
    .argv = argv,
    .env = env.str(),
    .appData = userConfig
  };

  // Note: you won't see any logs in the preload script before the
  // Web Inspector is opened
  String  preload = createPreload(opts);

  WKUserScript* initScript = [[WKUserScript alloc]
    initWithSource: [NSString stringWithUTF8String: preload.c_str()]
    injectionTime: WKUserScriptInjectionTimeAtDocumentStart
    forMainFrameOnly: NO];

  WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

  [config setURLSchemeHandler: bridge->router.schemeHandler
                 forURLScheme: @"ipc"];

  [config setURLSchemeHandler: bridge->router.schemeHandler
                 forURLScheme: @"socket"];

  self.content = [config userContentController];

  [self.content addScriptMessageHandler:self name: @"external"];
  [self.content addUserScript: initScript];

  self.webview = [[SSCBridgedWebView alloc] initWithFrame: appFrame configuration: config];
  self.webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  WKPreferences* prefs = self.webview.configuration.preferences;

  [self.webview.configuration
    setValue: @YES
      forKey: @"allowUniversalAccessFromFileURLs"
  ];

  [self.webview.configuration.preferences
    setValue: @YES
      forKey: @"allowFileAccessFromFileURLs"
  ];

  [self.webview.configuration.preferences
    setValue: @YES
      forKey: @"javaScriptEnabled"
  ];

  if (userConfig["permissions_allow_fullscreen"] == "false") {
    [prefs setValue: @NO forKey: @"fullScreenEnabled"];
  } else {
    [prefs setValue: @YES forKey: @"fullScreenEnabled"];
  }

  if (SSC::isDebugEnabled()) {
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    if (@available(macOS 13.3, iOS 16.4, tvOS 16.4, *)) {
      [self.webview setInspectable: YES];
    }
  }

  if (userConfig["permissions_allow_clipboard"] == "false") {
    [prefs setValue: @NO forKey: @"javaScriptCanAccessClipboard"];
  } else {
    [prefs setValue: @YES forKey: @"javaScriptCanAccessClipboard"];
  }

  if (userConfig["permissions_allow_data_access"] == "false") {
    [prefs setValue: @NO forKey: @"storageAPIEnabled"];
  } else {
    [prefs setValue: @YES forKey: @"storageAPIEnabled"];
  }

  if (userConfig["permissions_allow_device_orientation"] == "false") {
    [prefs setValue: @NO forKey: @"deviceOrientationEventEnabled"];
  } else {
    [prefs setValue: @YES forKey: @"deviceOrientationEventEnabled"];
  }

  if (userConfig["permissions_allow_notifications"] == "false") {
    [prefs setValue: @NO forKey: @"appBadgeEnabled"];
    [prefs setValue: @NO forKey: @"notificationsEnabled"];
    [prefs setValue: @NO forKey: @"notificationEventEnabled"];
  } else {
    [prefs setValue: @YES forKey: @"appBadgeEnabled"];
    [prefs setValue: @YES forKey: @"notificationsEnabled"];
    [prefs setValue: @YES forKey: @"notificationEventEnabled"];
  }

  NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
  [ns addObserver: self selector: @selector(keyboardDidShow:) name: UIKeyboardDidShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardDidHide:) name: UIKeyboardDidHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillShow:) name: UIKeyboardWillShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillHide:) name: UIKeyboardWillHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillChange:) name: UIKeyboardWillChangeFrameNotification object: nil];

  self.navDelegate = [[SSCNavigationDelegate alloc] init];
  [self.webview setNavigationDelegate: self.navDelegate];

  [viewController.view addSubview: self.webview];

  NSString* allowed = [[NSBundle mainBundle] resourcePath];
  NSURL* url;
  int port = getDevPort();

  if (@available(iOS 16.4, *)) {
    if (isDebugEnabled()) {
      [self.webview setInspectable: YES];
    }
  }

  if (isDebugEnabled() && port > 0) {
    static const auto devHost = getDevHost();
    NSString* host = [NSString stringWithUTF8String: devHost.c_str()];
    url = [NSURL
      URLWithString: [NSString stringWithFormat: @"%@:%d/", host, port]
    ];

    if (@available(iOS 15, *)) {
      [self.webview loadFileRequest: [NSURLRequest requestWithURL: url]
            allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
      ];
    } else {
      [self.webview loadRequest: [NSURLRequest requestWithURL: url]];
    }
  } else {
    if (userConfig["webview_root"].size() != 0) {
      url = [NSURL URLWithString: @(("socket://" + userConfig["meta_bundle_identifier"] + userConfig["webview_root"]).c_str())];
    } else {
      url = [NSURL URLWithString: @(("socket://" + userConfig["meta_bundle_identifier"] + "/index.html").c_str())];
    }

    auto request = [NSMutableURLRequest requestWithURL: url];
    [self.webview loadRequest: request];
  }

  self.webview.scrollView.delegate = self;
  [self.window makeKeyAndVisible];

  return YES;
}
@end

int main (int argc, char *argv[]) {
  struct rlimit limit;
  getrlimit(RLIMIT_NOFILE, &limit);
  limit.rlim_cur = 2048;
  setrlimit(RLIMIT_NOFILE, &limit);

  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
