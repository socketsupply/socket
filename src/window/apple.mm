#include "window.hh"
#include "../ipc/ipc.hh"

using namespace SSC;

@implementation SSCWindow
#if SSC_PLATFORM_MACOS
  - (void) layoutIfNeeded {
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

  - (void) sendEvent: (NSEvent*) event {
    if (event.type == NSEventTypeLeftMouseDown || event.type == NSEventTypeLeftMouseDragged) {
      if (event.type == NSEventTypeLeftMouseDown) {
        [self.webview mouseDown: event];
      }

      if (event.type == NSEventTypeLeftMouseDragged) {
        [self.webview mouseDragged: event];
        return;
      }
    }

    [super sendEvent:event];
  }
#endif
@end

@implementation SSCWindowDelegate
#if SSC_PLATFORM_MACOS
- (void) userContentController: (WKUserContentController*) userContentController
       didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
{
  auto window = (Window*) objc_getAssociatedObject(self, "window");
  if (!scriptMessage || !window) {
    return;
  }

  id body = scriptMessage.body;
  if (!body || ![body isKindOfClass: NSString.class]) {
    return;
  }

  const auto uri = String([body UTF8String]);
  if (uri.size() > 0) {
    if (!window->bridge->route(uri, nullptr, 0)) {
      window->onMessage(uri);
    }
  }
}

- (void) menuItemSelected: (NSMenuItem*) menuItem {
  auto window = (Window*) objc_getAssociatedObject(self, "window");

  if (!window) {
    return;
  }

  const auto title = String(menuItem.title.UTF8String);
  const auto state = String(menuItem.state == NSControlStateValueOn ? "true" : "false");
  const auto seq = std::to_string(menuItem.tag);
  const auto key = String(menuItem.keyEquivalent.UTF8String);
  auto parent = String(menuItem.menu.title.UTF8String);
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
}

- (BOOL) windowShouldClose: (SSCWindow*) _ {
  auto window = (Window*) objc_getAssociatedObject(self, "window");

  if (!window || !window->exiting) {
    return true;
  }

  const JSON::Object json = JSON::Object::Entries {
    {"data", window->index}
  };

  for (auto window : App::instance()->windowManager->windows) {
    if (window != nullptr) {
      window->eval(getEmitToRenderProcessJavaScript("window-closed", json.str()));
    }
  }

#if !__has_feature(objc_arc)
  if (window->window) {
    [window->window release];
  }
#endif

  window->window = nullptr;
  if (window->opts.canExit) {
    window->exiting = true;
    window->exit(0);
    return true;
  }

  window->eval(getEmitToRenderProcessJavaScript("window-hide", ""));
  window->hide();
  return true;
}
#endif
@end

@implementation SSCBridgedWebView
#if !SSC_PLATFORM_IOS && !SSC_PLATFORM_IOS_SIMULATOR

Vector<String> draggablePayload;
CGFloat MACOS_TRAFFIC_LIGHT_BUTTON_SIZE = 16;
int lastX = 0;
int lastY = 0;

- (void) viewDidAppear: (BOOL) animated {
  debug("viewDidAppear");
}

- (void) viewDidDisappear: (BOOL) animated {
  debug("viewDidDisappear");
}

- (void) viewDidChangeEffectiveAppearance {
  [super viewDidChangeEffectiveAppearance];

  if (@available(macOS 10.14, *)) {
    if ([self.window.effectiveAppearance.name containsString:@"Dark"]) {
      [self.window setBackgroundColor: [NSColor colorWithCalibratedWhite: 0.1 alpha: 1.0]]; // Dark mode color
    } else {
      [self.window setBackgroundColor: [NSColor colorWithCalibratedWhite: 1.0 alpha: 1.0]]; // Light mode color
    }
  }
}

- (void) resizeSubviewsWithOldSize: (NSSize) oldSize {
  [super resizeSubviewsWithOldSize: oldSize];

  const auto w = reinterpret_cast<SSCWindow*>(self.window);
  const auto viewWidth = w.titleBarView.frame.size.width;
  const auto viewHeight = w.titleBarView.frame.size.height;
  const auto newX = w.windowControlOffsets.x;
  const auto newY = 0.f;

  const auto closeButton = [w standardWindowButton: NSWindowCloseButton];
  const auto minimizeButton = [w standardWindowButton: NSWindowMiniaturizeButton];
  const auto zoomButton = [w standardWindowButton: NSWindowZoomButton];

  if (closeButton && minimizeButton && zoomButton) {
    [w.titleBarView addSubview: closeButton];
    [w.titleBarView addSubview: minimizeButton];
    [w.titleBarView addSubview: zoomButton];
  }

  w.titleBarView.frame = NSMakeRect(newX, newY, viewWidth, viewHeight);
}

- (instancetype) initWithFrame: (NSRect) frameRect
                 configuration: (WKWebViewConfiguration*) configuration
                        radius: (CGFloat) radius
                        margin: (CGFloat) margin
{
  self = [super initWithFrame: frameRect configuration: configuration];

  if (self && radius > 0.0) {
    self.radius = radius;
    self.margin = margin;
    self.layer.cornerRadius = radius;
    self.layer.masksToBounds = YES;
  }

  return self;
}

- (void) layout {
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
  const auto json = JSON::Object {};
  const auto payload = getEmitToRenderProcessJavaScript("dragenter", json.str());
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

  const auto data = JSON::Object::Entries {
    {"count", (unsigned int) count},
    {"inbound", inbound},
    {"x", x},
    {"y", y}
  };

  const auto json = JSON::Object {data};
  const auto payload = getEmitToRenderProcessJavaScript("drag", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
  return [super draggingUpdated: info];
}

- (void) draggingExited: (id<NSDraggingInfo>) info {
  const auto position = info.draggingLocation;
  const auto x = std::to_string(position.x);
  const auto y = std::to_string(self.frame.size.height - position.y);

  const auto data = JSON::Object::Entries {
    {"x", x},
    {"y", y}
  };

  const auto json = JSON::Object {data};
  const auto payload = getEmitToRenderProcessJavaScript("dragend", json.str());

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

  auto files = JSON::Array::Entries {};

  for (NSURL* file in pasteboardFiles) {
    files.push_back(file.path.UTF8String);
  }

  const auto data = JSON::Object::Entries {
    {"files", files},
    {"x", x},
    {"y", y}
  };

  const auto json = JSON::Object { data };
  const auto payload = getEmitToRenderProcessJavaScript("dropin", json.str());

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

  const auto data = JSON::Object::Entries {
    {"count", (unsigned int) count},
    {"x", x},
    {"y", y}
  };

  const auto json = JSON::Object { data };
  const auto payload = getEmitToRenderProcessJavaScript("drag", json.str());

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
      const auto data = JSON::Object::Entries {
        {"src", path},
        {"x", x},
        {"y", y}
      };

      const auto json = JSON::Object { data };
      const auto payload = getEmitToRenderProcessJavaScript("drop", json.str());

      [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];
    }
  }

  const auto data = JSON::Object::Entries {
    {"x", x},
    {"y", y}
  };

  const auto json = JSON::Object { data };
  auto payload = getEmitToRenderProcessJavaScript("dragend", json.str());

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

  String js(
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

    const auto match = String([result UTF8String]);

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
    auto payload = getEmitToRenderProcessJavaScript("dragexit", "{}");
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
    const auto data = JSON::Object::Entries {
      {"count", (unsigned int) draggablePayload.size()},
      {"x", x},
      {"y", y}
    };

    const auto json = JSON::Object { data };
    const auto payload = getEmitToRenderProcessJavaScript("drag", json.str());

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
  const auto dest = String(url.path.UTF8String);
  const auto src = String([filePromiseProvider.userInfo UTF8String]);
  const auto data = [@"" dataUsingEncoding: NSUTF8StringEncoding];

  [data writeToURL: url atomically: YES];

  const auto json = JSON::Object {
    JSON::Object::Entries {
      {"src", src},
      {"dest", dest}
    }
  };

  const auto payload = getEmitToRenderProcessJavaScript("dropout", json.str());

  [self evaluateJavaScript: @(payload.c_str()) completionHandler: nil];

  completionHandler(nil);
}

- (NSString*) filePromiseProvider: (NSFilePromiseProvider*) filePromiseProvider
                  fileNameForType: (NSString*) fileType
{
  const auto id = rand64();
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
  StringStream contentTypesSpec;

  for (NSString* acceptedMIMEType in acceptedMIMETypes) {
    contentTypesSpec << acceptedMIMEType.UTF8String << "|";
  }

  if (acceptedFileExtensions.count > 0) {
    contentTypesSpec << "*/*:";
    const auto count = acceptedFileExtensions.count;
    int seen = 0;
    for (NSString* acceptedFileExtension in acceptedFileExtensions) {
      const auto string = String(acceptedFileExtension.UTF8String);

      if (!string.starts_with(".")) {
        contentTypesSpec << ".";
      }

      contentTypesSpec << string;
      if (++seen < count) {
        contentTypesSpec << ",";
      }
    }
  }

  auto contentTypes = trim(contentTypesSpec.str());

  if (contentTypes.size() == 0) {
    contentTypes = "*/*";
  }

  if (contentTypes.ends_with("|")) {
    contentTypes = contentTypes.substr(0, contentTypes.size() - 1);
  }

  const auto options = Dialog::FileSystemPickerOptions {
    .directories = false,
    .multiple = parameters.allowsMultipleSelection ? true : false,
    .contentTypes = contentTypes,
    .defaultName = "",
    .defaultPath = "",
    .title = "Choose a File"
  };

  Dialog dialog;
  const auto results = dialog.showOpenFilePicker(options);

  if (results.size() == 0) {
    completionHandler(nullptr);
    return;
  }

  auto urls = [NSMutableArray array];

  for (const auto& result : results) {
    [urls addObject: [NSURL URLWithString: @(result.c_str())]];
  }

  completionHandler(urls);
}
#endif

#if (!SSC_PLATFORM_IOS && !SSC_PLATFORM_IOS_SIMULATOR) || (SSC_PLATFORM_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)
-                                      (void) webView: (WKWebView*) webView
 requestDeviceOrientationAndMotionPermissionForOrigin: (WKSecurityOrigin*) origin
                                     initiatedByFrame: (WKFrameInfo*) frame
                                      decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler {
  static auto userConfig = getUserConfig();

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
  static auto userConfig = getUserConfig();

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
  static auto userConfig = getUserConfig();
  auto title = userConfig["meta_title"] + ":";

  if (userConfig.contains("window_alert_title")) {
    title = userConfig["window_alert_title"];
  }

#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS
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
  static auto userConfig = getUserConfig();
  auto title = userConfig["meta_title"] + ":";

  if (userConfig.contains("window_alert_title")) {
    title = userConfig["window_alert_title"];
  }
#if SSC_PLATFORM_IOS || SSC_PLATFORM_IOS
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

#if !SSC_PLATFORM_IOS && !SSC_PLATFORM_IOS_SIMULATOR
namespace SSC {
  Window::Window (App& app, WindowOptions opts)
    : app(app),
      opts(opts),
      hotkey(this)
  {
    const auto processInfo = NSProcessInfo.processInfo;
    auto configuration = [[WKWebViewConfiguration new] autorelease];
    auto preferences = configuration.preferences;
    auto userConfig = opts.userConfig;

    this->index = opts.index;
    this->bridge = new IPC::Bridge(app.core, opts.userConfig);
    this->processPool = [WKProcessPool new];
    this->windowDelegate = [SSCWindowDelegate new];

    objc_setAssociatedObject(
      this->windowDelegate,
      "window",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    // inherit from `IPC::Bridge` if `clientId` was not given (default case)
    if (opts.clientId == 0) {
      opts.clientId = this->bridge->id;
    }

    this->bridge->router.dispatchFunction = [this] (auto callback) {
      this->app.dispatch(callback);
    };

    this->bridge->router.evaluateJavaScriptFunction = [this](auto source) {
      dispatch_async(dispatch_get_main_queue(), ^{
        this->eval(source);
      });
    };

    this->bridge->preload = createPreload(opts, {
      .module = true,
      .wrap = true,
      .userScript = opts.userScript
    });

    this->bridge->configureHandlers({
      .webview = configuration
    });

    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    configuration.defaultWebpagePreferences.allowsContentJavaScript = YES;
    configuration.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    configuration.limitsNavigationsToAppBoundDomains = YES;
    configuration.websiteDataStore = WKWebsiteDataStore.defaultDataStore;
    configuration.processPool = this->processPool;

    this->hotkey.init(this->bridge);

    [configuration.websiteDataStore.httpCookieStore
        setCookiePolicy: WKCookiePolicyAllow
      completionHandler: ^(){}
    ];

    [configuration.userContentController
      addScriptMessageHandler: this->windowDelegate
                         name: @"external"
    ];

    [configuration
      setValue: @NO
        forKey: @"crossOriginAccessControlCheckEnabled"
    ];

    preferences.javaScriptCanOpenWindowsAutomatically = YES;

    @try {
      if (userConfig["permissions_allow_fullscreen"] == "false") {
        [preferences setValue: @NO forKey: @"fullScreenEnabled"];
        [preferences setValue: @NO forKey: @"elementFullscreenEnabled"];
      } else {
        [preferences setValue: @YES forKey: @"fullScreenEnabled"];
        [preferences setValue: @YES forKey: @"elementFullscreenEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'fullScreenEnabled': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_fullscreen"] == "false") {
        [preferences setValue: @NO forKey: @"elementFullscreenEnabled"];
      } else {
        [preferences setValue: @YES forKey: @"elementFullscreenEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'elementFullscreenEnabled': %@", error);
    }

    if (opts.debug || isDebugEnabled()) {
      [preferences setValue:@YES forKey:@"developerExtrasEnabled"];
      if (@available(macOS 13.3, iOS 16.4, tvOS 16.4, *)) {
        [webview setInspectable: YES];
      }
    }

    @try {
      if (userConfig["permissions_allow_clipboard"] == "false") {
        [preferences setValue: @NO forKey: @"javaScriptCanAccessClipboard"];
      } else {
        [preferences setValue: @YES forKey: @"javaScriptCanAccessClipboard"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'javaScriptCanAccessClipboard': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_data_access"] == "false") {
        [preferences setValue: @NO forKey: @"storageAPIEnabled"];
      } else {
        [preferences setValue: @YES forKey: @"storageAPIEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'storageAPIEnabled': %@", error);
    }

    @try {
      if (userConfig["permissions_allow_device_orientation"] == "false") {
        [preferences setValue: @NO forKey: @"deviceOrientationEventEnabled"];
      } else {
        [preferences setValue: @YES forKey: @"deviceOrientationEventEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'deviceOrientationEventEnabled': %@", error);
    }

    if (userConfig["permissions_allow_notifications"] == "false") {
      @try {
        [preferences setValue: @NO forKey: @"appBadgeEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'deviceOrientationEventEnabled': %@", error);
      }

      @try {
        [preferences setValue: @NO forKey: @"notificationsEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationsEnabled': %@", error);
      }

      @try {
        [preferences setValue: @NO forKey: @"notificationEventEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationEventEnabled': %@", error);
      }
    } else {
      @try {
        [preferences setValue: @YES forKey: @"appBadgeEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'appBadgeEnabled': %@", error);
      }
    }

  #if !SSC_PLATFORM_IOS && !SSC_PLATFORM_IOS_SIMULATOR
    @try {
      [preferences setValue: @YES forKey: @"cookieEnabled"];

      if (userConfig["permissions_allow_user_media"] == "false") {
        [preferences setValue: @NO forKey: @"mediaStreamEnabled"];
      } else {
        [preferences setValue: @YES forKey: @"mediaStreamEnabled"];
      }
    } @catch (NSException *error) {
      debug("Failed to set preference: 'mediaStreamEnabled': %@", error);
    }
  #endif

    @try {
      if (userConfig["permissions_allow_airplay"] == "false") {
        configuration.allowsAirPlayForMediaPlayback = NO;
      } else {
        configuration.allowsAirPlayForMediaPlayback = YES;
      }
    } @catch (NSException *error) {
      debug("Failed to set preference 'allowsAirPlayForMediaPlayback': %@", error);
    }

    @try {
      [preferences setValue: @YES forKey: @"offlineApplicationCacheIsEnabled"];
    } @catch (NSException *error) {
      debug("Failed to set preference: 'offlineApplicationCacheIsEnabled': %@", error);
    }

    this->webview = [SSCBridgedWebView.alloc
      initWithFrame: NSZeroRect
      configuration: configuration
             radius: (CGFloat) opts.radius
             margin: (CGFloat) opts.margin
    ];

    this->webview.customUserAgent = [NSString
      stringWithFormat: @("Mozilla/5.0 (Macintosh; Intel Mac OS X %d_%d_%d) AppleWebKit/605.1.15 (KHTML, like Gecko) SocketRuntime/%s"),
        processInfo.operatingSystemVersion.majorVersion,
        processInfo.operatingSystemVersion.minorVersion,
        processInfo.operatingSystemVersion.patchVersion,
        VERSION_STRING.c_str()
    ];

    this->webview.wantsLayer = YES;
    this->webview.UIDelegate = webview;
    this->webview.navigationDelegate = this->bridge->navigator.navigationDelegate;
    this->webview.layer.backgroundColor = NSColor.clearColor.CGColor;
    this->webview.layer.opaque = NO;
    [this->webview setValue: @(0) forKey: @"drawsBackground"];

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

    this->window = [SSCWindow.alloc
        initWithContentRect: NSMakeRect(0, 0, opts.width, opts.height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO
    ];

    this->window.window = this;
    this->window.opaque = YES;
    this->window.webview = this->webview;
    this->window.delegate = this->windowDelegate;
    // this->window.appearance = [NSAppearance appearanceNamed: NSAppearanceNameVibrantDark];
    this->window.contentMinSize = NSMakeSize(opts.minWidth, opts.minHeight);
    this->window.titleVisibility = NSWindowTitleVisible;
    this->window.titlebarAppearsTransparent = true;
    // this->window.movableByWindowBackground = true;

    if (opts.maximizable == false) {
      this->window.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary;
    }

    if (opts.frameless) {
      this->window.titlebarAppearsTransparent = YES;
      this->window.movableByWindowBackground = YES;
      style = NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskResizable;
      this->window.styleMask = style;
    } else if (opts.utility) {
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskUtilityWindow;
      this->window.styleMask = style;
    }

    if (opts.titlebarStyle == "hidden") {
      // hidden title bar and a full-size content window.
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskResizable;
      this->window.styleMask = style;
      this->window.titleVisibility = NSWindowTitleHidden;
    } else if (opts.titlebarStyle == "hiddenInset") {
      // hidden titlebar with inset/offset window controls
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskTitled;
      style |= NSWindowStyleMaskResizable;

      this->window.styleMask = style;
      this->window.titleVisibility = NSWindowTitleHidden;

      auto x = 16.f;
      auto y = 42.f;

      if (opts.windowControlOffsets.size() > 0) {
        auto parts = split(opts.windowControlOffsets, 'x');
        try {
          x = std::stof(parts[0]);
          y = std::stof(parts[1]);
        } catch (...) {
          debug("invalid arguments for windowControlOffsets");
        }
      }

      auto titleBarView = [NSView.alloc initWithFrame: NSZeroRect];

      titleBarView.layer.backgroundColor = NSColor.clearColor.CGColor; // Set background color to clear
      titleBarView.autoresizingMask = NSViewWidthSizable | NSViewMinYMargin;
      titleBarView.wantsLayer = YES;

      const auto closeButton = [this->window standardWindowButton: NSWindowCloseButton];
      const auto minimizeButton = [this->window standardWindowButton: NSWindowMiniaturizeButton];
      const auto zoomButton = [this->window standardWindowButton: NSWindowZoomButton];

      if (closeButton && minimizeButton && zoomButton) {
        [titleBarView addSubview: closeButton];
        [titleBarView addSubview: minimizeButton];
        [titleBarView addSubview: zoomButton];

        const auto viewWidth = window.frame.size.width;
        const auto viewHeight = y + MACOS_TRAFFIC_LIGHT_BUTTON_SIZE;
        const auto newX = x;
        const auto newY = 0.f;

        titleBarView.frame = NSMakeRect(newX, newY, viewWidth, viewHeight);

        this->window.windowControlOffsets = NSMakePoint(x, y);
        this->window.titleBarView = titleBarView;

        [this->window.contentView addSubview: titleBarView];
      } else {
        NSLog(@"Failed to retrieve standard window buttons.");
      }
    }

    if (opts.aspectRatio.size() > 2) {
      auto parts = split(opts.aspectRatio, ':');
      if (parts.size() == 2) {
        CGFloat aspectRatio;

        @try {
          aspectRatio = std::stof(trim(parts[0])) / std::stof(trim(parts[1]));
        } @catch (NSException *error) {
          debug("Invalid aspect ratio: %@", error);
        }

          auto frame = this->window.frame;
        if (!std::isnan(aspectRatio)) {
          frame.size.height = frame.size.width / aspectRatio;
          this->window.contentAspectRatio = frame.size;
        }
      }
    }

    const auto appearance = NSAppearance.currentAppearance;
    bool didSetBackgroundColor = false;

    if ([appearance bestMatchFromAppearancesWithNames: @[NSAppearanceNameDarkAqua]]) {
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
      this->window.backgroundColor = NSColor.windowBackgroundColor;
    }

    // Add webview to window
    this->window.contentView = this->webview;

    // Position window in center of screen
    [this->window center];

    [this->window registerForDraggedTypes: [NSArray arrayWithObjects:
      NSPasteboardTypeURL,
      NSPasteboardTypeFileURL,
      (NSString*) kPasteboardTypeFileURLPromise,
      NSPasteboardTypeString,
      NSPasteboardTypeHTML,
      nil
		]];

    if (opts.index == 0) {
      if (opts.headless || userConfig["application_agent"] == "true") {
        NSApp.activationPolicy = NSApplicationActivationPolicyAccessory;

        if (userConfig["application_agent"] == "true") {
          this->window.backgroundColor = NSColor.clearColor;
          this->window.alphaValue = 0.0;
          this->window.ignoresMouseEvents = YES;
          this->window.canHide = NO;
          this->window.opaque = NO;
        }
      } else {
        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
      }

      if (opts.headless) {
        [NSApp activateIgnoringOtherApps: NO];
      } else {
        // Sets the app as the active app
        [NSApp activateIgnoringOtherApps: YES];
      }
    }

    this->navigate("0", opts.url);

    if (opts.title.size() > 0) {
      this->setTitle(opts.title);
    }
  }

  Window::~Window () {
    delete this->bridge;

  #if !__has_feature(objc_arc)
    if (this->processPool) {
      [this->processPool release];
    }

    if (this->webview) {
      [this->webview release];
    }

    if (this->windowDelegate) {
      [this->windowDelegate release];
    }

    if (this->window) {
      [this->window release];
    }
  #endif

    this->bridge = nullptr;
    this->window = nullptr;
    this->webview = nullptr;
    this->processPool = nullptr;
    this->windowDelegate = nullptr;
  }

  ScreenSize Window::getScreenSize () {
    NSRect e = [[NSScreen mainScreen] frame];

    return ScreenSize {
      .height = (int) e.size.height,
      .width = (int) e.size.width
    };
  }

  void Window::show () {
    if (this->opts.index == 0) {
      if (opts.headless || this->bridge->userConfig["application_agent"] == "true") {
        NSApp.activationPolicy = NSApplicationActivationPolicyAccessory;
      } else {
        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
      }
    }

    if (this->opts.headless || this->bridge->userConfig["application_agent"] == "true") {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      [this->webview becomeFirstResponder];
      [this->window makeKeyAndOrderFront: nil];
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
    App::instance()->dispatch([this]() {
      if (this->windowDelegate != nullptr) {
        objc_removeAssociatedObjects(this->windowDelegate);
     }

      if (this->webview != nullptr) {
        debug("stopLoading");
        [this->webview stopLoading];
        debug("removeScriptMessageHandlerForName");
        [this->webview.configuration.userContentController removeAllScriptMessageHandlers];
        [this->webview removeFromSuperview];
        this->webview.navigationDelegate = nullptr;
        this->webview.UIDelegate = nullptr;
      }

      if (this->window != nullptr) {
        auto contentView = this->window.contentView;
        auto subviews = NSMutableArray.array;
        for (NSView* view in contentView.subviews) {
          if (view == this->webview) {
            this->webview = nullptr;
          }
          [view removeFromSuperview];
          [view release];
        }

        [this->window performClose: nil];
        this->window = nullptr;
        this->window.window = nullptr;
        this->window.webview = nullptr;
        this->window.delegate = nullptr;
        this->window.contentView = nullptr;
      #if SSC_PLATFORM_MACOS
        if (this->window.titleBarView) {
          [this->window.titleBarView removeFromSuperview];
        #if !__has_feature(objc_arc)
          [this->window.titleBarView release];
        #endif
        }
        this->window.titleBarView = nullptr;
      #endif
      }
    });
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
      this->eval(getEmitToRenderProcessJavaScript("window-hide", "{}"));
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

  void Window::navigate (const String& seq, const String& value) {
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

  const String Window::getTitle () const {
    if (this->window && this->window.title.UTF8String != nullptr) {
      return this->window.title.UTF8String;
    }

    return "";
  }

  void Window::setTitle (const String& title) {
    if (this->window) {
      [this->window setTitle: @(title.c_str())];
    }
  }

  ScreenSize Window::getSize () {
    if (this->window == nullptr) {
      return ScreenSize {0, 0};
    }

    const auto frame = this->window.frame;

    this->height = frame.size.height;
    this->width = frame.size.width;

    return ScreenSize {
      .height = (int) frame.size.height,
      .width = (int) frame.size.width
    };
  }

  const ScreenSize Window::getSize () const {
    if (this->window == nullptr) {
      return ScreenSize {0, 0};
    }

    const auto frame = this->window.frame;

    return ScreenSize {
      .height = (int) frame.size.height,
      .width = (int) frame.size.width
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

  void Window::closeContextMenu (const String &seq) {
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

  void Window::setContextMenu (const String& seq, const String& menuSource) {
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

  void Window::setTrayMenu (const String& seq, const String& value) {
    this->setMenu(seq, value, true);
  }

  void Window::setSystemMenu (const String& seq, const String& value) {
    this->setMenu(seq, value, false);
  }

  void Window::setMenu (const String& seq, const String& menuSource, const bool& isTrayMenu) {
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
        String key = "";

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

      auto userConfig = getUserConfig();
      auto bundlePath = [[[NSBundle mainBundle] resourcePath] UTF8String];
      auto cwd = fs::path(bundlePath);
      auto trayIconPath = String("application_tray_icon");

      if (fs::exists(fs::path(cwd) / (trayIconPath + ".png"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".png")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".jpeg"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".jpeg")).string();
      } else if (fs::exists(fs::path(cwd) / (trayIconPath + ".ico"))) {
        trayIconPath = (fs::path(cwd) / (trayIconPath + ".ico")).string();
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
