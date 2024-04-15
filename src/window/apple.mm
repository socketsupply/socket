#include "window.hh"
#include "../ipc/ipc.hh"

@implementation SSCNavigationDelegate
-    (void) webView: (WKWebView*) webView
  didFailNavigation: (WKNavigation*) navigation
          withError: (NSError*) error
{
  // TODO(@jwerle)
}

-               (void) webView: (WKWebView*) webView
  didFailProvisionalNavigation: (WKNavigation*) navigation
                     withError: (NSError*) error {
  // TODO(@jwerle)
}

-                    (void) webView: (WKWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler
{
  if (
    webview != nullptr &&
    webview.URL != nullptr &&
    webview.URL.absoluteString.UTF8String != nullptr &&
    navigationAction != nullptr &&
    navigationAction.request.URL.absoluteString.UTF8String != nullptr
  ) {
    auto userConfig = self.bridge->userConfig;
    static const auto devHost = SSC::getDevHost();
    static const auto links = SSC::parseStringList(userConfig["meta_application_links"], ' ');

    auto base = SSC::String(webview.URL.absoluteString.UTF8String);
    auto request = SSC::String(navigationAction.request.URL.absoluteString.UTF8String);

    const auto applinks = SSC::parseStringList(userConfig["meta_application_links"], ' ');
    bool hasAppLink = false;

    if (applinks.size() > 0 && navigationAction.request.URL.host != nullptr) {
      auto host = SSC::String(navigationAction.request.URL.host.UTF8String);
      for (const auto& applink : applinks) {
        const auto parts = SSC::split(applink, '?');
        if (host == parts[0]) {
          hasAppLink = true;
          break;
        }
      }
    }

    if (hasAppLink) {
      if (self.bridge != nullptr) {
        decisionHandler(WKNavigationActionPolicyCancel);
        SSC::JSON::Object json = SSC::JSON::Object::Entries {{
          "url", request
        }};

        self.bridge->router.emit("applicationurl", json.str());
        return;
      }
    }

    if (
      userConfig["meta_application_protocol"].size() > 0 &&
      request.starts_with(userConfig["meta_application_protocol"]) &&
      !request.starts_with("socket://" + userConfig["meta_bundle_identifier"])
    ) {
      if (self.bridge != nullptr) {
        decisionHandler(WKNavigationActionPolicyCancel);

        SSC::JSON::Object json = SSC::JSON::Object::Entries {{
          "url", request
        }};

        self.bridge->router.emit("applicationurl", json.str());
        return;
      }
    }

    if (!request.starts_with("socket:") && !request.starts_with(devHost)) {
      decisionHandler(WKNavigationActionPolicyCancel);
      return;
    }
  }

  decisionHandler(WKNavigationActionPolicyAllow);
}

-                    (void) webView: (WKWebView*) webView
  decidePolicyForNavigationResponse: (WKNavigationResponse*) navigationResponse
                    decisionHandler: (void (^)(WKNavigationResponsePolicy)) decisionHandler {
  decisionHandler(WKNavigationResponsePolicyAllow);
}
@end

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
  @implementation SSCWindowDelegate
  @end
  @implementation SSCWindow : NSObject
  @end
#else
  @implementation SSCWindow
    - (void)layoutIfNeeded {
      [super layoutIfNeeded];

      if (self.titleBarView == nullptr || self.titleBarView.subviews.count > 0) return;

      NSButton *closeButton = [self standardWindowButton:NSWindowCloseButton];
      NSButton *minimizeButton = [self standardWindowButton:NSWindowMiniaturizeButton];
      NSButton *zoomButton = [self standardWindowButton:NSWindowZoomButton];

      if (closeButton && minimizeButton && zoomButton) {
        [self.titleBarView addSubview: closeButton];
        [self.titleBarView addSubview: minimizeButton];
        [self.titleBarView addSubview: zoomButton];
      }
    }

    - (void)sendEvent:(NSEvent *)event {
      if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeLeftMouseDragged) {
        if (event.type == NSEventTypeLeftMouseDown) {
          [self.webview mouseDown:event];
        }

        if (event.type == NSEventTypeLeftMouseDragged) {
          [self.webview mouseDragged:event];
          return;
        }
      }

      [super sendEvent:event];
    }
  @end
  @implementation SSCWindowDelegate
    - (void) userContentController: (WKUserContentController*) userContentController didReceiveScriptMessage: (WKScriptMessage*) scriptMessage {
    }
  @end
#endif

@implementation SSCBridgedWebView
#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
SSC::Vector<SSC::String> draggablePayload;
CGFloat MACOS_TRAFFIC_LIGHT_BUTTON_SIZE = 16;
int lastX = 0;
int lastY = 0;

- (void)viewDidChangeEffectiveAppearance {
  [super viewDidChangeEffectiveAppearance];

  SSCWindow *window = (SSCWindow*) self.window;

  if (@available(macOS 10.14, *)) {
    if ([window.effectiveAppearance.name containsString:@"Dark"]) {
      [window setBackgroundColor:[NSColor colorWithCalibratedWhite:0.1 alpha:1.0]]; // Dark mode color
    } else {
      [window setBackgroundColor:[NSColor colorWithCalibratedWhite:1.0 alpha:1.0]]; // Light mode color
    }
  }
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize {
  [super resizeSubviewsWithOldSize:oldSize];

  SSCWindow *w = (SSCWindow*) self.window;
  // if (w.titleBarView.subviews.count > 0) return;

  CGFloat viewWidth = w.titleBarView.frame.size.width;
  CGFloat viewHeight = w.titleBarView.frame.size.height;
  CGFloat newX = w.windowControlOffsets.x;
  CGFloat newY = 0.f;

  NSButton *closeButton = [w standardWindowButton:NSWindowCloseButton];
  NSButton *minimizeButton = [w standardWindowButton:NSWindowMiniaturizeButton];
  NSButton *zoomButton = [w standardWindowButton:NSWindowZoomButton];

  if (closeButton && minimizeButton && zoomButton) {
    [w.titleBarView addSubview: closeButton];
    [w.titleBarView addSubview: minimizeButton];
    [w.titleBarView addSubview: zoomButton];
  }

  w.titleBarView.frame = NSMakeRect(newX, newY, viewWidth, viewHeight);
}

- (instancetype)initWithFrame:(NSRect)frameRect configuration:(WKWebViewConfiguration *)configuration radius:(CGFloat)radius margin:(CGFloat)margin {
  self = [super initWithFrame:frameRect configuration: configuration];

  if (self && radius > 0.0) {
    self.radius = radius;
    self.margin = margin;
    self.layer.cornerRadius = radius;
    self.layer.masksToBounds = YES;
  }

  return self;
}

- (void)layout {
  [super layout];
  
  NSRect bounds = self.superview.bounds;

  if (self.radius > 0.0) {
    if (self.contentHeight == 0.0) {
      self.contentHeight = self.superview.bounds.size.height - self.bounds.size.height;
    }

    bounds.size.height = bounds.size.height - self.contentHeight;
  }

  if (self.margin > 0.0) {
    CGFloat borderWidth = self.margin;
    self.frame = NSInsetRect(bounds, borderWidth, borderWidth);
  }
}

- (BOOL) wantsPeriodicDraggingUpdates {
  return YES;
}

- (BOOL) prepareForDragOperation: (id<NSDraggingInfo>) info {
  [info setDraggingFormation: NSDraggingFormationNone];
  return YES;
}

- (BOOL) performDragOperation: (id<NSDraggingInfo>) info {
  return YES;
}

- (void) concludeDragOperation: (id<NSDraggingInfo>) info {
}

- (void) updateDraggingItemsForDrag: (id<NSDraggingInfo>) info {
}

- (NSDragOperation) draggingEntered: (id<NSDraggingInfo>) info {
  const auto json = SSC::JSON::Object {};
  const auto payload = SSC::getEmitToRenderProcessJavaScript("dragenter", json.str());
  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
  [self draggingUpdated: info];
  return NSDragOperationGeneric;
}

- (NSDragOperation) draggingUpdated: (id<NSDraggingInfo>) info {
  const auto position = info.draggingLocation;
  const auto x = std::to_string(position.x);
  const auto y = std::to_string(self.frame.size.height - position.y);

  auto count = draggablePayload.size();
  auto inbound = false;

  if (count == 0) {
    inbound = true;
    count = [info numberOfValidItemsForDrop];
  }

  const auto data = SSC::JSON::Object::Entries {
    {"count", (unsigned int) count},
    {"inbound", inbound},
    {"x", x},
    {"y", y}
  };

  const auto json = SSC::JSON::Object {data};
  const auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
  return [super draggingUpdated: info];
}

- (void) draggingExited: (id<NSDraggingInfo>) info {
  const auto position = info.draggingLocation;
  const auto x = std::to_string(position.x);
  const auto y = std::to_string(self.frame.size.height - position.y);

  const auto data = SSC::JSON::Object::Entries {
    {"x", x},
    {"y", y}
  };

  const auto json = SSC::JSON::Object {data};
  const auto payload = SSC::getEmitToRenderProcessJavaScript("dragend", json.str());

  draggablePayload.clear();

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
}

- (void) draggingEnded: (id<NSDraggingInfo>) info {
  const auto pasteboard = info.draggingPasteboard;
  const auto position = info.draggingLocation;
  const auto x = position.x;
  const auto y = self.frame.size.height - position.y;

  const auto pasteboardFiles = [pasteboard
    readObjectsForClasses: @[NSURL.class]
                  options: @{}
  ];

  auto files = SSC::JSON::Array::Entries {};

  for (NSURL* file in pasteboardFiles) {
    files.push_back(file.path.UTF8String);
  }

  const auto data = SSC::JSON::Object::Entries {
    {"files", files},
    {"x", x},
    {"y", y}
  };

  const auto json = SSC::JSON::Object { data };
  const auto payload = SSC::getEmitToRenderProcessJavaScript("dropin", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
}

- (void) updateEvent: (NSEvent*) event {
  const auto location = [self convertPoint: event.locationInWindow fromView :nil];
  const auto x = std::to_string(location.x);
  const auto y = std::to_string(location.y);
  const auto count = draggablePayload.size();

  if (((int) location.x) == lastX || ((int) location.y) == lastY) {
    return [super mouseDown: event];
  }

  const auto data = SSC::JSON::Object::Entries {
    {"count", (unsigned int) count},
    {"x", x},
    {"y", y}
  };

  const auto json = SSC::JSON::Object { data };
  const auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
}

- (void) mouseUp: (NSEvent*) event {
  [super mouseUp: event];

  const auto location = [self convertPoint: event.locationInWindow fromView: nil];
  const auto x = location.x;
  const auto y = location.y;

  const auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  const auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    for (const auto& path : draggablePayload) {
      const auto data = SSC::JSON::Object::Entries {
        {"src", path},
        {"x", x},
        {"y", y}
      };

      const auto json = SSC::JSON::Object { data };
      const auto payload = SSC::getEmitToRenderProcessJavaScript("drop", json.str());

      [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
    }
  }

  const auto data = SSC::JSON::Object::Entries {
    {"x", x},
    {"y", y}
  };

  const auto json = SSC::JSON::Object { data };
  auto payload = SSC::getEmitToRenderProcessJavaScript("dragend", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
}

- (void) mouseDown: (NSEvent*) event {
  self.shouldDrag = false;
  draggablePayload.clear();

  const auto location = [self convertPoint: event.locationInWindow fromView: nil];
  const auto x = std::to_string(location.x);
  const auto y = std::to_string(location.y);

  self.initialWindowPos = location;

  lastX = (int) location.x;
  lastY = (int) location.y;

  SSC::String js(
    "(() => {                                                                      "
    "  const v = '--app-region';                                                   "
    "  let el = document.elementFromPoint(" + x + "," + y + ");                    "
    "                                                                              "
    "  while (el) {                                                                "
    "    if (getComputedStyle(el).getPropertyValue(v) == 'drag') return 'movable'; "
    "    el = el.parentElement;                                                    "
    "  }                                                                           "
    "  return ''                                                                   "
    "})()                                                                          "
  );

  [self
    evaluateJavaScript: @(js.c_str())
     completionHandler: ^(id result, NSError *error)
  {
    if (error) {
      NSLog(@"%@", error);
      [super mouseDown: event];
      return;
    }

    if (![result isKindOfClass: NSString.class]) {
      [super mouseDown: event];
      return;
    }

    const auto match = SSC::String([result UTF8String]);

    if (match.compare("movable") != 0) {
      [super mouseDown: event];
      return;
    }

    self.shouldDrag = true;
    [self updateEvent: event];
  }];
}

- (void) mouseDragged: (NSEvent*) event {
  NSPoint currentLocation = [self convertPoint:event.locationInWindow fromView:nil];

  if (self.shouldDrag) {
    CGFloat deltaX = currentLocation.x - self.initialWindowPos.x;
    CGFloat deltaY = currentLocation.y - self.initialWindowPos.y;

    NSRect frame = self.window.frame;
    frame.origin.x += deltaX;
    frame.origin.y -= deltaY;

    [self.window setFrame:frame display:YES];
  }

  [super mouseDragged:event];

  if (!NSPointInRect(currentLocation, self.frame)) {
    auto payload = SSC::getEmitToRenderProcessJavaScript("dragexit", "{}");
    [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
  }

  /*

  // TODO(@heapwolf): refactor the legacy native multi-file drag-drop stuff

  if (draggablePayload.size() == 0) {
    return;
  }

  const auto x = location.x;
  const auto y = location.y;
  const auto significantMoveX = (lastX - x) > 6 || (x - lastX) > 6;
  const auto significantMoveY = (lastY - y) > 6 || (y - lastY) > 6;

  if (significantMoveX || significantMoveY) {
    const auto data = SSC::JSON::Object::Entries {
      {"count", (unsigned int) draggablePayload.size()},
      {"x", x},
      {"y", y}
    };

    const auto json = SSC::JSON::Object { data };
    const auto payload = SSC::getEmitToRenderProcessJavaScript("drag", json.str());

    [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
  }

  if (NSPointInRect(location, self.frame)) {
    return;
  }

  const auto pasteboard = [NSPasteboard pasteboardWithName: NSPasteboardNameDrag];
  const auto dragItems = [NSMutableArray new];
  const auto iconSize = NSMakeSize(32, 32); // according to documentation

  [pasteboard declareTypes: @[(NSString*) kPasteboardTypeFileURLPromise] owner:self];

  auto dragPosition = [self convertPoint: event.locationInWindow fromView: nil];
  dragPosition.x -= 16;
  dragPosition.y -= 16;

  NSRect imageLocation;
  imageLocation.origin = dragPosition;
  imageLocation.size = iconSize;

  for (const auto& file : draggablePayload) {
    const auto url = [NSURL fileURLWithPath: @(file.c_str())];
    const auto icon = [NSWorkspace.sharedWorkspace iconForContentType: UTTypeURL];

    NSArray* (^providerBlock)() = ^NSArray* () {
      const auto component = [
        [NSDraggingImageComponent.alloc initWithKey: NSDraggingImageComponentIconKey
      ] retain];

      component.frame = NSMakeRect(0, 0, iconSize.width, iconSize.height);
      component.contents = icon;
      return @[component];
    };

    auto provider = [NSFilePromiseProvider.alloc initWithFileType: @"public.url" delegate: self];

    [provider setUserInfo: @(file.c_str())];

    auto dragItem = [NSDraggingItem.alloc initWithPasteboardWriter: provider];

    dragItem.draggingFrame = NSMakeRect(
      dragPosition.x,
      dragPosition.y,
      iconSize.width,
      iconSize.height
    );

    dragItem.imageComponentsProvider = providerBlock;
    [dragItems addObject: dragItem];
  }

  auto session = [self
      beginDraggingSessionWithItems: dragItems
                              event: event
                             source: self
  ];

  session.draggingFormation = NSDraggingFormationPile;
  draggablePayload.clear();
  */
}

-       (NSDragOperation) draggingSession: (NSDraggingSession*) session
    sourceOperationMaskForDraggingContext: (NSDraggingContext) context
{
  return NSDragOperationGeneric;
}

- (void) filePromiseProvider: (NSFilePromiseProvider*) filePromiseProvider
           writePromiseToURL: (NSURL*) url
           completionHandler: (void (^)(NSError *errorOrNil)) completionHandler
{
  const auto dest = SSC::String(url.path.UTF8String);
  const auto src = SSC::String([filePromiseProvider.userInfo UTF8String]);
  const auto data = [@"" dataUsingEncoding: NSUTF8StringEncoding];

  [data writeToURL: url atomically: YES];

  const auto json = SSC::JSON::Object {
    SSC::JSON::Object::Entries {
      {"src", src},
      {"dest", dest}
    }
  };

  const auto payload = SSC::getEmitToRenderProcessJavaScript("dropout", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*) filePromiseProvider
                  fileNameForType: (NSString*) fileType
{
  const auto id = SSC::rand64();
  const auto filename = std::to_string(id) + ".download";
  return @(filename.c_str());
}

-             (void) webView: (WKWebView*) webView
  runOpenPanelWithParameters: (WKOpenPanelParameters*) parameters
            initiatedByFrame: (WKFrameInfo*) frame
           completionHandler: (void (^)(NSArray<NSURL*>*URLs)) completionHandler
{
  const auto acceptedFileExtensions = parameters._acceptedFileExtensions;
  const auto acceptedMIMETypes = parameters._acceptedMIMETypes;
  SSC::StringStream contentTypesSpec;

  for (NSString* acceptedMIMEType in acceptedMIMETypes) {
    contentTypesSpec << acceptedMIMEType.UTF8String << "|";
  }

  if (acceptedFileExtensions.count > 0) {
    contentTypesSpec << "*/*:";
    const auto count = acceptedFileExtensions.count;
    int seen = 0;
    for (NSString* acceptedFileExtension in acceptedFileExtensions) {
      const auto string = SSC::String(acceptedFileExtension.UTF8String);

      if (!string.starts_with(".")) {
        contentTypesSpec << ".";
      }

      contentTypesSpec << string;
      if (++seen < count) {
        contentTypesSpec << ",";
      }
    }
  }

  auto contentTypes = SSC::trim(contentTypesSpec.str());

  if (contentTypes.size() == 0) {
    contentTypes = "*/*";
  }

  if (contentTypes.ends_with("|")) {
    contentTypes = contentTypes.substr(0, contentTypes.size() - 1);
  }

  const auto options = SSC::Dialog::FileSystemPickerOptions {
    .directories = false,
    .multiple = parameters.allowsMultipleSelection ? true : false,
    .contentTypes = contentTypes,
    .defaultName = "",
    .defaultPath = "",
    .title = "Choose a File"
  };

  SSC::Dialog dialog;
  const auto results = dialog.showOpenFilePicker(options);

  if (results.size() == 0) {
    completionHandler(nullptr);
    return;
  }

  auto urls = [NSMutableArray new];

  for (const auto& result : results) {
    [urls addObject: [NSURL URLWithString: @(result.c_str())]];
  }

  completionHandler(urls);
}
#endif

#if (!TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR) || (TARGET_OS_IPHONE && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)
-                                      (void) webView: (WKWebView*) webView
 requestDeviceOrientationAndMotionPermissionForOrigin: (WKSecurityOrigin*) origin
                                     initiatedByFrame: (WKFrameInfo*) frame
                                      decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler {
  static auto userConfig = SSC::getUserConfig();

  if (userConfig["permissions_allow_device_orientation"] == "false") {
    decisionHandler(WKPermissionDecisionDeny);
    return;
  }

  decisionHandler(WKPermissionDecisionGrant);
}

-                        (void) webView: (WKWebView*) webView
 requestMediaCapturePermissionForOrigin: (WKSecurityOrigin*) origin
                       initiatedByFrame: (WKFrameInfo*) frame
                                   type: (WKMediaCaptureType) type
                        decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler {
  static auto userConfig = SSC::getUserConfig();

  if (userConfig["permissions_allow_user_media"] == "false") {
    decisionHandler(WKPermissionDecisionDeny);
    return;
  }

  if (type == WKMediaCaptureTypeCameraAndMicrophone) {
    if (
      userConfig["permissions_allow_camera"] == "false" ||
      userConfig["permissions_allow_microphone"] == "false"
    ) {
      decisionHandler(WKPermissionDecisionDeny);
      return;
    }
  }

  if (
    type == WKMediaCaptureTypeCamera &&
    userConfig["permissions_allow_camera"] == "false"
  ) {
    decisionHandler(WKPermissionDecisionDeny);
    return;
  }

  if (
    type == WKMediaCaptureTypeMicrophone &&
    userConfig["permissions_allow_microphone"] == "false"
  ) {
    decisionHandler(WKPermissionDecisionDeny);
    return;
  }

  decisionHandler(WKPermissionDecisionGrant);
}
#endif

-                     (void) webView: (WKWebView*) webView
  runJavaScriptAlertPanelWithMessage: (NSString*) message
                    initiatedByFrame: (WKFrameInfo*) frame
                   completionHandler: (void (^)(void)) completionHandler {
  static auto userConfig = SSC::getUserConfig();
  auto title = userConfig["meta_title"] + ":";

  if (userConfig.contains("window_alert_title")) {
    title = userConfig["window_alert_title"];
  }

#if TARGET_OS_IPHONE || TARGET_OS_IPHONE
  auto alert = [UIAlertController
    alertControllerWithTitle: @(title.c_str())
                     message: message
              preferredStyle: UIAlertControllerStyleAlert
  ];

  auto ok = [UIAlertAction
    actionWithTitle: @"OK"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler();
  }];

  [alert addAction: ok];

  [webView presentViewController:alert animated: YES completion: nil];
#else
  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText: @(title.c_str())];
  [alert setInformativeText: message];
  [alert addButtonWithTitle: @"OK"];
  [alert runModal];
  completionHandler();
#endif
}

-                       (void) webView: (WKWebView*) webView
  runJavaScriptConfirmPanelWithMessage: (NSString*) message
                      initiatedByFrame: (WKFrameInfo*) frame
                     completionHandler: (void (^)(BOOL result)) completionHandler {
  static auto userConfig = SSC::getUserConfig();
  auto title = userConfig["meta_title"] + ":";

  if (userConfig.contains("window_alert_title")) {
    title = userConfig["window_alert_title"];
  }
#if TARGET_OS_IPHONE || TARGET_OS_IPHONE
  auto alert = [UIAlertController
    alertControllerWithTitle: @(title.c_str())
                     message: message
              preferredStyle: UIAlertControllerStyleAlert
  ];

  auto ok = [UIAlertAction
    actionWithTitle: @"OK"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler(YES);
  }];

  auto cancel = [UIAlertAction
    actionWithTitle: @"Cancel"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler(NO);
  }];

  [alert addAction: ok];
  [alert addAction: cancel];

  [webView presentViewController: alert animated: YES completion: nil];
#else
  NSAlert *alert = [[NSAlert alloc] init];
  [alert setMessageText: @(title.c_str())];
  [alert setInformativeText: message];
  [alert addButtonWithTitle: @"OK"];
  [alert addButtonWithTitle: @"Cancel"];
  completionHandler([alert runModal] == NSAlertFirstButtonReturn);
#endif
}

@end

#if !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
namespace SSC {
  static bool isDelegateSet = false;

  Window::Window (App& app, WindowOptions opts)
    : app(app),
      opts(opts),
      hotkey(this)
  {
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled;

    // Set window to be resizable
    if (opts.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (opts.closable) {
      style |= NSWindowStyleMaskClosable;
    }

    if (opts.minimizable) {
      style |= NSWindowStyleMaskMiniaturizable;
    }

    window = [[SSCWindow alloc]
        initWithContentRect: NSMakeRect(0, 0, opts.width, opts.height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO];

    if (opts.maximizable == false) {
      [window setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
    }

    NSArray* draggableTypes = [NSArray arrayWithObjects:
      NSPasteboardTypeURL,
      NSPasteboardTypeFileURL,
      (NSString*) kPasteboardTypeFileURLPromise,
      NSPasteboardTypeString,
      NSPasteboardTypeHTML,
      nil
		];

    // window.movableByWindowBackground = true;
    window.titlebarAppearsTransparent = true;

    auto userConfig = opts.userConfig;

    this->index = opts.index;
    this->bridge = new IPC::Bridge(app.core, userConfig);
    this->hotkey.init(this->bridge);

    this->bridge->router.dispatchFunction = [this] (auto callback) {
      this->app.dispatch(callback);
    };

    this->bridge->router.evaluateJavaScriptFunction = [this](auto source) {
      dispatch_async(dispatch_get_main_queue(), ^{
        this->eval(source);
      });
    };

    this->bridge->router.map("window.eval", [=, this](auto message, auto router, auto reply) {
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

    // Initialize WKWebView
    WKWebViewConfiguration* config = [WKWebViewConfiguration new];
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    config.limitsNavigationsToAppBoundDomains = YES;

    [config setValue: @NO forKey: @"crossOriginAccessControlCheckEnabled"];

    WKPreferences* prefs = [config preferences];
    prefs.javaScriptCanOpenWindowsAutomatically = YES;

    @try {
      if (userConfig["permissions_allow_fullscreen"] == "false") {
        [prefs setValue: @NO forKey: @"fullScreenEnabled"];
        [prefs setValue: @NO forKey: @"elementFullscreenEnabled"];
      } else {
        [prefs setValue: @YES forKey: @"fullScreenEnabled"];
        [prefs setValue: @YES forKey: @"elementFullscreenEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'fullScreenEnabled': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_fullscreen"] == "false") {
        [prefs setValue: @NO forKey: @"elementFullscreenEnabled"];
      } else {
        [prefs setValue: @YES forKey: @"elementFullscreenEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'elementFullscreenEnabled': %@", error);
    }

    if (opts.debug || SSC::isDebugEnabled()) {
      [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
      if (@available(macOS 13.3, iOS 16.4, tvOS 16.4, *)) {
        [webview setInspectable: YES];
      }
    }

    @try {
      if (userConfig["permissions_allow_clipboard"] == "false") {
        [prefs setValue: @NO forKey: @"javaScriptCanAccessClipboard"];
      } else {
        [prefs setValue: @YES forKey: @"javaScriptCanAccessClipboard"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'javaScriptCanAccessClipboard': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_data_access"] == "false") {
        [prefs setValue: @NO forKey: @"storageAPIEnabled"];
      } else {
        [prefs setValue: @YES forKey: @"storageAPIEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'storageAPIEnabled': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_device_orientation"] == "false") {
        [prefs setValue: @NO forKey: @"deviceOrientationEventEnabled"];
      } else {
        [prefs setValue: @YES forKey: @"deviceOrientationEventEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'deviceOrientationEventEnabled': %@", error);
    }

    if (userConfig["permissions_allow_notifications"] == "false") {
      @try {
        [prefs setValue: @NO forKey: @"appBadgeEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'deviceOrientationEventEnabled': %@", error);
      }

      @try {
        [prefs setValue: @NO forKey: @"notificationsEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationsEnabled': %@", error);
      }

      @try {
        [prefs setValue: @NO forKey: @"notificationEventEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationEventEnabled': %@", error);
      }
    } else {
      @try {
        [prefs setValue: @YES forKey: @"appBadgeEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'appBadgeEnabled': %@", error);
      }
    }

  #if !TARGET_OS_IPHONE
    @try {
      [prefs setValue: @YES forKey: @"cookieEnabled"];

      if (userConfig["permissions_allow_user_media"] == "false") {
        [prefs setValue: @NO forKey: @"mediaStreamEnabled"];
      } else {
        [prefs setValue: @YES forKey: @"mediaStreamEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'mediaStreamEnabled': %@", error);
    }
  #endif

    @try {
      if (userConfig["permissions_allow_airplay"] == "false") {
        config.allowsAirPlayForMediaPlayback = NO;
      } else {
        config.allowsAirPlayForMediaPlayback = YES;
      }
    } @catch (NSException *error) {
      debug("Failed to set preference 'allowsAirPlayForMediaPlayback': %@", error);
    }

    config.defaultWebpagePreferences.allowsContentJavaScript = YES;
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    config.websiteDataStore = WKWebsiteDataStore.defaultDataStore;
    config.processPool = [WKProcessPool new];

    [config.websiteDataStore.httpCookieStore
        setCookiePolicy: WKCookiePolicyAllow
      completionHandler: ^(){}
    ];

    [config
      setValue: @YES
        forKey: @"allowUniversalAccessFromFileURLs"
    ];

    [config.preferences
      setValue: @YES
        forKey: @"allowFileAccessFromFileURLs"
    ];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"npm"];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"ipc"];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"socket"];

    [config setURLSchemeHandler: bridge->router.schemeHandler
                   forURLScheme: @"node"];

    for (const auto& entry : split(opts.userConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (app.core->protocolHandlers.registerHandler(scheme)) {
        [config setURLSchemeHandler: bridge->router.schemeHandler
                      forURLScheme: @(replace(trim(scheme), ":", "").c_str())];
      }
    }

    for (const auto& entry : opts.userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        const auto data = entry.second;
        if (app.core->protocolHandlers.registerHandler(scheme, { data })) {
          [config setURLSchemeHandler: bridge->router.schemeHandler
                         forURLScheme: @(scheme.c_str())];
        }
      }
    }

    static const auto devHost = SSC::getDevHost();
    if (devHost.starts_with("http:")) {
      [config.processPool
        performSelector: @selector(_registerURLSchemeAsSecure:)
        withObject: @"http"
      ];
    }

    @try {
      [prefs setValue: @YES forKey: @"offlineApplicationCacheIsEnabled"];
    } @catch (NSException *error) {
      debug("Failed to set preference: 'offlineApplicationCacheIsEnabled': %@", error);
    }

    WKUserContentController* controller = config.userContentController;

    opts.clientId = this->bridge->id;

    this->bridge->preload = createPreload(opts, {
      .module = true,
      .wrap = true,
      .userScript = opts.userScript
    });

    webview = [SSCBridgedWebView.alloc
      initWithFrame: NSZeroRect
      configuration: config
             radius: (CGFloat) opts.radius
             margin: (CGFloat) opts.margin
    ];

    window.webview = webview;

    const auto processInfo = NSProcessInfo.processInfo;
    webview.customUserAgent = [NSString
      stringWithFormat: @("Mozilla/5.0 (Macintosh; Intel Mac OS X %d_%d_%d) AppleWebKit/605.1.15 (KHTML, like Gecko) SocketRuntime/%s"),
      processInfo.operatingSystemVersion.majorVersion,
      processInfo.operatingSystemVersion.minorVersion,
      processInfo.operatingSystemVersion.patchVersion,
      SSC::VERSION_STRING.c_str()
    ];

    webview.wantsLayer = YES;

    //
    // Initial setup of the window background color
    //

    NSAppearance *appearance = [NSAppearance currentAppearance];
    bool didSetBackgroundColor = false;

    if ([appearance bestMatchFromAppearancesWithNames:@[NSAppearanceNameDarkAqua]]) {
      if (opts.backgroundColorDark.size()) {
        this->setBackgroundColor(opts.backgroundColorDark);
        didSetBackgroundColor = true;
      }
    } else {
      if (opts.backgroundColorLight.size()) {
        this->setBackgroundColor(opts.backgroundColorLight);
        didSetBackgroundColor = true;
      }
    }

    if (!didSetBackgroundColor) {
      [window setBackgroundColor: [NSColor windowBackgroundColor]];
    }

    webview.layer.backgroundColor = [NSColor clearColor].CGColor;
    webview.layer.opaque = NO;
    [webview setValue: [NSNumber numberWithBool: NO] forKey: @"drawsBackground"];

    // [webview registerForDraggedTypes:
    //  [NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    //

    windowDelegate = [SSCWindowDelegate new];
    navigationDelegate = [SSCNavigationDelegate new];
    navigationDelegate.bridge = this->bridge;
    [controller addScriptMessageHandler: windowDelegate name: @"external"];

    // set delegates
    window.delegate = windowDelegate;
    webview.UIDelegate = webview;
    webview.navigationDelegate = navigationDelegate;

    if (!isDelegateSet) {
      isDelegateSet = true;

      class_replaceMethod(
        [SSCWindowDelegate class],
        @selector(windowShouldClose:),
        imp_implementationWithBlock(
          [&](id self, SEL cmd, id notification) {
            auto window = (Window*) objc_getAssociatedObject(self, "window");

            SSC::JSON::Object json = SSC::JSON::Object::Entries {
              {"data", window->index}
            };

            for (auto window : App::instance()->windowManager->windows) {
              if (window != nullptr) {
                window->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));
              }
            }

            if (!window || window->exiting) {
              return true;
            }

            if (window->opts.canExit) {
              window->exiting = true;
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
          [=, this](id self, SEL cmd, WKScriptMessage* scriptMessage) {
            auto window = (Window*) objc_getAssociatedObject(self, "window");

            if (!scriptMessage || !window) return;
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

            if (!window) {
              return;
            }

            id menuItem = (id) item;
            SSC::String title = [[menuItem title] UTF8String];
            SSC::String state = [menuItem state] == NSControlStateValueOn ? "true" : "false";
            SSC::String parent = [[[menuItem menu] title] UTF8String];
            SSC::String seq = std::to_string([menuItem tag]);
            SSC::String key = [[menuItem keyEquivalent] UTF8String];
            id representedObject = [menuItem representedObject];
            String type = "system";

            // "contextMenu" is parent menu identifier used in `setContextMenu()`
            if (parent == "contextMenu") {
              parent = "";
              type = "context";
            }

            if (representedObject != nullptr) {
              const auto parts = split([representedObject UTF8String], ':');
              if (parts.size() > 0) {
                type = parts[0];
              }

              if (parts.size() > 1) {
                parent = trim(parts[1]);
              }
            }

            window->eval(getResolveMenuSelectionJavaScript(seq, title, parent, type));
          }),
        "v@:@:@:"
      );
    }

    objc_setAssociatedObject(
      windowDelegate,
      "window",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    // Initialize application
    [NSApplication sharedApplication];

    if (userConfig["application_agent"] == "true") {
      [NSApp setActivationPolicy: NSApplicationActivationPolicyAccessory];

      if (this->opts.index == 0) {
        [window setBackgroundColor: [NSColor clearColor]];
        [window setAlphaValue: 0.0];
        [window setIgnoresMouseEvents: YES];
        [window setCanHide: NO];
        [window setOpaque: NO];
      }
    } else {
      [NSApp setActivationPolicy: NSApplicationActivationPolicyRegular];
    }

    if (opts.headless) {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      // Sets the app as the active app
      [NSApp activateIgnoringOtherApps: YES];
    }

    // Add webview to window
    [window setContentView: webview];

    navigate("0", opts.url);

    // Position window in center of screen
    [window center];
    [window setOpaque: YES];
    [window setTitleVisibility: NSWindowTitleVisible];

    if (opts.title.size() > 0) {
      this->setTitle(opts.title);
    }

    // Minimum window size
    [window setContentMinSize: NSMakeSize(opts.minWidth, opts.minHeight)];
    [window registerForDraggedTypes: draggableTypes];
    // [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];

    if (opts.frameless) {
      [window setTitlebarAppearsTransparent: YES];
      [window setMovableByWindowBackground: YES];
      style = NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskResizable;
      [window setStyleMask: style];
    }

    else if (opts.utility) {
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskUtilityWindow;
      [window setStyleMask: style];
    }

    //
    // results in a hidden title bar and a full-size content window.
    //
    if (opts.titlebarStyle == "hidden") {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskResizable;
      [window setStyleMask: style];
      [window setTitleVisibility: NSWindowTitleHidden];
    }

    //
    // results in a hidden titlebar with inset/offset window controls
    //
    else if (opts.titlebarStyle == "hiddenInset") {
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskTitled;
      style |= NSWindowStyleMaskResizable;

      [window setStyleMask: style];
      [window setTitleVisibility: NSWindowTitleHidden];

      CGFloat x = 16.f;
      CGFloat y = 42.f;

      if (opts.windowControlOffsets.size() > 0) {
        auto parts = split(opts.windowControlOffsets, 'x');
        try {
          x = std::stof(parts[0]);
          y = std::stof(parts[1]);
        } catch (...) {
          debug("invalid arguments for windowControlOffsets");
        }
      }

      NSView *titleBarView = [[NSView alloc] initWithFrame:NSZeroRect];

      [titleBarView setAutoresizingMask:NSViewWidthSizable | NSViewMinYMargin];
      [titleBarView setWantsLayer:YES];
      [titleBarView.layer setBackgroundColor:[NSColor clearColor].CGColor]; // Set background color to clear

      NSButton *closeButton = [window standardWindowButton: NSWindowCloseButton];
      NSButton *minimizeButton = [window standardWindowButton: NSWindowMiniaturizeButton];
      NSButton *zoomButton = [window standardWindowButton: NSWindowZoomButton];

      if (closeButton && minimizeButton && zoomButton) {
        [titleBarView addSubview:closeButton];
        [titleBarView addSubview:minimizeButton];
        [titleBarView addSubview:zoomButton];

        CGFloat viewWidth = window.frame.size.width;
        CGFloat viewHeight = y + MACOS_TRAFFIC_LIGHT_BUTTON_SIZE;
        CGFloat newX = x;
        CGFloat newY = 0.f;

        titleBarView.frame = NSMakeRect(newX, newY, viewWidth, viewHeight);

        window.windowControlOffsets = NSMakePoint(x, y);
        window.titleBarView = titleBarView;

        [window.contentView addSubview:titleBarView];
      } else {
        NSLog(@"Failed to retrieve standard window buttons.");
      }
    }

    if (opts.aspectRatio.size() > 2) {
      auto parts = split(opts.aspectRatio, ':');
      if (parts.size() != 2) return;
      CGFloat aspectRatio;

      @try {
        aspectRatio = std::stof(trim(parts[0])) / std::stof(trim(parts[1]));
      } @catch (NSException *error) {
        debug("Invalid aspect ratio: %@", error);
      }

      if (!std::isnan(aspectRatio)) {
        NSRect frame = [window frame];
        frame.size.height = frame.size.width / aspectRatio;
        [window setContentAspectRatio: frame.size];
      }
    }
  }

  Window::~Window () {
    this->close(0);
    delete this->bridge;
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
      [webview becomeFirstResponder];
      [window makeKeyAndOrderFront: nil];
      [NSApp activateIgnoringOtherApps: YES];
    }
  }

  void Window::exit (int code) {
    exiting = true;
    this->close(code);;
    if (onExit != nullptr) onExit(code);
  }

  void Window::kill () {
  }

  void Window::close (int code) {
    if (this->window != nullptr) {
      [this->window performClose: nil];
      auto app = App::instance();
      app->windowManager->destroyWindow(this->index);
      this->window = nullptr;
    }

    if (this->webview) {
      this->webview = nullptr;
    }

    if (this->windowDelegate != nullptr) {
      objc_removeAssociatedObjects(this->windowDelegate);
      this->windowDelegate = nullptr;
    }

    if (this->navigationDelegate != nullptr) {
      this->navigationDelegate = nullptr;
    }
  }

  void Window::maximize () {
    [this->window zoom: this->window];
  }

  void Window::minimize () {
    [this->window miniaturize: this->window];
  }

  void Window::restore () {
    [this->window deminiaturize: this->window];
  }

  void Window::hide () {
    if (this->window) {
      [this->window orderOut: this->window];
      this->eval(getEmitToRenderProcessJavaScript("windowHide", "{}"));
    }
  }

  void Window::eval (const String& source) {
    if (this->webview != nullptr) {
      [this->webview
        evaluateJavaScript: @(source.c_str())
         completionHandler: ^(id result, NSError *error)
      {
        if (error) {
          debug("JavaScriptError: %@", error);
        }
      }];
    }
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
    if (!this->window || !this->webview) return;
    NSMenu* menuBar = [NSApp mainMenu];
    NSArray* menuBarItems = [menuBar itemArray];
    if (menuBarItems.count == 0) return;

    NSMenu* menu = [menuBarItems[barPos] submenu];
    if (!menu) return;

    NSArray* menuItems = [menu itemArray];
    if (menuItems.count == 0) return;
    NSMenuItem* menuItem = menuItems[menuPos];

    if (!menuItem) return;

    [menuItem setTarget: nil];
    [menuItem setAction: NULL];
  }

  void Window::navigate (const SSC::String& seq, const SSC::String& value) {
    auto url = [NSURL URLWithString: [NSString stringWithUTF8String: value.c_str()]];

    if (url != nullptr && this->webview != nullptr) {
      if (String(url.scheme.UTF8String) == "file") {
        NSString* allowed = [[NSBundle mainBundle] resourcePath];
        [this->webview loadFileURL: url
          allowingReadAccessToURL: [NSURL fileURLWithPath: allowed]
        ];
      } else {
        auto request = [NSMutableURLRequest requestWithURL: url];
        [this->webview loadRequest: request];
      }

      if (seq.size() > 0) {
        auto index = std::to_string(this->opts.index);
        this->resolvePromise(seq, "0", index);
      }
    }
  }

  SSC::String Window::getTitle () {
    if (this->window) {
      return SSC::String([this->window.title UTF8String]);
    }

    return "";
  }

  void Window::setTitle (const SSC::String& value) {
    if (this->window) {
      auto title = [NSString stringWithUTF8String:value.c_str()];
      [this->window setTitle: title];
    }
  }

  ScreenSize Window::getSize () {
    if (this->window == nullptr) {
      return ScreenSize {0, 0};
    }

    NSRect e = this->window.frame;

    this->height = e.size.height;
    this->width = e.size.width;

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void Window::setSize (int width, int height, int hints) {
    if (this->window) {
      [this->window
        setFrame: NSMakeRect(0.f, 0.f, (float) width, (float) height)
         display: YES
         animate: NO
      ];

      [this->window center];

      this->height = height;
      this->width = width;
    }
  }

  void Window::closeContextMenu () {
    // @TODO(jwerle)
  }

  void Window::closeContextMenu (const SSC::String &seq) {
    // @TODO(jwerle)
  }

  void Window::showInspector () {
    if (this->webview) {
      // This is a private method on the webview, so we need to use
      // the pragma keyword to suppress the access warning.
      #pragma clang diagnostic ignored "-Wobjc-method-access"
      [[this->webview _inspector] show];
    }
  }

  void Window::setBackgroundColor (const String& rgbaString) {
    NSString *rgba = @(rgbaString.c_str());
    NSRegularExpression *regex =
      [NSRegularExpression regularExpressionWithPattern: @"rgba\\((\\d+),\\s*(\\d+),\\s*(\\d+),\\s*([\\d.]+)\\)"
                                                options: NSRegularExpressionCaseInsensitive
                                                  error: nil];

    NSTextCheckingResult *rgbaMatch =
      [regex firstMatchInString: rgba
                        options: 0
                          range: NSMakeRange(0, [rgba length])];

    if (rgbaMatch) {
      int r = [[rgba substringWithRange:[rgbaMatch rangeAtIndex:1]] intValue];
      int g = [[rgba substringWithRange:[rgbaMatch rangeAtIndex:2]] intValue];
      int b = [[rgba substringWithRange:[rgbaMatch rangeAtIndex:3]] intValue];
      float a = [[rgba substringWithRange:[rgbaMatch rangeAtIndex:4]] floatValue];

      this->setBackgroundColor(r, g, b, a);
    } else {
      debug("invalid arguments for window background color");
    }
  }

  void Window::setBackgroundColor (int r, int g, int b, float a) {
    if (this->window) {
      CGFloat sRGBComponents[4] = { r / 255.0, g / 255.0, b / 255.0, a };
      NSColorSpace *colorSpace = [NSColorSpace sRGBColorSpace];

      [this->window setBackgroundColor:
        [NSColor colorWithColorSpace: colorSpace
                          components: sRGBComponents
                               count: 4]
      ];
    }
  }

  String Window::getBackgroundColor () {
    if (!this->window) return std::string("");

    NSColor *backgroundColor = [this->window backgroundColor];
    NSColor *rgbColor = [backgroundColor colorUsingColorSpace:[NSColorSpace sRGBColorSpace]];

    CGFloat r, g, b, a;
    [rgbColor getRed:&r green:&g blue:&b alpha:&a];

    NSString *colorString = [NSString stringWithFormat: @"rgba(%.0f,%.0f,%.0f,%.1f)", r * 255, g * 255, b * 255, a];
    return [colorString UTF8String];
  }

  void Window::setContextMenu (const SSC::String& seq, const SSC::String& menuSource) {
    const auto mouseLocation = NSEvent.mouseLocation;
    const auto contextMenu = [[NSMenu.alloc initWithTitle: @"contextMenu"] autorelease];
    const auto location = NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y));
    const auto menuItems = split(menuSource, '\n');
    // remove the 'R' prefix as we'll use this value in the menu item "tag" property
    const auto id = std::stoi(seq.substr(1));

    // context menu item index
    int index = 0;

    for (const auto& item : menuItems) {
      if (trim(item).size() == 0) {
        continue;
      }

      if (item.find("---") != -1) {
        const auto separator  = NSMenuItem.separatorItem;
        [contextMenu addItem: separator];
        index++;
        continue;
      }

      const auto pair = split(trim(item), ':');
      const auto title = pair[0];

      // create context menu item with title at index offset
      auto menuItem = [contextMenu
        insertItemWithTitle: @(title.c_str())
                     action: @selector(menuItemSelected:)
              keyEquivalent: @("")
                    atIndex: index
      ];

      // use represented object as type indicator with extra
      // information to store in "parent" if available, otherwise just the
      // application menu category type ('context', 'system', or 'tray')
      if (pair.size() > 1) {
        menuItem.representedObject = @((String("context:") + trim(pair[1])).c_str());
      } else {
        menuItem.representedObject = @((String("context")).c_str());
      }

      // use menu item tag to store the promise resolution sequence index
      menuItem.tag = id;

      index++;
    }

    // actually show the context menu at the current mouse location with
    // the first context menu item as the initial item
    [contextMenu
      popUpMenuPositioningItem: contextMenu.itemArray[0]
                    atLocation: location
                        inView: nil
    ];
  }

  void Window::setTrayMenu (const SSC::String& seq, const SSC::String& value) {
    this->setMenu(seq, value, true);
  }

  void Window::setSystemMenu (const SSC::String& seq, const SSC::String& value) {
    this->setMenu(seq, value, false);
  }

  void Window::setMenu (const SSC::String& seq, const SSC::String& menuSource, const bool& isTrayMenu) {
    if (menuSource.empty()) return void(0);

    NSStatusItem *statusItem;
    NSString *title;
    NSString* nssTitle;
    NSMenu *menu;
    NSMenu *appleMenu;
    NSMenu *serviceMenu;
    NSMenu *windowMenu;
    NSMenu *editMenu;
    NSMenu *ctx;
    NSMenuItem *menuItem;

    menu = [[NSMenu alloc] init];
    // menu = [[NSMenu alloc] initWithTitle:@""];

    // id appName = [[NSProcessInfo processInfo] processName];
    // title = [@"About " stringByAppendingString:appName];

    auto menus = split(menuSource, ';');

    for (auto m : menus) {
      auto menuSource = split(m, '\n');
      if (menuSource.size() == 0) continue;

      auto i = -1;

      // find the first non-empty line
      std::string line = "";
      while (line.empty() && ++i < menuSource.size()) {
        line = trim(menuSource[i]);
      }

      if (i == menuSource.size()) {
        continue;
      }

      auto menuTitle = split(line, ':')[0];
      nssTitle = [NSString stringWithUTF8String: menuTitle.c_str()];
      ctx = isTrayMenu ? menu : [[NSMenu alloc] initWithTitle: nssTitle];
      bool isDisabled = false;

      if (isTrayMenu && menuSource.size() == 1) {
        auto menuParts = split(line, ':');
        auto action = @("menuItemSelected:");
        menuItem = [ctx
          addItemWithTitle: @(menuTitle.c_str())
          action: NSSelectorFromString(action)
          keyEquivalent: @""
        ];

        if (menuParts.size() > 1) {
          [menuItem setRepresentedObject: @((String("tray:") + trim(menuParts[1])).c_str())];
        } else {
          [menuItem setRepresentedObject: @("tray")];
        }
      }

      while (++i < menuSource.size()) {
        line = trim(menuSource[i]);
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
            bool isShift = String("ABCDEFGHIJKLMNOPQRSTUVWXYZ").find(key) != -1;

            for (int i = 1; i < accelerator.size(); i++) {
              auto modifier = trim(accelerator[i]);
              // normalize modifier to lower case
              std::transform(
                modifier.begin(),
                modifier.end(),
                modifier.begin(),
                [](auto ch) { return std::tolower(ch); }
              );

              if (isShift || modifier.compare("shift") == 0) {
                mask |= NSEventModifierFlagShift;
              }

              if (
                modifier.compare("command") == 0 ||
                modifier.compare("super") == 0 ||
                modifier.compare("meta") == 0
              ) {
                mask |= NSEventModifierFlagCommand;
              } else if (modifier.compare("commandorcontrol")) {
                mask |= NSEventModifierFlagCommand;
                mask |= NSEventModifierFlagControl;
              } else if (modifier.compare("control") == 0) {
                mask |= NSEventModifierFlagControl;
              } else if (modifier.compare("alt") == 0 || modifier.compare("option") == 0) {
                mask |= NSEventModifierFlagOption;
              }
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
          if (title.compare("Undo") == 0) nssSelector = [NSString stringWithUTF8String:"undo:"];
          if (title.compare("Redo") == 0) nssSelector = [NSString stringWithUTF8String:"redo:"];
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
          nssSelector = [NSString stringWithUTF8String:"terminate:"];
        }

        if (title.compare("Minimize") == 0) {
          nssSelector = [NSString stringWithUTF8String:"performMiniaturize:"];
        }

        if (title.compare("Maximize") == 0) {
          nssSelector = [NSString stringWithUTF8String:"performZoom:"];
        }

        if (title.find("---") != -1) {
          [ctx addItem: [NSMenuItem separatorItem]];
        } else {
          menuItem = [ctx
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

      if (!isTrayMenu) {
        // create a top level menu item
        menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent:@""];
        [menu addItem: menuItem];
        // set its submenu
        [menuItem setSubmenu: ctx];
        [menuItem release];
        [ctx release];
      }
    }

    if (isTrayMenu) {
      [menu setTitle: nssTitle];
      [menu setDelegate: (id)this->app.delegate]; // bring the main window to the front when clicked 
      [menu setAutoenablesItems: NO];

      auto userConfig = SSC::getUserConfig();
      auto bundlePath = [[[NSBundle mainBundle] resourcePath] UTF8String];
      auto cwd = SSC::fs::path(bundlePath);
      auto trayIconPath = String("application_tray_icon");

      if (SSC::fs::exists(SSC::fs::path(cwd) / (trayIconPath + ".png"))) {
        trayIconPath = (SSC::fs::path(cwd) / (trayIconPath + ".png")).string();
      } else if (SSC::fs::exists(SSC::fs::path(cwd) / (trayIconPath + ".jpg"))) {
        trayIconPath = (SSC::fs::path(cwd) / (trayIconPath + ".jpg")).string();
      } else if (SSC::fs::exists(SSC::fs::path(cwd) / (trayIconPath + ".jpeg"))) {
        trayIconPath = (SSC::fs::path(cwd) / (trayIconPath + ".jpeg")).string();
      } else if (SSC::fs::exists(SSC::fs::path(cwd) / (trayIconPath + ".ico"))) {
        trayIconPath = (SSC::fs::path(cwd) / (trayIconPath + ".ico")).string();
      } else {
        trayIconPath = "";
      }

      NSString *imagePath = [NSString stringWithUTF8String: trayIconPath.c_str()];
      NSImage *image = [[NSImage alloc] initWithContentsOfFile: imagePath];
      NSSize newSize = NSMakeSize(32, 32);
      NSImage *resizedImage = [[NSImage alloc] initWithSize:newSize];
      [resizedImage lockFocus];
      [image drawInRect:NSMakeRect(0, 0, newSize.width, newSize.height) fromRect:NSZeroRect operation:NSCompositingOperationCopy fraction:1.0];
      [resizedImage unlockFocus];

      if (image) this->app.delegate.statusItem.button.image = resizedImage;

      [this->app.delegate.statusItem.button setToolTip: nssTitle];
      [this->app.delegate.statusItem setMenu: menu];
      [this->app.delegate.statusItem retain];
    } else {
      [NSApp setMainMenu: menu];
    }

    if (seq.size() > 0) {
      auto index = std::to_string(this->opts.index);
      this->resolvePromise(seq, "0", index);
    }
  }
}
#endif
