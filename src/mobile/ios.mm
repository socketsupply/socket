#include <objc/runtime.h>
#include <objc/message.h>
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
  "co.socketsupply.queue.app",
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

void SSCSwapInstanceMethodWithBlock(Class cls, SEL original, id replacementBlock, SEL replacementSelector)
{
  Method originalMethod = class_getInstanceMethod(cls, original);
  if (!originalMethod) {
    return;
  }

  IMP implementation = imp_implementationWithBlock(replacementBlock);
  class_addMethod(cls, replacementSelector, implementation, method_getTypeEncoding(originalMethod));
  Method newMethod = class_getInstanceMethod(cls, replacementSelector);
  method_exchangeImplementations(originalMethod, newMethod);
}

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

  NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
  [ns addObserver: self selector: @selector(keyboardDidShow) name: UIKeyboardDidShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardDidHide) name: UIKeyboardDidHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillShow) name: UIKeyboardWillShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillHide) name: UIKeyboardWillHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillChange:) name: UIKeyboardWillChangeFrameNotification object: nil];

  [self listenForKeyDown];
  [self setUpWebView];
  [self.window makeKeyAndVisible];

  return YES;
}

- (void)setUpWebView
{
  [self.content removeAllUserScripts];
  self.webview.navigationDelegate = nil;
  self.webview.scrollView.delegate = nil;
  [self.webview stopLoading];
  [self.webview removeFromSuperview];

  auto appData = parseConfig(decodeURIComponent(_settings));

  StringStream env;

  for (auto const &envKey : split(appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << String(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  CGRect appFrame = self.window.frame;
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

  self.content = [config userContentController];

  [self.content addScriptMessageHandler:self name: @"external"];
  [self.content addUserScript: initScript];

  self.webview = [[SSCBridgedWebView alloc] initWithFrame:appFrame configuration: config];
  self.webview.autoresizingMask = UIViewAutoresizingNone;
  self.webview.translatesAutoresizingMaskIntoConstraints = YES;
  self.webview.scrollView.delegate = self;

  [self.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
  [self.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

  self.navDelegate = [[SSCNavigationDelegate alloc] init];
  [self.webview setNavigationDelegate: self.navDelegate];

  UIViewController* viewController = self.window.rootViewController;
  [viewController.view addSubview: self.webview];

  NSString* allowed = [[NSBundle mainBundle] resourcePath];

  NSURL* url;
  if (isDebugEnabled()) {
    NSString* host = [NSString stringWithUTF8String:getDevHost()];
    int port = getDevPort();
    url = [NSURL
        URLWithString:[NSString stringWithFormat:@"http://%@:%d/", host, port]];
    [self.webview loadRequest:[NSURLRequest requestWithURL:url]];
  } else {
    url = [NSURL fileURLWithPath:[allowed stringByAppendingPathComponent:@"ui/index.html"]];
    [self.webview loadFileURL:url
        allowingReadAccessToURL:[NSURL fileURLWithPath:allowed]];
  }
}

- (void)listenForKeyDown
{
  SEL originalKeyEventSelector = NSSelectorFromString(@"handleKeyUIEvent:");
  SEL swizzledKeyEventSelector = NSSelectorFromString(
      [NSString stringWithFormat:@"_ssc_swizzle_%x_%@", arc4random(), NSStringFromSelector(originalKeyEventSelector)]);

  SSCSwapInstanceMethodWithBlock([UIApplication class], originalKeyEventSelector, ^BOOL (UIApplication* app, UIEvent* event) {
      NSString *modifiedInput = nil;
      UIKeyModifierFlags modifierFlags = 0;
      BOOL isKeyDown = NO;

      if ([event respondsToSelector:@selector(_modifiedInput)]) {
        modifiedInput = (NSString *)[event _modifiedInput];
      }

      if ([event respondsToSelector:@selector(_modifierFlags)]) {
        modifierFlags = (UIKeyModifierFlags)[event _modifierFlags];
      }

      if ([event respondsToSelector:@selector(_isKeyDown)]) {
        isKeyDown = (BOOL)[event _isKeyDown];
      }

      if (isKeyDown && modifiedInput.length > 0) {
        if (modifierFlags == UIKeyModifierCommand && [modifiedInput isEqualToString:@"r"]) {
          [self setUpWebView];
        } else {
          // Ignore key commands (except escape) when there's an active responder
          UIResponder *firstResponder = [self.window valueForKey:@"firstResponder"];
          if (!firstResponder) {
            // TODO: native key handlers
          }
        }
      }

      return ((BOOL (*)(id, SEL, id))objc_msgSend)(app, swizzledKeyEventSelector, event);
  }, swizzledKeyEventSelector);
}

@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
