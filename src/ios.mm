#import <Webkit/Webkit.h>
#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>
#import "common.hh"

@interface AppDelegate : UIResponder <UIApplicationDelegate>
@property (strong, nonatomic) UIWindow *window;
@end

//
// iOS has no "window". There is no navigation, just a single webview. It also
// has no "main" process, we want to expose some network functionality to the
// JavaScript environment so it can be used by the web app and the wasm layer.
//
@implementation AppDelegate
- (void) handleMessage : (NSString *) message {
  // TODO match the same api as the mac version
    NSLog(@"handleMessage: %@", message);
}

- (BOOL) application: (UIApplication *) application
  didFinishLaunchingWithOptions: (NSDictionary *) launchOptions {
    self.window = [[UIWindow alloc]
      initWithFrame: [[UIScreen mainScreen] bounds]];

    //
    // Configure the webview.
    //
    WKWebViewConfiguration *config = [[WKWebViewConfiguration alloc] init];

    WKWebView* webview = [[WKWebView alloc]
      initWithFrame: self.window.frame
      configuration: config];

    /* webview.navigationDelegate = self;

    [webview.configuration.preferences
      setValue:@YES
      forKey:@"allowFileAccessFromFileURLs"];

    //
    // Add the controller and set up a script message listener.
    //
    WKUserContentController* controller = [config userContentController];

    NSString *preload = @"
      window.external = {\n
        invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n
      };\n
    "];
    // " + createPreload(opts) + "\n"

    WKUserScript* userScript = [WKUserScript alloc];

    [userScript
      initWithSource: [NSString stringWithUTF8String: preload.c_str()]
      injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO];

    [controller addUserScript: userScript];

    //
    // Set delegate to window
    //
    WindowDelegate* delegate = [WindowDelegate alloc];

    class_replaceMethod(
      [WindowDelegate class],
      @selector(userContentController:didReceiveScriptMessage:),
      imp_implementationWithBlock(
        [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
          [self handleMessage: scriptMessage];
        }
      )
    );

    [controller
      addScriptMessageHandler: delegate
      name: @"webview"];

    [window setDelegate: delegate];

    //
    // Get the path and load the file.
    //
    NSString *path = [[NSBundle mainBundle]
      pathForResource: @"index"
      ofType: @"html"];

    NSURL *url = [NSURL fileURLWithPath:path];
    [webview loadFileURL:url allowingReadAccessToURL:url]; */

    [self.window makeKeyAndVisible];
    return YES;
}

@end

int main (int argc, char *argv[]) {
  @autoreleasepool {
    return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
  }
}
