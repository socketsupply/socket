#include "common.hh"
#import <Cocoa/Cocoa.h>
#import <Webkit/Webkit.h>
#include <objc/objc-runtime.h>

// ObjC declarations may only appear in global scope
@interface WindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
@end

@implementation WindowDelegate
- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)scriptMessage {
}
@end

namespace Opkit {
  class App;
  class Window;

  class App {
    NSAutoreleasePool* pool = [NSAutoreleasePool new];

    bool shouldExit = false;

    public:
      int run();
      void exit();
      void dispatch(std::function<void()> work);
  };

  class Window {
    NSWindow* window;
    WKWebView* webview;
    bool initDone = false;
    App app;
    std::function<void(String)> _onMessage = nullptr;

    public:
      Window(App&, WindowOptions);
      void onMessage(std::function<void(String)>);
      void eval(const String&);
      void show();
      void hide();
      String url;
      String title;
      void navigate(const String&);
      void setTitle(const String&);
      void openDialog();
      void createContextMenu();
      void createSystemMenu();
  };

  int App::run () {
    NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:true];
    if (event) {
        [NSApp sendEvent:event];
    }

    // [NSApp run];
    return shouldExit;
  }

  // We need to dispatch any code that touches
  // a window back into the main thread.
  void App::dispatch(std::function<void()> f) {
    auto priority = DISPATCH_QUEUE_PRIORITY_DEFAULT;
    auto queue = dispatch_get_global_queue(priority, 0);

    dispatch_async(queue, ^{
      dispatch_sync(dispatch_get_main_queue(), ^{
        f();
      });
    });
  }

  void App::exit () {
    // Distinguish window closing with app exiting
    shouldExit = true;
    [NSApp terminate:nil];
  }

  Window::Window (App& app, WindowOptions opts) {
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled;

    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.frameless) {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
    } else {
      style |= NSWindowStyleMaskClosable;
      style |= NSWindowStyleMaskMiniaturizable;
    }

    // Initialize Cocoa window
    window = [[NSWindow alloc]
        // Initial window size
        initWithContentRect:NSMakeRect(0, 0, opts.width, opts.height)
                  // Window style
                  styleMask:style
                    backing:NSBackingStoreBuffered
                      defer:NO];

    // Minimum window size
    [window setContentMinSize:NSMakeSize(opts.width, opts.height)];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent:true];
    }

    // Position window in center of screen
    [window center];

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    WKPreferences* prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];
    [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKUserContentController* controller = [config userContentController];

    // Add preload script
    String preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg);\n"
      "};\n"
      "" + opts.preload + "\n"
      "\n"
      "//#preload.js"
    );

    WKUserScript* userScript = [WKUserScript alloc];
    [userScript initWithSource:[NSString stringWithUTF8String:preload.c_str()]
                 injectionTime:WKUserScriptInjectionTimeAtDocumentStart
              forMainFrameOnly:NO];
    [controller addUserScript:userScript];

    webview = [[WKWebView alloc] initWithFrame:NSZeroRect configuration:config];


    [webview.configuration.preferences
      setValue:@YES
        forKey:@"allowFileAccessFromFileURLs"];

    // Add delegate methods manually in order to capture "this"
    class_replaceMethod(
      [WindowDelegate class], @selector(windowWillClose:),
      imp_implementationWithBlock(
        [&](id self, SEL cmd, id notification) {
          app.exit();
        }),
      "v@:@"
    );

    class_replaceMethod(
      [WindowDelegate class],
      @selector(userContentController:didReceiveScriptMessage:),
      imp_implementationWithBlock(
        [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
          if (this->_onMessage == nullptr) return;

          id body = [scriptMessage body];
          if (![body isKindOfClass:[NSString class]]) {
            return;
          }

          std::string msg = [body UTF8String];
          this->_onMessage(msg);
        }),
      "v@:@"
    );

    WindowDelegate* delegate = [WindowDelegate alloc];
    [controller addScriptMessageHandler:delegate name:@"webview"];
    // Set delegate to window
    [window setDelegate:delegate];

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Sets the app as the active app
    [NSApp activateIgnoringOtherApps:YES];

    // Add webview to window
    [window setContentView:webview];

    // [webview.configuration.preferences setValue:@YES forKey:@"allowFileAccessFromFileURLs"];
    // [webview.configuration setValue:@YES forKey:@"allowUniversalAccessFromFileURLs"];


    // Done initialization, set properties
    initDone = true;

    navigate(opts.url);
  }

  void Window::onMessage(std::function<void(String)> cb) {
    _onMessage = cb;
  }

  void Window::show () {
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
  }

  void Window::hide () {
    [window orderOut:window];
  }

  void Window::eval(const String& js) {
    [webview evaluateJavaScript:
      [NSString stringWithUTF8String:js.c_str()]
      completionHandler:nil];
  }

  void Window::navigate (const String& s) {
    [webview loadRequest:
      [NSURLRequest requestWithURL:
        [NSURL URLWithString:
          [NSString stringWithUTF8String: s.c_str()]]]];
  }

  void Window::setTitle(const String& s) {
    [window setTitle:[NSString stringWithUTF8String:s.c_str()]];
  }

  void Window::openDialog () {}
  void Window::createContextMenu () {}
  void Window::createSystemMenu () {}
}
