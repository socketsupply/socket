#include "../core/core.hh"
#include "../ipc/ipc.hh"
#include "../window/window.hh"

using namespace SSC;

constexpr auto _settings = STR_VALUE(SSC_SETTINGS);
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

  bridge->route([body UTF8String], nullptr, 0);
}

- (void) keyboardWillHide {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-hide"}
    }}
  };

  self.webview.scrollView.scrollEnabled = YES;
  bridge->router.emit("keyboard", JSON::Object(json).str());
}

- (void) keyboardDidHide {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-hide"}
    }}
  };

  bridge->router.emit("keyboard", JSON::Object(json).str());
}

- (void) keyboardWillShow {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-show"}
    }}
  };

  self.webview.scrollView.scrollEnabled = NO;
  bridge->router.emit("keyboard", JSON::Object(json).str());
}

- (void) keyboardDidShow {
  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "did-show"}
    }}
  };

  bridge->router.emit("keyboard", JSON::Object(json).str());
}

- (void) keyboardWillChange: (NSNotification*) notification {
  NSDictionary* keyboardInfo = [notification userInfo];
  NSValue* keyboardFrameBegin = [keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat width = rect.size.width;
  CGFloat height = rect.size.height;

  auto json = JSON::Object::Entries {
    {"value", JSON::Object::Entries {
      {"event", "will-change"},
      {"width", width},
      {"height", height},
    }}
  };

  bridge->router.emit("keyboard", JSON::Object(json).str());
}

- (void) scrollViewDidScroll: (UIScrollView*) scrollView {
  scrollView.bounds = self.webview.bounds;
}

- (BOOL) application: (UIApplication*) app
             openURL: (NSURL*) url
             options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*) options
{
  // TODO can this be escaped or is the url encoded property already?
  auto json = JSON::Object::Entries {{"url", [url.absoluteString UTF8String]}};
  bridge->router.emit("protocol", JSON::Object(json).str());
  return YES;
}

-           (BOOL) application: (UIApplication*) application
 didFinishLaunchingWithOptions: (NSDictionary*) launchOptions
{
  using namespace SSC;

  platform.os = "ios";

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

  auto appData = parseINI(decodeURIComponent(_settings));

  StringStream env;

  for (auto const &envKey : parseStringList(appData["build_env"])) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << String(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  env << String("width=" + std::to_string(appFrame.size.width) + "&");
  env << String("height=" + std::to_string(appFrame.size.height) + "&");

  NSFileManager *fileManager = [NSFileManager defaultManager];
  NSString *currentDirectoryPath = [fileManager currentDirectoryPath];
  NSString *cwd = [NSHomeDirectory() stringByAppendingPathComponent: currentDirectoryPath];

  WindowOptions opts {
    .debug = _debug,
    .env = env.str(),
    .cwd = String([cwd UTF8String])
  };

  // Note: you won't see any logs in the preload script before the
  // Web Inspector is opened
  String  preload = ToString(createPreload(opts));

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

  [self.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
  [self.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

  NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
  [ns addObserver: self selector: @selector(keyboardDidShow) name: UIKeyboardDidShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardDidHide) name: UIKeyboardDidHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillShow) name: UIKeyboardWillShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillHide) name: UIKeyboardWillHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillChange:) name: UIKeyboardWillChangeFrameNotification object: nil];

  self.navDelegate = [[SSCNavigationDelegate alloc] init];
  [self.webview setNavigationDelegate: self.navDelegate];

  [viewController.view addSubview: self.webview];

  NSString* allowed = [[NSBundle mainBundle] resourcePath];
  NSURL* url;
  int port = getDevPort();

  if (isDebugEnabled() && port > 0 && getDevHost() != nullptr) {
    NSString* host = [NSString stringWithUTF8String:getDevHost()];
    url = [NSURL
      URLWithString: [NSString stringWithFormat: @"http://%@:%d/", host, port]
    ];

    [self.webview loadFileRequest: [NSURLRequest requestWithURL: url]
          allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
    ];
  } else {
    url = [NSURL
      fileURLWithPath: [allowed stringByAppendingPathComponent:@"ui/index.html"]
    ];

    auto preload = createPreload(opts, PreloadOptions { .module = true });
    auto script = "<script type=\"module\">" + preload + "</script>";
    auto html = String([[NSString stringWithContentsOfURL: url encoding: NSUTF8StringEncoding error: nil] UTF8String]);

    if (html.find("<head>") != -1) {
      html = replace(html, "<head>", "<head>" + script);
    } else if (html.find("<body>") != -1) {
      html = replace(html, "<body>", "<body>" + script);
    } else if (html.find("<html>") != -1) {
      html = replace(html, "<html>", "<html>" + script);
    } else {
      html = script + html;
    }

    [self.webview
      loadHTMLString: [NSString stringWithUTF8String: html.c_str()]
             baseURL: [url URLByDeletingLastPathComponent]
    ];
  }

  self.webview.scrollView.delegate = self;
  [self.window makeKeyAndVisible];

  return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
