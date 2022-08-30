#import <UIKit/UIKit.h>
#import <Webkit/Webkit.h>
#import "common.hh"

#include <_types/_uint64_t.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <map>

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = false;

@interface BridgedWebView : WKWebView
@end

#include "./apple.mm" // creates instance of bridge

Bridge* bridge;
BluetoothDelegate* bt;
SSC::Core* core;

@implementation BridgedWebView
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler, UIScrollViewDelegate>
@property (strong, nonatomic) UIWindow* window;
@property (strong, nonatomic) NavigationDelegate* navDelegate;
@property (strong, nonatomic) BridgedWebView* webview;
@property (strong, nonatomic) WKUserContentController* content;
@end

void uncaughtExceptionHandler (NSException *exception) {
  NSLog(@"%@", exception.name);
  NSLog(@"%@", exception.reason);
  NSLog(@"%@", exception.callStackSymbols);
}

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) applicationDidEnterBackground: (UIApplication*)application {
  [self.webview evaluateJavaScript: @"window.blur()" completionHandler:nil];
}

- (void) applicationWillEnterForeground: (UIApplication*)application {
  [self.webview evaluateJavaScript: @"window.focus()" completionHandler:nil];
  [[bridge bluetooth] startScanning];
}

- (void) applicationDidBecomeActive: (UIApplication*)application {
  dispatch_async(queue, ^{
    SSC::Peer::resumeAllBound();
    SSC::runDefaultLoop();
  });
}

- (void) applicationWillResignActive: (UIApplication*)application {
  dispatch_async(queue, ^{
    SSC::Peer::pauseAllBound();
    SSC::stopDefaultLoop();
  });
}

//
// When a message is received try to route it.
// Messages may also be received and routed via the custom scheme handler.
//
- (void) userContentController: (WKUserContentController*) userContentController didReceiveScriptMessage: (WKScriptMessage*)scriptMessage {
  id body = [scriptMessage body];

  if (![body isKindOfClass:[NSString class]]) {
    return;
  }

  [bridge route: [body UTF8String] buf: NULL bufsize: 0];
}

- (void) keyboardWillHide {
  self.webview.scrollView.scrollEnabled = YES;
  [bridge emit: "keyboard" msg: SSC::format(R"JSON({
    "value": { "data": { "event": "will-hide" } }
  })JSON")];
}

- (void) keyboardDidHide {
  [bridge emit: "keyboard" msg: SSC::format(R"JSON({
    "value": { "data": { "event": "did-hide" } }
  })JSON")];
}

- (void) keyboardWillShow {
  self.webview.scrollView.scrollEnabled = NO;
  [bridge emit: "keyboard" msg: SSC::format(R"JSON({
    "value": { "data": { "event": "will-show" } }
  })JSON")];
}

- (void) keyboardDidShow {
  [bridge emit: "keyboard" msg: SSC::format(R"JSON({
    "value": { "data": { "event": "did-show" } }
  })JSON")];
}

- (void) keyboardWillChange: (NSNotification*)notification {
  NSDictionary* keyboardInfo = [notification userInfo];
  NSValue* keyboardFrameBegin = [keyboardInfo valueForKey: UIKeyboardFrameEndUserInfoKey];
  CGRect rect = [keyboardFrameBegin CGRectValue];
  CGFloat width = rect.size.width;
  CGFloat height = rect.size.height;

  [bridge emit: "keyboard" msg: SSC::format(R"JSON({
    "value": { "data": { "event": "will-change", "width": "$S", "height": "$S" } }
  })JSON", std::to_string((float)width), std::to_string((float)height))];
}

- (void) scrollViewDidScroll: (UIScrollView*)scrollView {
  scrollView.bounds = self.webview.bounds;
}

- (BOOL) application: (UIApplication*)app openURL: (NSURL*)url options: (NSDictionary<UIApplicationOpenURLOptionsKey, id>*)options {
  auto str = std::string(url.absoluteString.UTF8String);

  // TODO can this be escaped or is the url encoded property already?
  [bridge emit: "protocol" msg: SSC::format(R"JSON({
    "url": "$S",
  })JSON", str)];

  return YES;
}

- (BOOL) application: (UIApplication*)application didFinishLaunchingWithOptions: (NSDictionary*)launchOptions {
  using namespace SSC;

  NSSetUncaughtExceptionHandler(&uncaughtExceptionHandler);
  platform.os = "ios";

  core = new SSC::Core;
  bridge = [Bridge new];

  bt = [BluetoothDelegate new];
  [bt setBridge: bridge];

  auto appFrame = [[UIScreen mainScreen] bounds];

  self.window = [[UIWindow alloc] initWithFrame: appFrame];

  UIViewController *viewController = [[UIViewController alloc] init];
  viewController.view.frame = appFrame;
  self.window.rootViewController = viewController;

  auto appData = parseConfig(decodeURIComponent(_settings));

  std::stringstream env;

  for (auto const &envKey : split(appData["env"], ',')) {
    auto cleanKey = trim(envKey);
    auto envValue = getEnv(cleanKey.c_str());

    env << std::string(
      cleanKey + "=" + encodeURIComponent(envValue) + "&"
    );
  }

  env << std::string("width=" + std::to_string(appFrame.size.width) + "&");
  env << std::string("height=" + std::to_string(appFrame.size.height) + "&");

  NSFileManager *fileManager = [NSFileManager defaultManager];
  NSString *currentDirectoryPath = [fileManager currentDirectoryPath];
  NSString *cwd = [NSHomeDirectory() stringByAppendingPathComponent: currentDirectoryPath];

  WindowOptions opts {
    .debug = _debug,
    .executable = appData["executable"],
    .title = appData["title"],
    .version = appData["version"],
    .preload = gPreloadMobile,
    .env = env.str(),
    .cwd = std::string([cwd UTF8String])
  };

  // Note: you won't see any logs in the preload script before the
  // Web Inspector is opened
  std::string  preload = Str(
    "window.addEventListener('unhandledrejection', e => console.log(e.message));\n"
    "window.addEventListener('error', e => console.log(e.reason));\n"

    "window.external = {\n"
    "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
    "};\n"

    "console.error = console.warn = console.log;\n"
    "" + createPreload(opts) + "\n"
    "//# sourceURL=preload.js"
  );

  WKUserScript* initScript = [[WKUserScript alloc]
    initWithSource: [NSString stringWithUTF8String: preload.c_str()]
    injectionTime: WKUserScriptInjectionTimeAtDocumentStart
    forMainFrameOnly: NO];

  WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

  IPCSchemeHandler* handler = [IPCSchemeHandler new];
  [handler setBridge: bridge];
  [config setURLSchemeHandler: handler forURLScheme:@"ipc"];

  self.content = [config userContentController];

  [self.content addScriptMessageHandler:self name: @"webview"];
  [self.content addUserScript: initScript];

  self.webview = [[BridgedWebView alloc] initWithFrame: appFrame configuration: config];
  self.webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  [self.webview.configuration.preferences setValue: @YES forKey: @"allowFileAccessFromFileURLs"];
  [self.webview.configuration.preferences setValue: @YES forKey: @"javaScriptEnabled"];

  NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
  [ns addObserver: self selector: @selector(keyboardDidShow) name: UIKeyboardDidShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardDidHide) name: UIKeyboardDidHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillShow) name: UIKeyboardWillShowNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillHide) name: UIKeyboardWillHideNotification object: nil];
  [ns addObserver: self selector: @selector(keyboardWillChange:) name: UIKeyboardWillChangeFrameNotification object: nil];

  [bridge setBluetooth: [BluetoothDelegate new]];
  [bridge setWebview: self.webview];
  [bridge setCore: new SSC::Core];

  self.navDelegate = [[NavigationDelegate alloc] init];
  [self.webview setNavigationDelegate: self.navDelegate];

  [viewController.view addSubview: self.webview];

  NSString* allowed = [[NSBundle mainBundle] resourcePath];
  NSString* url = [allowed stringByAppendingPathComponent:@"ui/index.html"];

  [self.webview
		loadFileURL: [NSURL fileURLWithPath: url]
    allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
  ];

  self.webview.scrollView.delegate = self;

  [self.window makeKeyAndVisible];
  [bridge initNetworkStatusObserver];

  return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
