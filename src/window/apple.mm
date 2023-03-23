#include "window.hh"

@implementation SSCNavigationDelegate
- (void) webview: (SSCBridgedWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler {

  SSC::String base = webview.URL.absoluteString.UTF8String;
  SSC::String request = navigationAction.request.URL.absoluteString.UTF8String;

  if (request.find("file://") == 0 && request.find("http://localhost") == 0) {
    decisionHandler(WKNavigationActionPolicyCancel);
  } else {
    decisionHandler(WKNavigationActionPolicyAllow);
  }
}
@end

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
@implementation SSCBridgedWebView
@end
#else
@interface SSCWindowDelegate : NSObject <NSWindowDelegate, WKScriptMessageHandler>
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage;
@end

@implementation SSCWindowDelegate
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  // overloaded with `class_replaceMethod()`
}
@end

@implementation SSCBridgedWebView
SSC::Vector<SSC::String> draggablePayload;

int lastX = 0;
int lastY = 0;

- (BOOL) prepareForDragOperation: (id<NSDraggingInfo>)info {
  [info setDraggingFormation: NSDraggingFormationNone];
  return NO;
}

- (void) draggingExited: (id<NSDraggingInfo>)info {
  NSPoint pos = [info draggingLocation];
  auto x = std::to_string(pos.x);
  auto y = std::to_string([self frame].size.height - pos.y);

  SSC::String json = (
    "{\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = SSC::getEmitToRenderProcessJavaScript("dragend", json);
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

  SSC::String json = (
    "{\"count\":" + std::to_string(count) + ","
    "\"inbound\":" + (inbound ? "true" : "false") + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  return NSDragOperationGeneric;
}

- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>)info {
  [self draggingUpdated: info];

  auto payload = SSC::getEmitToRenderProcessJavaScript("dragenter", "{}");
  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()]
    completionHandler:nil];
  return NSDragOperationGeneric;
}

- (void) draggingEnded: (id<NSDraggingInfo>)info {
  NSPasteboard *pboard = [info draggingPasteboard];
  NSPoint pos = [info draggingLocation];
  int y = [self frame].size.height - pos.y;

  NSArray<Class> *classes = @[[NSURL class]];
  NSDictionary *options = @{};
  NSArray<NSURL*> *files = [pboard readObjectsForClasses:classes options:options];

  // if (NSPointInRect([info draggingLocation], self.frame)) {
    // NSWindow is (0,0) at bottom left, browser is (0,0) at top left
    // so we need to flip the y coordinate to convert to browser coordinates

  SSC::StringStream ss;
  int len = [files count];
  ss << "[";

  for (int i = 0; i < len; i++) {
    NSURL *url = files[i];
    SSC::String path = [[url path] UTF8String];
    // path = SSC::replace(path, "\"", "'");
    // path = SSC::replace(path, "\\", "\\\\");
    ss << "\"" << path << "\"";

    if (i < len - 1) {
      ss << ",";
    }
  }

  ss << "]";

  SSC::String json = (
    "{\"files\": " + ss.str() + ","
    "\"x\":" + std::to_string(pos.x) + ","
    "\"y\":" + std::to_string(y) + "}"
  );

  auto payload = SSC::getEmitToRenderProcessJavaScript("dropin", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  // }

  // [super draggingEnded:info];
}

- (void) updateEvent: (NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);
  auto count = std::to_string(draggablePayload.size());

  if (((int) location.x) == lastX || ((int) location.y) == lastY) {
    [super mouseDown:event];
    return;
  }

  SSC::String json = (
    "{\"count\":" + count + ","
    "\"x\":" + x + ","
    "\"y\":" + y + "}"
  );

  auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
}

- (void) mouseUp: (NSEvent*)event {
  [super mouseUp:event];

  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  int x = location.x;
  int y = location.y;

  auto sx = std::to_string(x);
  auto sy = std::to_string(y);

  auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    for (auto path : draggablePayload) {
      path = SSC::replace(path, "\"", "'");

      SSC::String json = (
        "{\"src\":\"" + path + "\","
        "\"x\":" + sx + ","
        "\"y\":" + sy + "}"
      );

      auto payload = SSC::getEmitToRenderProcessJavaScript("drop", json);

      [self evaluateJavaScript:
        [NSString stringWithUTF8String: payload.c_str()]
        completionHandler:nil];
    }
  }

  SSC::String json = (
    "{\"x\":" + sx + ","
    "\"y\":" + sy + "}"
  );

  auto payload = SSC::getEmitToRenderProcessJavaScript("dragend", json);

  [self evaluateJavaScript:
    [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
}

- (void) mouseDown: (NSEvent*)event {
  draggablePayload.clear();

  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  auto x = std::to_string(location.x);
  auto y = std::to_string(location.y);

  lastX = (int) location.x;
  lastY = (int) location.y;

  SSC::String js(
    "(() => {"
    "  const el = document.elementFromPoint(" + x + "," + y + ");"
    "  if (!el) return;"
    "  const found = el.matches('[data-src]') ? el : el.closest('[data-src]');"
    "  return found && found.dataset.src"
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

    SSC::Vector<SSC::String> files =
      SSC::split(SSC::String([result UTF8String]), ';');

    if (files.size() == 0) {
      [super mouseDown:event];
      return;
    }

    draggablePayload = files;
    [self updateEvent: event];
  }];
}

- (void) mouseDragged: (NSEvent*)event {
  NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
  [super mouseDragged:event];

  /* NSPoint currentLocation;
  NSPoint newOrigin;

  NSRect  screenFrame = [[NSScreen mainScreen] frame];
  NSRect  windowFrame = [self frame];

  currentLocation = [NSEvent mouseLocation];
  newOrigin.x = currentLocation.x - lastX;
  newOrigin.y = currentLocation.y - lastY;

  if ((newOrigin.y+windowFrame.size.height) > (screenFrame.origin.y+screenFrame.size.height)){
    newOrigin.y=screenFrame.origin.y + (screenFrame.size.height-windowFrame.size.height);
  }

  [[self window] setFrameOrigin:newOrigin]; */

  if (!NSPointInRect(location, self.frame)) {
    auto payload = SSC::getEmitToRenderProcessJavaScript("dragexit", "{}");
    [self evaluateJavaScript:
      [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  }

  if (draggablePayload.size() == 0) return;

  int x = location.x;
  int y = location.y;

  auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    auto sx = std::to_string(x);
    auto sy = std::to_string(y);
    auto count = std::to_string(draggablePayload.size());

    SSC::String json = (
      "{\"count\":" + count + ","
      "\"x\":" + sx + ","
      "\"y\":" + sy + "}"
    );

    auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json);

    [self evaluateJavaScript:
      [NSString stringWithUTF8String: payload.c_str()] completionHandler:nil];
  }

  if (NSPointInRect(location, self.frame)) return;

  NSPasteboard *pboard = [NSPasteboard pasteboardWithName: NSPasteboardNameDrag];
  [pboard declareTypes: @[(NSString*) kPasteboardTypeFileURLPromise] owner:self];

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
  SSC::String dest = [[url path] UTF8String];
  SSC::String src([[filePromiseProvider userInfo] UTF8String]);

  NSData *data = [@"" dataUsingEncoding:NSUTF8StringEncoding];
  [data writeToURL:url atomically:YES];

  SSC::String json = (
    "{\"src\":\"" + src + "\","
    "\"dest\":\"" + dest + "\"}"
  );

  SSC::String js = SSC::getEmitToRenderProcessJavaScript("dropout", json);

  [self
    evaluateJavaScript: [NSString stringWithUTF8String:js.c_str()]
     completionHandler: nil
  ];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*)filePromiseProvider fileNameForType:(NSString *)fileType {
  SSC::String file(std::to_string(SSC::rand64()) + ".download");
  return [NSString stringWithUTF8String:file.c_str()];
}
@end

namespace SSC {
  static bool isDelegateSet = false;

  Window::Window (App& app, WindowOptions opts) : app(app), opts(opts) {
    this->bridge = new IPC::Bridge(app.core);

    this->bridge->router.dispatchFunction = [this] (auto callback) {
      this->app.dispatch(callback);
    };

    this->bridge->router.evaluateJavaScriptFunction = [this](auto js) {
      dispatch_async(dispatch_get_main_queue(), ^{ this->eval(js); });
    };

    this->bridge->router.map("window.eval", [=](auto message, auto router, auto reply) {
      auto value = message.value;
      auto seq = message.seq;
      auto  script = [NSString stringWithUTF8String: value.c_str()];

      dispatch_async(dispatch_get_main_queue(), ^{
        [webview evaluateJavaScript: script completionHandler: ^(id result, NSError *error) {
          if (result) {
            auto msg = String([[NSString stringWithFormat:@"%@", result] UTF8String]);
            this->bridge->router.send(seq, msg, Post{});
          } else if (error) {
            auto exception = (NSString *) error.userInfo[@"WKJavaScriptExceptionMessage"];
            auto message = [[NSString stringWithFormat:@"%@", exception] UTF8String];
            auto err = encodeURIComponent(String(message));

            if (err == "(null)") {
              this->bridge->router.send(seq, "null", Post{});
              return;
            }

            auto json = JSON::Object::Entries {
              {"err", JSON::Object::Entries {
                {"message", String("Error: ") + err}
              }}
            };

            this->bridge->router.send(seq, JSON::Object(json).str(), Post{});
          } else {
            this->bridge->router.send(seq, "undefined", Post{});
          }
        }];
      });
    });

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

    window = [[NSWindow alloc]
        initWithContentRect: NSMakeRect(0, 0, opts.width, opts.height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO];

    NSArray* draggableTypes = [NSArray arrayWithObjects:
      NSPasteboardTypeURL,
      NSPasteboardTypeFileURL,
      (NSString*) kPasteboardTypeFileURLPromise,
      NSPasteboardTypeString,
      NSPasteboardTypeHTML,
      nil
		];

    // Position window in center of screen
    [window center];
    [window setOpaque: YES];
    // Minimum window size
    [window setContentMinSize: NSMakeSize(opts.minWidth, opts.minHeight)];
    [window setBackgroundColor: [NSColor controlBackgroundColor]];
    [window registerForDraggedTypes: draggableTypes];
    // [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent: true];
    }

    // window.movableByWindowBackground = true;
    window.titlebarAppearsTransparent = true;

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    // config.limitsNavigationsToAppBoundDomains = YES;

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"ipc"];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"socket"];

    WKPreferences* prefs = [config preferences];
    [prefs setJavaScriptCanOpenWindowsAutomatically:NO];

    if (SSC::isDebugEnabled()) {
      [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
    }

    WKUserContentController* controller = [config userContentController];

    // Add preload script, normalizing the interface to be cross-platform.
    SSC::String preload = ToString(createPreload(opts));

    WKUserScript* userScript = [WKUserScript alloc];

    [userScript
      initWithSource: [NSString stringWithUTF8String:preload.c_str()]
      injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO
    ];

    [controller addUserScript: userScript];

    webview = [[SSCBridgedWebView alloc]
      initWithFrame: NSZeroRect
      configuration: config
    ];

    [webview.configuration
      setValue: @YES
        forKey: @"allowUniversalAccessFromFileURLs"
    ];

    [webview.configuration.preferences
      setValue: @YES
        forKey: @"allowFileAccessFromFileURLs"
    ];

    /* [webview
      setValue: [NSNumber numberWithBool: YES]
        forKey: @"drawsTransparentBackground"
    ]; */

    /* [[NSNotificationCenter defaultCenter] addObserver: webview
                                             selector: @selector(systemColorsDidChangeNotification:)
                                                 name: NSSystemColorsDidChangeNotification
                                               object: nil
    ]; */

    // [webview registerForDraggedTypes:
    //  [NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    //
    bool exiting = false;

    SSCWindowDelegate* delegate = [SSCWindowDelegate alloc];
    [controller addScriptMessageHandler: delegate name: @"external"];

    // Set delegate to window
    [window setDelegate:delegate];

    SSCNavigationDelegate *navDelegate = [[SSCNavigationDelegate alloc] init];
    [webview setNavigationDelegate: navDelegate];

    if (!isDelegateSet) {
      isDelegateSet = true;

      class_replaceMethod(
        [SSCWindowDelegate class],
        @selector(windowShouldClose:),
        imp_implementationWithBlock(
          [&](id self, SEL cmd, id notification) {
            if (exiting) return true;

            auto window = (Window*) objc_getAssociatedObject(self, "window");

            if (window->opts.canExit) {
              exiting = true;
              window->exit(0);
              return true;
            }
            window->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
            window->hide();
            return false;
          }),
        "v@:@"
      );

      class_replaceMethod(
        [SSCWindowDelegate class],
        @selector(userContentController:didReceiveScriptMessage:),
        imp_implementationWithBlock(
          [=](id self, SEL cmd, WKScriptMessage* scriptMessage) {
            auto window = (Window*) objc_getAssociatedObject(self, "window");

            if (!scriptMessage) return;
            id body = [scriptMessage body];
            if (!body || ![body isKindOfClass:[NSString class]]) {
              return;
            }

            String uri = [body UTF8String];
            if (!uri.size()) return;

            if (!bridge->route(uri, nullptr, 0)) {
              if (window != nullptr && window->onMessage != nullptr) {
                window->onMessage(uri);
              }
            }
          }),
        "v@:@"
      );

      class_addMethod(
        [SSCWindowDelegate class],
        @selector(menuItemSelected:),
        imp_implementationWithBlock(
          [=](id self, SEL _cmd, id item) {
            auto window = (Window*) objc_getAssociatedObject(self, "window");

            id menuItem = (id) item;
            SSC::String title = [[menuItem title] UTF8String];
            SSC::String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
            SSC::String parent = [[[menuItem menu] title] UTF8String];
            SSC::String seq = std::to_string([menuItem tag]);

            window->eval(getResolveMenuSelectionJavaScript(seq, title, parent));
          }),
        "v@:@:@:"
      );
    }

    objc_setAssociatedObject(
      delegate,
      "window",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    // Initialize application
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];

    if (opts.headless) {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      // Sets the app as the active app
      [NSApp activateIgnoringOtherApps: YES];
    }

    // Add webview to window
    [window setContentView: webview];

    navigate("0", opts.url);
  }

  ScreenSize Window::getScreenSize () {
    NSRect e = [[NSScreen mainScreen] frame];

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void Window::show () {
    if (this->opts.headless == true) {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      [window makeKeyAndOrderFront: nil];
      [NSApp activateIgnoringOtherApps: YES];
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

  void Window::hide () {
    [window orderOut:window];
    this->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
  }

  void Window::eval (const SSC::String& js) {
    [webview evaluateJavaScript:
      [NSString stringWithUTF8String:js.c_str()]
      completionHandler:nil];
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    NSMenu* menuBar = [NSApp mainMenu];
    NSArray* menuBarItems = [menuBar itemArray];

    NSMenu* menu = menuBarItems[barPos];
    if (!menu) return;

    NSArray* menuItems = [menu itemArray];
    NSMenuItem* menuItem = menuItems[menuPos];

    if (!menuItem) return;

    [menuItem setTarget: nil];
    [menuItem setAction: NULL];
  }

  void Window::navigate (const SSC::String& seq, const SSC::String& value) {
    auto url = [NSURL URLWithString: [NSString stringWithUTF8String: value.c_str()]];

    if (url != nullptr) {
      if (String(url.scheme.UTF8String) == "file") {
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

        [webview
          loadHTMLString: [NSString stringWithUTF8String: html.c_str()]
                 baseURL: [url URLByDeletingLastPathComponent]
        ];
      } else {
        auto request = [NSMutableURLRequest requestWithURL: url];
        [webview loadRequest: request];
      }

      if (seq.size() > 0) {
        auto index = std::to_string(this->opts.index);
        this->resolvePromise(seq, "0", index);
      }
    }
  }

  SSC::String Window::getTitle () {
    return SSC::String([this->window.title UTF8String]);
  }

  void Window::setTitle (const SSC::String& value) {
    [window setTitle:[NSString stringWithUTF8String:value.c_str()]];
  }

  ScreenSize Window::getSize () {
    NSRect e = this->window.frame;

    this->height = e.size.height;
    this->width = e.size.width;

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void Window::setSize (int width, int height, int hints) {
    [window setFrame: NSMakeRect(0.f, 0.f, (float) width, (float) height)
             display: YES
             animate: NO];

    [window center];

    this->height = height;
    this->width = width;
  }

  int Window::openExternal (const SSC::String& s) {
    NSString* nsu = [NSString stringWithUTF8String:s.c_str()];
    return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: nsu]];
  }

  void Window::closeContextMenu () {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu (const SSC::String &seq) {
    // @TODO(jwerle)
  }

  void Window::showInspector () {
    // This is a private method on the webview, so we need to use
    // the pragma keyword to suppress the access warning.
    #pragma clang diagnostic ignored "-Wobjc-method-access"
    [[this->webview _inspector] show];
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    CGFloat sRGBComponents[4] = { r / 255.0, g / 255.0, b / 255.0, a };
    NSColorSpace *colorSpace = [NSColorSpace sRGBColorSpace];

    [window setBackgroundColor:
      [NSColor colorWithColorSpace: colorSpace
                        components: sRGBComponents
                             count: 4]
    ];
  }

  void Window::setContextMenu (const SSC::String& seq, const SSC::String& value) {
    auto menuItems = split(value, '_');
    auto id = std::stoi(seq.substr(1)); // remove the 'R' prefix

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

  void Window::setSystemMenu (const SSC::String& seq, const SSC::String& value) {
    SSC::String menu = SSC::String(value);

    NSMenu *mainMenu;
    NSString *title;
    NSMenu *appleMenu;
    NSMenu *serviceMenu;
    NSMenu *windowMenu;
    NSMenu *editMenu;
    NSMenu *dynamicMenu;
    NSMenuItem *menuItem;

    if (NSApp == nil) return;

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
      bool isDisabled = false;

      for (int i = 1; i < menu.size(); i++) {
        auto line = trim(menu[i]);
        if (line.empty()) continue;
        auto parts = split(line, ':');
        auto title = parts[0];
        NSUInteger mask = 0;
        SSC::String key = "";

        if (title.size() > 0 && title.find("!") == 0) {
          title = title.substr(1);
          isDisabled = true;
        }

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

        if (isDisabled) {
          [menuItem setTarget:nil];
          [menuItem setAction:NULL];
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
      this->resolvePromise(seq, "0", index);
    }
  }

  void Window::openDialog (
    const SSC::String& seq,
    bool isSave,
    bool allowDirs,
    bool allowFiles,
    bool allowMultiple,
    const SSC::String& defaultPath = "",
    const SSC::String& title = "",
    const SSC::String& defaultName = "")
  {

    NSURL *url;
    const char *utf8_path;
    NSSavePanel *dialog_save;
    NSOpenPanel *dialog_open;
    NSURL *default_url = nil;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    if (isSave) {
      dialog_save = [NSSavePanel savePanel];
      [dialog_save setTitle: [NSString stringWithUTF8String:title.c_str()]];
    } else {
      dialog_open = [NSOpenPanel openPanel];
      // open does not support title for some reason
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
        SSC::String url = (char*) [[[dialog_save URL] path] UTF8String];
        auto wrapped = SSC::String("\"" + url + "\"");
        this->resolvePromise(seq, "0", encodeURIComponent(wrapped));
      }
      return;
    }

    SSC::String result = "";
    SSC::Vector<SSC::String> paths;
    NSArray* urls;

    if ([dialog_open runModal] == NSModalResponseOK) {
      urls = [dialog_open URLs];

      for (NSURL* url in urls) {
        if ([url isFileURL]) {
          paths.push_back(SSC::String((char*) [[url path] UTF8String]));
        }
      }
    }

    [pool release];

    for (size_t i = 0, i_end = paths.size(); i < i_end; ++i) {
      result += (i ? "\\n" : "");
      result += paths[i];
    }

    auto wrapped = SSC::String("\"" + result + "\"");
    this->resolvePromise(seq, "0", encodeURIComponent(wrapped));
  }
}
#endif
