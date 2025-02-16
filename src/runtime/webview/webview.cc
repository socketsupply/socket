#include "../app.hh"
#include "../crypto.hh"
#include "../string.hh"
#include "../window.hh"

#include "../webview.hh"

using namespace ssc::runtime;
using ssc::runtime::javascript::getEmitToRenderProcessJavaScript;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::window::Dialog;
using ssc::runtime::window::Window;
using ssc::runtime::crypto::rand64;
using ssc::runtime::string::trim;
using ssc::runtime::app::App;

#if SOCKET_RUNTIME_PLATFORM_APPLE
#if SOCKET_RUNTIME_PLATFORM_MACOS
@interface WKOpenPanelParameters (WKPrivate)
- (NSArray<NSString*>*) _acceptedMIMETypes;
- (NSArray<NSString*>*) _acceptedFileExtensions;
- (NSArray<NSString*>*) _allowedFileExtensions;
@end
#endif

@implementation SSCWebView
#if SOCKET_RUNTIME_PLATFORM_MACOS
Vector<String> draggablePayload;
int lastX = 0;
int lastY = 0;

- (void) viewDidAppear: (BOOL) animated {
}

- (void) viewDidDisappear: (BOOL) animated {
}

- (void) viewDidChangeEffectiveAppearance {
  [super viewDidChangeEffectiveAppearance];
  const auto window = (Window*) objc_getAssociatedObject(self, "window");

  if ([window->window.effectiveAppearance.name containsString: @"Dark"]) {
    [window->window setBackgroundColor: [NSColor colorWithCalibratedWhite: 0.1 alpha: 1.0]]; // Dark mode color
  } else {
    [window->window setBackgroundColor: [NSColor colorWithCalibratedWhite: 1.0 alpha: 1.0]]; // Light mode color
  }
}

- (void) resizeSubviewsWithOldSize: (NSSize) oldSize {
  [super resizeSubviewsWithOldSize: oldSize];

  auto window = (Window*) objc_getAssociatedObject(self, "window");
  const auto w = reinterpret_cast<SSCWindow*>(window->window);
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
  self.wantsLayer = YES;
  self.UIDelegate = self;
  self.layer.backgroundColor = NSColor.clearColor.CGColor;
  self.layer.opaque = NO;

  if (radius > 0.0) {
    self.radius = radius;
    self.margin = margin;
    self.layer.cornerRadius = radius;
    self.layer.masksToBounds = YES;
  }

  return self;
}

- (void) layout {
  [super layout];
  auto window = (Window*) objc_getAssociatedObject(self, "window");

#if SOCKET_RUNTIME_PLATFORM_MACOS
  self.translatesAutoresizingMaskIntoConstraints = YES;
  self.autoresizesSubviews = YES;
  self.autoresizingMask = (
    NSViewHeightSizable |
    NSViewWidthSizable |
    NSViewMaxXMargin |
    NSViewMinYMargin
  );
#endif

  NSRect bounds = self.superview.bounds;

  if (self.radius > 0.0) {
    if (self.contentHeight == 0.0) {
      self.contentHeight = self.superview.bounds.size.height - self.bounds.size.height;
    }

    bounds.size.height = bounds.size.height - self.contentHeight;
  }

  if (self.margin > 0.0) {
    CGFloat borderWidth = self.margin;
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    [window->window
      setFrame: NSInsetRect(bounds, borderWidth, borderWidth)
       display: YES
       animate: NO
    ];
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    window->window.frame = NSInsetRect(bounds, borderWidth, borderWidth);
  #endif
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

  const auto point = [self convertPoint: event.locationInWindow fromView: nil];
  const auto x = std::to_string(point.x);
  const auto y = std::to_string(point.y);

  self.initialWindowPos = point;

  lastX = (int) point.x;
  lastY = (int) point.y;

  String js(
    "(() => {                                                                  "
    "  const v = '--app-region';                                               "
    "  let el = document.elementFromPoint(" + x + "," + y + ");                "
    "                                                                          "
    "  while (el) {                                                            "
    "    if (getComputedStyle(el).getPropertyValue(v) === 'drag') {            "
    "      return 'movable';                                                   "
    "    }                                                                     "
    "    el = el.parentElement;                                                "
    "  }                                                                       "
    "  return ''                                                               "
    "})();                                                                     "
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
  const auto window = (Window*) objc_getAssociatedObject(self, "window");

  if (self.shouldDrag) {
    CGFloat deltaX = currentLocation.x - self.initialWindowPos.x;
    CGFloat deltaY = currentLocation.y - self.initialWindowPos.y;

    NSRect frame = window->window.frame;
    frame.origin.x += deltaX;
    frame.origin.y -= deltaY;

    [window->window setFrame:frame display:YES];
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
  const auto window = (Window*) objc_getAssociatedObject(self, "window");
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

  Dialog dialog(window);
  const auto success = dialog.showOpenFilePicker(options, [=](const auto results) {
    if (results.size() == 0) {
      completionHandler(nullptr);
      return;
    }

    auto urls = [NSMutableArray array];

    for (const auto& result : results) {
      [urls addObject: [NSURL URLWithString: @(result.c_str())]];
    }

    completionHandler(urls);
  });

  if (!success) {
    completionHandler(nullptr);
    return;
  }
}
#elif SOCKET_RUNTIME_PLATFORM_IOS
- (instancetype) initWithFrame: (CGRect) frame
                 configuration: (WKWebViewConfiguration*) configuration
     withRefreshControlEnabled: (BOOL) refreshControlEnabled
{
  self = [super initWithFrame: frame configuration: configuration];
  self.UIDelegate = self;
  self.allowsBackForwardNavigationGestures = NO;
  self.translatesAutoresizingMaskIntoConstraints = NO;
  self.autoresizingMask = (
    UIViewAutoresizingFlexibleWidth |
    UIViewAutoresizingFlexibleHeight
  );

  self.layer.opaque = NO;

  self.scrollView.backgroundColor = UIColor.blackColor;
  self.scrollView.alwaysBounceVertical = NO;
  self.scrollView.bounces = NO;
  self.scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;
  self.scrollView.scrollEnabled = YES;

  if (refreshControlEnabled) {
    String title = "Pull to Refresh";
    self.refreshControl = [UIRefreshControl new];
    self.refreshControl.tintColor = UIColor.grayColor;
    self.refreshControl.attributedTitle = [NSAttributedString.alloc
      initWithString: @(title.c_str())
      attributes: @{
        NSForegroundColorAttributeName: UIColor.grayColor
      }
    ];

    [self.refreshControl layoutIfNeeded];
    [self.refreshControl
             addTarget: self
                action: @selector(handlePullToRefresh:)
      forControlEvents: UIControlEventValueChanged
    ];

    if (@available(iOS 17.4, *)) {
      self.scrollView.bouncesVertically = YES;
    }

    self.scrollView.alwaysBounceVertical = YES;
    self.scrollView.bounces = YES;
    self.scrollView.userInteractionEnabled = YES;

    self.scrollView.refreshControl =  self.refreshControl;
  }

  return self;
}

- (void) handlePullToRefresh: (UIRefreshControl*) refreshControl {
  [self reload];
}
#endif

#if (!SOCKET_RUNTIME_PLATFORM_IOS_SIMULATOR) || (SOCKET_RUNTIME_PLATFORM_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)
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

-                     (void) webView: (SSCWebView*) webview
  runJavaScriptAlertPanelWithMessage: (NSString*) message
                    initiatedByFrame: (WKFrameInfo*) frame
                   completionHandler: (void (^)(void)) completionHandler
{
  static auto userConfig = getUserConfig();
  const auto title = userConfig.contains("window_alert_title")
    ? userConfig["window_alert_title"]
    : userConfig["meta_title"] + ":";

#if SOCKET_RUNTIME_PLATFORM_IOS
  const auto alert = [UIAlertController
    alertControllerWithTitle: @(title.c_str())
                     message: message
              preferredStyle: UIAlertControllerStyleAlert
  ];

  const auto ok = [UIAlertAction
    actionWithTitle: @"OK"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler();
  }];

  [alert addAction: ok];

  [webview.window.rootViewController
    presentViewController: alert
                 animated: YES
               completion: nil
  ];
#else
  const auto alert = [NSAlert new];
  [alert setMessageText: @(title.c_str())];
  [alert setInformativeText: message];
  [alert addButtonWithTitle: @"OK"];
  [alert runModal];
  completionHandler();
#if !__has_feature(objc_arc)
  [alert release];
#endif
#endif
}

#if SOCKET_RUNTIME_PLATFORM_IOS
- (void) traitCollectionDidChange: (UITraitCollection*) previousTraitCollection {
  [super traitCollectionDidChange:previousTraitCollection];

  static auto userConfig = getUserConfig();
  const auto window = (Window*) objc_getAssociatedObject(self, "window");

  UIUserInterfaceStyle interfaceStyle = window->window.traitCollection.userInterfaceStyle;

  auto hasBackgroundDark = userConfig.count("window_background_color_dark") > 0;
  auto hasBackgroundLight = userConfig.count("window_background_color_light") > 0;

  if (interfaceStyle == UIUserInterfaceStyleDark && hasBackgroundDark) {
    window->setBackgroundColor(userConfig["window_background_color_dark"]);
  } else if (hasBackgroundLight) {
    window->setBackgroundColor(userConfig["window_background_color_light"]);
  }
}
#endif

-                       (void) webView: (WKWebView*) webview
  runJavaScriptConfirmPanelWithMessage: (NSString*) message
                      initiatedByFrame: (WKFrameInfo*) frame
                     completionHandler: (void (^)(BOOL result)) completionHandler
{
  static auto userConfig = getUserConfig();
  const auto title = userConfig.contains("window_alert_title")
    ? userConfig["window_alert_title"]
    : userConfig["meta_title"] + ":";

#if SOCKET_RUNTIME_PLATFORM_IOS
  const auto alert = [UIAlertController
    alertControllerWithTitle: @(title.c_str())
                     message: message
              preferredStyle: UIAlertControllerStyleAlert
  ];

  const auto ok = [UIAlertAction
    actionWithTitle: @"OK"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler(YES);
  }];

  const auto cancel = [UIAlertAction
    actionWithTitle: @"Cancel"
              style: UIAlertActionStyleDefault
            handler: ^(UIAlertAction * action) {
    completionHandler(NO);
  }];

  [alert addAction: ok];
  [alert addAction: cancel];

  [webview.window.rootViewController
    presentViewController: alert
                 animated: YES
               completion: nil
  ];
#else
  const auto alert = [NSAlert new];
  [alert setMessageText: @(title.c_str())];
  [alert setInformativeText: message];
  [alert addButtonWithTitle: @"OK"];
  [alert addButtonWithTitle: @"Cancel"];
  completionHandler([alert runModal] == NSAlertFirstButtonReturn);
#if !__has_feature(objc_arc)
  [alert release];
#endif
#endif
}
@end

#if SOCKET_RUNTIME_PLATFORM_IOS
@implementation SSCWebViewController
-                         (BOOL) gestureRecognizer: (UIGestureRecognizer*) gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer: (UIGestureRecognizer*) otherGestureRecognizer
{
  return YES;
}

- (BOOL) gestureRecognizer: (UIGestureRecognizer*) gestureRecognizer
        shouldReceiveTouch: (UITouch*) touch
{
  if ([touch.view isDescendantOfView: self.webview]) {
    return NO;
  }

  return YES;
}

- (void) viewSafeAreaInsetsDidChange {
  [super viewSafeAreaInsetsDidChange];
  const auto topInset = self.view.safeAreaInsets.top;
  auto contentInset = self.webview.scrollView.contentInset;
  auto offset = self.webview.scrollView.contentOffset;
  contentInset.top = topInset;
  contentInset.bottom -= topInset;
  offset.y = -topInset;
  self.webview.scrollView.contentInset = contentInset;
  self.webview.scrollView.contentOffset = offset;
}

- (void) viewDidLayoutSubviews {
  [super viewDidLayoutSubviews];
  const auto topInset = self.view.safeAreaInsets.top;
  auto contentInset = self.webview.scrollView.contentInset;
  auto offset = self.webview.scrollView.contentOffset;
  contentInset.top = topInset;
  contentInset.bottom -= topInset;
  offset.y = -topInset;
  self.webview.scrollView.contentInset = contentInset;
  self.webview.scrollView.contentOffset = offset;
}

- (void) viewWillDisappear: (BOOL) animated {
  [super viewWillDisappear: animated];
  const auto window = (Window*) objc_getAssociatedObject(self.webview, "window");
  auto app = App::sharedApplication();
  if (window) {
    app->runtime.windowManager.destroyWindow(window->index);
  }
}
@end
#endif
#endif
