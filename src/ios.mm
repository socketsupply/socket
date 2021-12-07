#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "common.hh"

constexpr auto _settings = STR_VALUE(SETTINGS);
constexpr auto _debug = DEBUG;

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
  didReceiveScriptMessage: (WKScriptMessage *) message {
  // TODO match the same api as the mac version
    NSLog(@"handleMessage: %@", message);
}

- (void) viewWillLayoutSubviews {
  self.window.frame = [UIScreen mainScreen].bounds;
}

- (BOOL) application: (UIApplication *) application
  didFinishLaunchingWithOptions: (NSDictionary *) launchOptions {
    auto appFrame = [[UIScreen mainScreen] bounds];

    self.window = [[UIWindow alloc]
      initWithFrame: appFrame];

    // auto width = [[UIScreen mainScreen] bounds].size.width;
    // auto height = [[UIScreen mainScreen] bounds].size.height * 1.5;
    // auto frame = CGRectMake(0, 0, width, height);

    UIViewController *viewController = [[UIViewController alloc] init];
    // viewController.view.frame = CGRectMake(0, 0, width, height);

    // viewController.view.backgroundColor = [UIColor whiteColor];
    // viewController.view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    // viewController.modalPresentationStyle = UIModalPresentationFullScreen;
    // [viewController setModalPresentationStyle: UIModalPresentationFullScreen];

    // self.window.layer.borderColor = [UIColor redColor];
    // self.window.layer.borderWidth = 2;

    self.window.rootViewController = viewController;

    using namespace Opkit;
    auto appData = parseConfig(decodeURIComponent(_settings));

    std::stringstream env;

    for (auto const &envKey : split(appData["env"], ',')) {
      auto cleanKey = trim(envKey);
      auto envValue = getEnv(cleanKey.c_str());

      env << std::string(
        cleanKey + "=" + encodeURIComponent(envValue) + "&"
      );
    }

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

    [webview.configuration.preferences
      setValue: @YES
      forKey: @"allowFileAccessFromFileURLs"];

    [viewController.view addSubview: webview];

    //
    // Get the path and load the file.
    //
    NSString *path = [[NSBundle mainBundle]
      pathForResource: @"index"
      ofType: @"html"];

    NSURL *url = [NSURL fileURLWithPath:path];
    [webview loadFileURL:url allowingReadAccessToURL:url];

    [self.window makeKeyAndVisible];
    return YES;
}

@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
