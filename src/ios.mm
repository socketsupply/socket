#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
// #import "common.hh"

/* constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = DEBUG;

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
@end

@implementation NavigationDelegate
- (void) webView: (WKWebView*) webView
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  // std::string base = webView.URL.absoluteString.UTF8String;
  std::string request = navigationAction.request.URL.absoluteString.UTF8String;

  if (request.find("file://") == 0) {
    decisionHandler(WKNavigationActionPolicyAllow);
  } else {
    decisionHandler(WKNavigationActionPolicyCancel);
  }
}
@end

@interface AppDelegate : UIResponder <UIApplicationDelegate, WKScriptMessageHandler>
@property (strong, nonatomic) UIWindow *window;
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) userContentController: (WKUserContentController *) userContentController
  didReceiveScriptMessage: (WKScriptMessage *) scriptMessage {
    using namespace Opkit;

    id body = [scriptMessage body];
    if (![body isKindOfClass:[NSString class]]) {
      return;
    }

    std::string msg = [body UTF8String];

    if (msg.find("ipc://") == 0) {
      Parse cmd(msg);

      if (cmd.name == "open") {
        // NSString *url = [NSString stringWithUTF8String:cmd.value.c_str()];
        // [[UIApplication sharedApplication] openURL:[NSURL URLWithString:url]];
        return;
      }
    }

    NSLog(@"OPKIT: console '%s'", msg.c_str());
}

- (BOOL) application: (UIApplication *) application
  didFinishLaunchingWithOptions: (NSDictionary *) launchOptions {
    using namespace Opkit;

    auto appFrame = [[UIScreen mainScreen] bounds];

    self.window = [[UIWindow alloc]
      initWithFrame: appFrame];

    UIViewController *viewController = [[UIViewController alloc] init];
    self.window.rootViewController = viewController;

    auto width = [[UIScreen mainScreen] bounds].size.width;
    auto height = [[UIScreen mainScreen] bounds].size.height;
    // auto frame = CGRectMake(0, 0, width, height);

    UIViewController *viewController = [[UIViewController alloc] init];
    // viewController.view.frame = CGRectMake(0, 0, width, height);

    // viewController.view.backgroundColor = [UIColor whiteColor];
    // viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    // viewController.view.autoresizesSubviews = YES;
    // viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    // viewController.modalPresentationStyle = UIModalPresentationFullScreen;
    // [viewController setModalPresentationStyle: UIModalPresentationFullScreen];

    // self.window.layer.borderColor = [UIColor redColor];
    // self.window.layer.borderWidth = 2;

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

    env << std::string("width=" + std::to_string(width) + "&");
    env << std::string("height=" + std::to_string(height) + "&");

    WindowOptions opts {
      .debug = _debug,
      .executable = appData["executable"],
      .title = appData["title"],
      .version = appData["version"],
      .env = env.str()
    };

    //
    // Configure the webview.
    //
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

    //
    // Add the controller and set up a script message listener.
    //
    WKUserContentController* controller = [config userContentController];
    [controller addScriptMessageHandler:self name: @"webview"];

    std::string preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
      "};\n"
      "console.log = (...args) => {\n"
      "  window.external.invoke(JSON.stringify(args));\n"
      "};\n"

      "console.error = console.warn = console.log;\n"
      "window.addEventListener('unhandledrejection', e => console.log(e.message));\n"
      "window.addEventListener('error', e => console.log(e.reason));\n"
      "" + createPreload(opts) + "\n"
    );

    WKUserScript* userScript = [WKUserScript alloc];

    [userScript
      initWithSource: [NSString stringWithUTF8String: preload.c_str()]
      injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO];

    [controller addUserScript: userScript];

    WKWebView* webview = [[WKWebView alloc]
      initWithFrame: appFrame
      configuration: config];

    webview.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

    [webview.configuration.preferences
      setValue: @YES
      forKey: @"allowFileAccessFromFileURLs"];

    [viewController.view addSubview: webview];

    NavigationDelegate *navDelegate = [[NavigationDelegate alloc] init];
    [webview setNavigationDelegate:navDelegate];

    //
    // Get the path and load the file.
    //
    NSString *path = [[NSBundle mainBundle]
      pathForResource: @"index"
      ofType: @"html"];

    NSURL *url = [NSURL fileURLWithPath:path];
    [webview loadFileURL:url allowingReadAccessToURL:url];

    NSLog(@"OPKIT: fs '%@'", url);
    [self.window makeKeyAndVisible];
    return YES;
}

@end */

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

@implementation AppDelegate
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(id)options {
  CGRect mainScreenBounds = [[UIScreen mainScreen] bounds];
  self.window = [[UIWindow alloc] initWithFrame:mainScreenBounds];

  UIViewController *viewController = [[UIViewController alloc] init];
  viewController.view.backgroundColor = [UIColor whiteColor];
  viewController.view.frame = mainScreenBounds;
  UILabel *label = [[UILabel alloc] initWithFrame:mainScreenBounds];
  [label setText:@"Wow! I was built with clang and make!"];
  [viewController.view addSubview: label];
  self.window.rootViewController = viewController;
  [self.window makeKeyAndVisible];

  return YES;
}
@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
