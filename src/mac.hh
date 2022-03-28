#include "common.hh"
#import <Cocoa/Cocoa.h>
#import <Webkit/Webkit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <objc/objc-runtime.h>

@interface WV : WKWebView<
  WKUIDelegate,
  NSDraggingDestination,
  NSFilePromiseProviderDelegate,
  NSDraggingSource>
- (NSDragOperation) draggingSession: (NSDraggingSession *)session
  sourceOperationMaskForDraggingContext:(NSDraggingContext)context;
@end

@implementation WV
std::vector<std::string> draggablePayload;

- (BOOL) prepareForDragOperation: (id<NSDraggingInfo>)info {
  [info setDraggingFormation: NSDraggingFormationNone];
  return NO;
}

- (void) draggingExited: (id<NSDraggingInfo>)info {
  auto payload = Operator::emitToRenderProcess("dragend", "{}");
  draggablePayload.clear();

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
}

- (NSDragOperation) draggingUpdated:(id <NSDraggingInfo>)info {
  NSPoint pos = [info draggingLocation];
  auto x = std::to_string(pos.x);
  auto y = std::to_string([self frame].size.height - pos.y);

  int count = draggablePayload.size();
  bool inbound = false;

  if (count == 0) {
    inbound = true;
    count = [info numberOfValidItemsForDrop];
  }

  std::string json = (
    "{\"count\":" + std::to_string(count) + ","
    "\"inbound\":" + (inbound ? "true" : "false") + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = Operator::emitToRenderProcess("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
  return NSDragOperationGeneric;
}

- (void) draggingEntered: (id<NSDraggingInfo>)info {
  [NSApp activateIgnoringOtherApps:YES];
  [self draggingUpdated: info];
}

- (void) draggingEnded: (id<NSDraggingInfo>)info {
  NSPasteboard *pboard = [info draggingPasteboard];

  if (NSPointInRect([info draggingLocation], self.frame)) {
    NSArray<Class> *classes = @[[NSURL class]];
    NSDictionary *options = @{};
    NSArray<NSURL*> *files = [pboard readObjectsForClasses:classes options:options];

    NSPoint pos = [info draggingLocation];
    // NSWindow is (0,0) at bottom left, browser is (0,0) at top left
    // so we need to flip the y coordinate to convert to browser coordinates
    int y = [self frame].size.height - pos.y;

    for (NSURL *url in files) {
      std::string path([[url path] UTF8String]);
      path = Operator::replace(path, "\"", "'");

      std::string json = (
        "{\"src\":\"" + path + "\","
        "\"x\":" + std::to_string(pos.x) + ","
        "\"y\":" + std::to_string(y) + "}"
      );

      auto payload = Operator::emitToRenderProcess("dropin", json);

      [self evaluateJavaScript:
        [NSString stringWithUTF8String: payload.c_str()]
        completionHandler:nil];
    }
  }

  auto payload = Operator::emitToRenderProcess("dragend", "{}");

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];

  // [super draggingEnded:info];
}

- (void) updateEvent: (NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);
  auto count = std::to_string(draggablePayload.size());

  std::string json = (
    "{\"count\":" + count + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = Operator::emitToRenderProcess("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
}

- (void) mouseUp: (NSEvent*)event {
  [super mouseUp:event];
  auto payload = Operator::emitToRenderProcess("dragend", "{}");
  draggablePayload.clear();

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
}

- (void) mouseDown: (NSEvent*)event {
  draggablePayload.clear();

  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);

  std::string js(
    "(() => {"
    "  const el = document.elementFromPoint(" + x + "," + y + ");"
    "  return el && el.dataset?.src"
    "})()");

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: ^(id result, NSError *error) {
    if (error) {
      NSLog(@"%@", error);
      [super mouseDown:event];
      return;
    }

    if (![result isKindOfClass:[NSString class]]) {
      [super mouseDown:event];
      return;
    }

    std::vector<std::string> files =
      Operator::split(std::string([result UTF8String]), ';');

    if (files.size() == 0) {
      [super mouseDown:event];
      return;
    }

    draggablePayload = files;
    [self updateEvent: event];
  }];
}

- (void) mouseDragged: (NSEvent*)event {
  [super mouseDragged:event];
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];

  if (draggablePayload.size() == 0) return;

  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);
  auto count = std::to_string(draggablePayload.size());

  std::string json = (
    "{\"count\":" + count + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = Operator::emitToRenderProcess("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];

  if (NSPointInRect(location, self.frame)) return;

  NSPasteboard *pboard = [NSPasteboard pasteboardWithName: NSPasteboardNameDrag];
  [pboard declareTypes: @[(NSString*)kPasteboardTypeFileURLPromise] owner:self];

  NSMutableArray* dragItems = [[NSMutableArray alloc] init];
  NSSize iconSize = NSMakeSize(32, 32); // according to documentation
  NSRect imageLocation;
  NSPoint dragPosition = [self
    convertPoint: [event locationInWindow]
    fromView: nil];

  dragPosition.x -= 16;
  dragPosition.y -= 16;
  imageLocation.origin = dragPosition;
  imageLocation.size = iconSize;

  for (auto& file : draggablePayload) {
    NSString* nsFile = [[NSString alloc] initWithUTF8String:file.c_str()];
    NSURL* fileURL = [NSURL fileURLWithPath: nsFile];

    NSImage* icon = [[NSWorkspace sharedWorkspace] iconForContentType: UTTypeURL];

    NSArray* (^providerBlock)() = ^NSArray*() {
      NSDraggingImageComponent* comp = [[[NSDraggingImageComponent alloc]
        initWithKey: NSDraggingImageComponentIconKey] retain];

      comp.frame = NSMakeRect(0, 0, iconSize.width, iconSize.height);
      comp.contents = icon;
      return @[comp];
    };

    NSDraggingItem* dragItem;
    auto provider = [[NSFilePromiseProvider alloc] initWithFileType:@"public.url" delegate: self];
    [provider setUserInfo: [NSString stringWithUTF8String:file.c_str()]];
    dragItem = [[NSDraggingItem alloc] initWithPasteboardWriter: provider];

    dragItem.draggingFrame = NSMakeRect(
      dragPosition.x,
      dragPosition.y,
      iconSize.width,
      iconSize.height
    );

    dragItem.imageComponentsProvider = providerBlock;
    [dragItems addObject: dragItem];
  }

  NSDraggingSession* session = [self
      beginDraggingSessionWithItems: dragItems
                              event: event
                             source: self];

  session.draggingFormation = NSDraggingFormationPile;
  draggablePayload.clear();
}

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
  return NSDragOperationCopy;
}

- (void) filePromiseProvider:(NSFilePromiseProvider*)filePromiseProvider writePromiseToURL:(NSURL *)url
  completionHandler:(void (^)(NSError *errorOrNil))completionHandler
{
  std::string dest = [[url path] UTF8String];
  std::string src([[filePromiseProvider userInfo] UTF8String]);

  NSData *data = [@"" dataUsingEncoding:NSUTF8StringEncoding];
  [data writeToURL:url atomically:YES];

  std::string json = (
    "{\"src\":\"" + src + "\","
    "\"dest\":\"" + dest + "\"}"
  );

  std::string js = Operator::emitToRenderProcess("dropout", json);

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: nil
  ];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*)filePromiseProvider fileNameForType:(NSString *)fileType {
  std::string file(std::to_string(Operator::rand64()) + ".download");
  return [NSString stringWithUTF8String:file.c_str()];
}

@end

@interface WindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
- (void)userContentController: (WKUserContentController*) userContentController
      didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end

@implementation WindowDelegate
- (void)userContentController: (WKUserContentController*) userContentController
      didReceiveScriptMessage: (WKScriptMessage*) scriptMessage {
        // To be overridden
}
@end

@interface NavigationDelegate : NSObject<WKNavigationDelegate>
@end

@implementation NavigationDelegate
- (void) webView: (WV*) webView
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

/* @interface WebInspector : NSObject {
  WKWebView *webView;
}

- (id) initWithWebView: (WKWebView*) webView;
- (void) detach: (id) sender;
- (void) show: (id) sender;
@end

typedef const struct OpaqueWKPage* WKPageRef;
typedef const struct OpaqueWKInspector* WKInspectorRef;

WKInspectorRef WKPageGetInspector(WKPageRef pageRef);
void WKInspectorShow(WKInspectorRef inspector);

@interface WKWebView (Extras)
@property(readonly, nonatomic) WKPageRef _pageRefForTransitionToWKWebView;
@end

void WKInspectorShow(WKInspectorRef inspectorRef) {
  inspectorRef->show();
} */

namespace Operator {

  static bool isDelegateSet = false;

  class App : public IApp {
    NSAutoreleasePool* pool = [NSAutoreleasePool new];

    public:
      App(int);
      int run();
      void kill();
      void restart();
      void dispatch(std::function<void()> work);
      std::string getCwd(const std::string&);
  };

  class Window : public IWindow {
    NSWindow* window;
    WV* webview;
    // WebInspector* inspector;

    public:
      App app;
      WindowOptions opts;
      Window(App&, WindowOptions);
      bool webviewFailed = false;

      void eval(const std::string&);
      void show(const std::string&);
      void hide(const std::string&);
      void kill();
      void exit(int code);
      void close(int code);
      void navigate(const std::string&, const std::string&);
      void setTitle(const std::string&, const std::string&);
      void setSize(const std::string&, int, int, int);
      void setContextMenu(const std::string&, const std::string&);
      void closeContextMenu(const std::string&);
      void closeContextMenu();
      void openDialog(const std::string&, bool, bool, bool, bool, const std::string&, const std::string&, const std::string&);
      void showInspector();

      void setSystemMenu(const std::string& seq, const std::string& menu);
      int openExternal(const std::string& s);
      ScreenSize getScreenSize();
  };

  App::App (int instanceId) {
    // TODO enforce single instance is set
  }

  int App::run () {
    /* NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                        untilDate:[NSDate distantFuture]
                                           inMode:NSDefaultRunLoopMode
                                          dequeue:true];
    if (event) {
        [NSApp sendEvent:event];
    } */

    [NSApp run];
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

  void App::kill () {
    // Distinguish window closing with app exiting
    shouldExit = true;
    [NSApp terminate:nil];
  }

  void App::restart () {
  }

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled;

    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.frameless) {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
    } else if (opts.utility) {
      style |= NSWindowStyleMaskUtilityWindow;
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

    NSArray* draggableTypes = [NSArray arrayWithObjects:
                  NSPasteboardTypeURL,
                  NSPasteboardTypeFileURL,
                  (NSString *)kPasteboardTypeFileURLPromise,
                  NSPasteboardTypeString,
                  NSPasteboardTypeHTML,
                  nil];

    [window registerForDraggedTypes:draggableTypes];

    // Minimum window size
    [window setContentMinSize:NSMakeSize(opts.width, opts.height)];

    // [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];

    // [window setOpaque:NO];
    [window setBackgroundColor:[NSColor textBackgroundColor]];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent:true];
    }

    // Position window in center of screen
    [window center];

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    // config.limitsNavigationsToAppBoundDomains = YES;

    WKPreferences* prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];

    #if DEBUG == 1
      [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    #endif

    WKUserContentController* controller = [config userContentController];

    // Add preload script, normalizing the interface to be cross-platform.
    std::string preload = Str(
      "window.external = {\n"
      "  invoke: arg => window.webkit.messageHandlers.webview.postMessage(arg)\n"
      "};\n"
      "" + createPreload(opts) + "\n"
    );

    WKUserScript* userScript = [WKUserScript alloc];

    [userScript
      initWithSource:[NSString stringWithUTF8String:preload.c_str()]
      injectionTime:WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly:NO];

    [controller
      addUserScript: userScript];

    webview = [[WV alloc]
      initWithFrame: NSZeroRect
      configuration: config];

    [webview.configuration.preferences
      setValue:@YES
      forKey:@"allowFileAccessFromFileURLs"];

    window.titlebarAppearsTransparent = true;

    // [webview registerForDraggedTypes:
    //  [NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    //
    bool exiting = false;

    WindowDelegate* delegate = [WindowDelegate alloc];
    [controller addScriptMessageHandler:delegate name:@"webview"];

    // Set delegate to window
    [window setDelegate:delegate];

    NavigationDelegate *navDelegate = [[NavigationDelegate alloc] init];
    [webview setNavigationDelegate:navDelegate];

    if (!isDelegateSet) {
      isDelegateSet = true;

      class_replaceMethod(
        [WindowDelegate class],
        @selector(windowShouldClose:),
        imp_implementationWithBlock(
          [&](id self, SEL cmd, id notification) {
            if (exiting) return true;

            auto* w = (Window*) objc_getAssociatedObject(self, "webview");

            if (w->opts.canExit) {
              exiting = true;
              w->exit(0);
              return true;
            }

            w->eval(emitToRenderProcess("windowHide", "{}"));
            w->hide("");
            return false;
          }),
        "v@:@"
      );

      class_replaceMethod(
        [WindowDelegate class],
        @selector(userContentController:didReceiveScriptMessage:),
        imp_implementationWithBlock(
          [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
            auto* w = (Window*) objc_getAssociatedObject(self, "webview");
            if (w->onMessage == nullptr) return;

            id body = [scriptMessage body];
            if (![body isKindOfClass:[NSString class]]) {
              return;
            }
            String msg = [body UTF8String];

            w->onMessage(msg);
          }),
        "v@:@"
      );

      class_addMethod(
        [WindowDelegate class],
        @selector(menuItemSelected:),
        imp_implementationWithBlock(
          [=](id self, SEL _cmd, id item) {
            auto* w = (Window*) objc_getAssociatedObject(self, "webview");
            if (w->onMessage == nullptr) return;

            id menuItem = (id) item;
            String title = [[menuItem title] UTF8String];
            String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
            String parent = [[[menuItem menu] title] UTF8String];
            String seq = std::to_string([menuItem tag]);

            w->eval(resolveMenuSelection(seq, title, parent));
          }),
        "v@:@:@:"
      );
    }

    objc_setAssociatedObject(
      delegate,
      "webview",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    // Sets the app as the active app
    [NSApp activateIgnoringOtherApps:YES];

    // Add webview to window
    [window setContentView:webview];

    navigate("0", opts.url);
  }

  ScreenSize Window::getScreenSize () {
    NSRect e = [[NSScreen mainScreen] frame];

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void Window::show (const std::string& seq) {
    [window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::exit (int code) {
    if (onExit != nullptr) onExit(code);
  }

  void Window::kill () {
  }

  void Window::close (int code) {
    [window performClose:nil];
  }

  void Window::hide (const std::string& seq) {
    [window orderOut:window];
    this->eval(emitToRenderProcess("windowHide", "{}"));

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::eval(const std::string& js) {
    [webview evaluateJavaScript:
      [NSString stringWithUTF8String:js.c_str()]
      completionHandler:nil];
  }

  void Window::navigate (const std::string& seq, const std::string& value) {
    [webview loadRequest:
      [NSURLRequest requestWithURL:
        [NSURL URLWithString:
          [NSString stringWithUTF8String: value.c_str()]]]];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::setTitle(const std::string& seq, const std::string& value) {
    [window setTitle:[NSString stringWithUTF8String:value.c_str()]];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::setSize(const std::string& seq, int width, int height, int hints) {
    [window setFrame:NSMakeRect(0.f, 0.f, (float) width, (float) height) display:YES animate:YES];
    [window center];

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  int Window::openExternal (const std::string& s) {
    NSString* nsu = [NSString stringWithUTF8String:s.c_str()];
    return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: nsu]];
  }

  std::string App::getCwd (const std::string& s) {
    NSString *bundlePath = [[NSBundle mainBundle] resourcePath];
    return String([bundlePath UTF8String]);
  }

  void Window::closeContextMenu() {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu(const std::string &seq) {
    // @TODO(jwerle)
  }

  void Window::showInspector () {
    [[this->webview _inspector] show];
  }

  void Window::setContextMenu (const std::string& seq, const std::string& value) {
    auto menuItems = split(value, '_');
    auto id = std::stoi(seq);

    NSPoint mouseLocation = [NSEvent mouseLocation];
    NSMenu *pMenu = [[[NSMenu alloc] initWithTitle:@"contextMenu"] autorelease];
    NSMenuItem *menuItem;
    int index = 0;

    for (auto item : menuItems) {
      if (trim(item).size() == 0) continue;

      if (item.find("---") != -1) {
        NSMenuItem *sep = [NSMenuItem separatorItem];
        [pMenu addItem:sep];
        index++;
        continue;
      }

      auto pair = split(trim(item), ':');

      NSString* nssTitle = [NSString stringWithUTF8String:pair[0].c_str()];
      NSString* nssKey = @"";

      if (pair.size() > 1) {
        nssKey = [NSString stringWithUTF8String:pair[1].c_str()];
      }

      menuItem = [pMenu
        insertItemWithTitle:nssTitle
        action:@selector(menuItemSelected:)
        keyEquivalent:nssKey
        atIndex:index
      ];

      [menuItem setTag:id];

      index++;
    }

    [pMenu
      popUpMenuPositioningItem:pMenu.itemArray[0]
        atLocation:NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y))
        inView:nil];
  }

  void Window::setSystemMenu (const std::string& seq, const std::string& value) {
    std::string menu = std::string(value);

    NSMenu *mainMenu;
    NSString *title;
    NSMenu *appleMenu;
    NSMenu *serviceMenu;
    NSMenu *windowMenu;
    NSMenu *editMenu;
    NSMenu *dynamicMenu;
    NSMenuItem *menuItem;

    if (NSApp == nil) {
      return;
    }

    mainMenu = [[NSMenu alloc] init];

    // Create the main menu bar
    [NSApp setMainMenu:mainMenu];
    [mainMenu release];
    mainMenu = nil;

    // Create the application menu

    // id appName = [[NSProcessInfo processInfo] processName];
    // title = [@"About " stringByAppendingString:appName];

    // deserialize the menu
    menu = replace(menu, "%%", "\n");

    // split on ;
    auto menus = split(menu, ';');

    for (auto m : menus) {
      auto menu = split(m, '\n');
      auto line = trim(menu[0]);
      if (line.empty()) continue;
      auto menuTitle = split(line, ':')[0];
      NSString* nssTitle = [NSString stringWithUTF8String:menuTitle.c_str()];
      dynamicMenu = [[NSMenu alloc] initWithTitle:nssTitle];

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.empty()) continue;
        auto parts = split(line, ':');
        auto title = parts[0];
        NSUInteger mask = 0;
        std::string key = "";

        if (parts.size() == 2) {
          if (parts[1].find("+") != -1) {
            auto accelerator = split(parts[1], '+');
            key = trim(accelerator[0]);

            if (accelerator[1].find("CommandOrControl") != -1) {
              mask |= NSEventModifierFlagCommand;
            } else if (accelerator[1].find("Meta") != -1) {
              mask |= NSEventModifierFlagCommand;
            } else if (accelerator[1].find("Control") != -1) {
              mask |= NSEventModifierFlagControl;
            }

            if (accelerator[1].find("Alt") != -1) {
              mask |= NSEventModifierFlagOption;
            }
          } else {
            key = trim(parts[1]);
          }
        }

        NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
        NSString* nssKey = [NSString stringWithUTF8String:key.c_str()];
        NSString* nssSelector = [NSString stringWithUTF8String:"menuItemSelected:"];

        if (menuTitle.compare("Edit") == 0) {
          if (title.compare("Cut") == 0) nssSelector = [NSString stringWithUTF8String:"cut:"];
          if (title.compare("Copy") == 0) nssSelector = [NSString stringWithUTF8String:"copy:"];
          if (title.compare("Paste") == 0) nssSelector = [NSString stringWithUTF8String:"paste:"];
          if (title.compare("Delete") == 0) nssSelector = [NSString stringWithUTF8String:"delete:"];
          if (title.compare("Select All") == 0) nssSelector = [NSString stringWithUTF8String:"selectAll:"];
        }

        if (title.find("About") == 0) {
          nssSelector = [NSString stringWithUTF8String:"orderFrontStandardAboutPanel:"];
        }

        if (title.find("Hide") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hide:"];
        }

        if (title.find("Hide Others") == 0) {
          nssSelector = [NSString stringWithUTF8String:"hideOtherApplications:"];
        }

        if (title.find("Show All") == 0) {
          nssSelector = [NSString stringWithUTF8String:"unhideAllApplications:"];
        }

        if (title.find("Quit") == 0) {
          // nssSelector = [NSString stringWithUTF8String:"terminate:"];
        }

        if (title.compare("Minimize") == 0) {
          nssSelector = [NSString stringWithUTF8String:"performMiniaturize:"];
        }

        // if (title.compare("Zoom") == 0) nssSelector = [NSString stringWithUTF8String:"performZoom:"];

        if (title.find("---") != -1) {
          NSMenuItem *sep = [NSMenuItem separatorItem];
          [dynamicMenu addItem:sep];
        } else {
          menuItem = [dynamicMenu
            addItemWithTitle: nssTitle
            action: NSSelectorFromString(nssSelector)
            keyEquivalent: nssKey
          ];
        }

        if (mask != 0) {
          [menuItem setKeyEquivalentModifierMask: mask];
        }

        [menuItem setTag:0]; // only contextMenu uses the tag
      }

      menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent:@""];

      [[NSApp mainMenu] addItem:menuItem];
      [menuItem setSubmenu:dynamicMenu];
      [menuItem release];
      [dynamicMenu release];
    }

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->onMessage(resolveToMainProcess(seq, "0", index));
    }
  }

  void Window::openDialog(
    const std::string& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const std::string& defaultPath = "",
    const std::string& title = "",
    const std::string& defaultName = "")
  {

    NSURL *url;
    const char *utf8_path;
    NSSavePanel *dialog_save;
    NSOpenPanel *dialog_open;
    NSURL *default_url = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (isSave) {
      dialog_save = [NSSavePanel savePanel];
    } else {
      dialog_open = [NSOpenPanel openPanel];
    }

    if (!isSave) {
      [dialog_open setCanCreateDirectories:YES];
    }

    if (allowDirs == true && isSave == false) {
      [dialog_open setCanChooseDirectories:YES];
    }

    if (isSave == false) {
      [dialog_open setCanChooseFiles: allowFiles ? YES : NO];
    }

    if ((isSave == false || allowDirs == true) && allowMultiple == true) {
      [dialog_open setAllowsMultipleSelection:YES];
    }

    if (defaultName.size() > 0) {
      default_url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String: defaultName.c_str()]];
    } else if (defaultPath.size() > 0) {
      default_url = [NSURL fileURLWithPath:
        [NSString stringWithUTF8String: defaultPath.c_str()]];
    }

    if (default_url != nil) {
      if (isSave) {
        [dialog_save setDirectoryURL: default_url];
        [dialog_save setNameFieldStringValue: default_url.lastPathComponent];
      } else {
        [dialog_open setDirectoryURL: default_url];
        [dialog_open setNameFieldStringValue: default_url.lastPathComponent];
      }
    }

    if (title.size() > 0) {
      NSString* nssTitle = [NSString stringWithUTF8String:title.c_str()];
      if (isSave) {
        [dialog_save setTitle: nssTitle];
      } else {
        [dialog_open setTitle: nssTitle];
      }
    }

    if (isSave) {
      if ([dialog_save runModal] == NSModalResponseOK) {
        String url = (char*) [[[dialog_save URL] path] UTF8String];
        auto wrapped = std::string("\"" + url + "\"");
        this->eval(resolveToRenderProcess(seq, "0", encodeURIComponent(wrapped)));
      }
      return;
    }

    String result = "";
    std::vector<String> paths;
    NSArray* urls;

    if ([dialog_open runModal] == NSModalResponseOK) {
      urls = [dialog_open URLs];

      for (NSURL* url in urls) {
        if ([url isFileURL]) {
          paths.push_back(String((char*) [[url path] UTF8String]));
        }
      }
    }

    [pool release];

    for (size_t i = 0, i_end = paths.size(); i < i_end; ++i) {
      result += (i ? "\\n" : "");
      result += paths[i];
    }

    auto wrapped = std::string("\"" + result + "\"");
    this->eval(resolveToRenderProcess(seq, "0", encodeURIComponent(wrapped)));
  }
}
