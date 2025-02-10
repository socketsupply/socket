#include "../../cli.hh"
#include "../filesystem.hh"
#include "../javascript.hh"
#include "../runtime.hh"
#include "../webview.hh"
#include "../version.hh"
#include "../config.hh"
#include "../string.hh"
#include "../cwd.hh"
#include "../env.hh"
#include "../app.hh"

#include "../window.hh"

using ssc::runtime::javascript::getResolveMenuSelectionJavaScript;
using ssc::runtime::javascript::getEmitToRenderProcessJavaScript;

using ssc::runtime::config::isDebugEnabled;

using ssc::runtime::string::split;
using ssc::runtime::string::trim;

using namespace ssc::runtime;

#if SOCKET_RUNTIME_PLATFORM_IOS
static dispatch_queue_attr_t qos = dispatch_queue_attr_make_with_qos_class(
  DISPATCH_QUEUE_CONCURRENT,
  QOS_CLASS_USER_INITIATED,
  -1
);

static dispatch_queue_t queue = dispatch_queue_create(
  "socket.runtime.app.queue",
  qos
);
#endif

@implementation SSCWindow
#if SOCKET_RUNTIME_PLATFORM_MACOS
CGFloat MACOS_TRAFFIC_LIGHT_BUTTON_SIZE = 16;
  - (void) layoutIfNeeded {
    [super layoutIfNeeded];

    if (self.titleBarView && self.titleBarView.subviews.count == 0) {
      const auto minimizeButton = [self standardWindowButton: NSWindowMiniaturizeButton];
      const auto closeButton = [self standardWindowButton: NSWindowCloseButton];
      const auto zoomButton = [self standardWindowButton: NSWindowZoomButton];

      if (closeButton && minimizeButton && zoomButton) {
        [self.titleBarView addSubview: closeButton];
        [self.titleBarView addSubview: minimizeButton];
        [self.titleBarView addSubview: zoomButton];
      }
    }
  }

  - (void) sendEvent: (NSEvent*) event {
    [super sendEvent: event];
  }
#endif
@end

@implementation SSCWindowDelegate
  - (void) userContentController: (WKUserContentController*) userContentController
         didReceiveScriptMessage: (WKScriptMessage*) scriptMessage
  {
    const auto window = (Window*) objc_getAssociatedObject(self, "window");
    if (!window || !scriptMessage || !scriptMessage.body) {
      return;
    }

    if (![scriptMessage.body isKindOfClass: NSString.class]) {
      return;
    }

    const auto string = (NSString*) scriptMessage.body;
    const auto uri = String(string.UTF8String);

  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto msg = ipc::Message(uri);
    if (msg.name == "application.exit" || msg.name == "process.exit") {
      const auto code = std::stoi(msg.get("value", "0"));

      if (code > 0) {
        ssc::cli::notify(SIGTERM);
      } else {
        ssc::cli::notify(SIGUSR2);
      }
    }
  #endif

    if (uri.size() > 0) {
      if (!window->bridge->route(uri, nullptr, 0)) {
        if (window->onMessage != nullptr) {
          window->onMessage(uri);
        }
      }
    }
  }

  - (SSCWebView*) webView: (SSCWebView*) webview
  createWebViewWithConfiguration: (WKWebViewConfiguration*) configuration
             forNavigationAction: (WKNavigationAction*) navigationAction
                  windowFeatures: (WKWindowFeatures*) windowFeatures
{
  // TODO(@jwerle): handle 'window.open()'
  return nullptr;
}

#if SOCKET_RUNTIME_PLATFORM_MACOS
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
  auto app = App::sharedApplication();

  if (!app || !window || window->isExiting) {
    return true;
  }

  auto index = window->index;
  const JSON::Object json = JSON::Object::Entries {
    {"data", window->index}
  };

  for (auto window : app->runtime.windowManager.windows) {
    if (window != nullptr && window->index != index) {
      window->eval(getEmitToRenderProcessJavaScript("windowclosed", json.str()));
    }
  }

  app->runtime.windowManager.destroyWindow(index);
  return true;
}
#elif SOCKET_RUNTIME_PLATFORM_IOS
- (void) scrollViewDidScroll: (UIScrollView*) scrollView {
  auto window = (Window*) objc_getAssociatedObject(self, "window");
  if (window) {
    scrollView.bounds = window->webview.bounds;
  }
}

- (void) handleEdgePanGesture: (UIScreenEdgePanGestureRecognizer*) gesture {
  auto window = (Window*) objc_getAssociatedObject(self, "window");
  if (!window) {
    return;
  }
  CGPoint translation = [gesture translationInView: window->viewController.view];
  CGFloat progress = translation.x / window->viewController.view.bounds.size.width;
  progress = fmin(fmax(progress, 0.0), 1.0);
  if (
    gesture.state == UIGestureRecognizerStateEnded ||
    gesture.state == UIGestureRecognizerStateCancelled
  ) {
    if (progress > 0.3) {
      if (window && window->webview.canGoBack) {
        [window->webview goBack];
      } else {
        const auto app = App::sharedApplication();
        const auto index = window->index;
        app->dispatch([index, app](){
          app->runtime.windowManager.destroyWindow(index);
        });
      }
    }
  }
}
#endif
@end

namespace ssc::runtime::window {
  Window::Window (SharedPointer<bridge::Bridge> bridge, const Window::Options& options)
    : options(options),
      bridge(bridge),
      hotkey(this),
      dialog(this)
  {
  #if SOCKET_RUNTIME_PLATFORM_IOS
    const auto frame = UIScreen.mainScreen.bounds;
  #endif
    const auto processInfo = NSProcessInfo.processInfo;
    const auto configuration = [WKWebViewConfiguration new];
    const auto preferences = configuration.preferences;
    auto userConfig = options.userConfig;

    this->index = options.index;
    this->windowDelegate = [SSCWindowDelegate new];

    this->bridge->navigateHandler = [this] (const auto url) {
      this->navigate(url);
    };

    this->bridge->evaluateJavaScriptHandler = [this](auto source) {
      this->eval(source);
    };

    this->bridge->client.preload = webview::Preload::compile({
      .client = this->bridge->client,
      .index = options.index,
      .userScript = options.userScript,
      .userConfig = options.userConfig,
      .conduit = {
        {"port", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.port},
        {"hostname", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.hostname},
        {"sharedKey", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.sharedKey}
      }
    });

    configuration.defaultWebpagePreferences.allowsContentJavaScript = YES;

    #if SOCKET_RUNTIME_PLATFORM_IOS
      configuration.allowsInlineMediaPlayback = YES;
    #endif

    configuration.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    // https://webkit.org/blog/10882/app-bound-domains/
    // https://developer.apple.com/documentation/webkit/wkwebviewconfiguration/3585117-limitsnavigationstoappbounddomai
    configuration.limitsNavigationsToAppBoundDomains = YES;
    configuration.websiteDataStore = WKWebsiteDataStore.defaultDataStore;

    if (@available(macOS 14.0, iOS 17.0, *)) {
      [configuration.websiteDataStore.httpCookieStore
          setCookiePolicy: WKCookiePolicyAllow
        completionHandler: ^(){}
      ];
    }

    [configuration.userContentController
      addScriptMessageHandler: this->windowDelegate
                         name: @"external"
    ];

    auto preloadUserScript= [WKUserScript alloc];
    auto preloadUserScriptSource = webview::Preload::compile({
      .features = webview::Preload::Options::Features {
        .useGlobalCommonJS = false,
        .useGlobalNodeJS = false,
        .useTestScript = false,
        .useHTMLMarkup = false,
        .useESM = false,
        .useGlobalArgs = true
      },
      .client = this->bridge->client,
      .index = options.index,
      .userScript = options.userScript,
      .userConfig = options.userConfig,
      .conduit = {
        {"port", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.port},
        {"hostname", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.hostname},
        {"sharedKey", static_cast<runtime::Runtime&>(this->bridge->context).services.conduit.sharedKey}
      }
    });

    [preloadUserScript
        initWithSource: @(preloadUserScriptSource.str().c_str())
         injectionTime: WKUserScriptInjectionTimeAtDocumentStart
      forMainFrameOnly: NO
    ];

    [configuration.userContentController
      addUserScript: preloadUserScript
    ];

    [configuration
      setValue: @NO
        forKey: @"crossOriginAccessControlCheckEnabled"
    ];

    preferences.javaScriptCanOpenWindowsAutomatically = YES;
    preferences.elementFullscreenEnabled = userConfig["permissions_allow_fullscreen"] != "false";

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
        [preferences setValue: @NO forKey: @"notificationsEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationsEnabled': %@", error);
      }

      @try {
        [preferences setValue: @NO forKey: @"notificationEventEnabled"];
      } @catch (NSException *error) {
        debug("Failed to set preference: 'notificationEventEnabled': %@", error);
      }
    }

  #if SOCKET_RUNTIME_PLATFORM_MACOS
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

    if (options.debug || isDebugEnabled()) {
      [preferences setValue: @YES forKey: @"developerExtrasEnabled"];
    }

    this->bridge->init();
    this->hotkey.init();
    this->bridge->configureNavigatorMounts();
    this->bridge->configureSchemeHandlers({
      .webview = configuration
    });

  #if SOCKET_RUNTIME_PLATFORM_MACOS
    this->webview = [SSCWebView.alloc
      initWithFrame: NSZeroRect
      configuration: configuration
             radius: (CGFloat) options.radius
             margin: (CGFloat) options.margin
    ];

    this->webview.wantsLayer = YES;
    this->webview.layer.backgroundColor = NSColor.clearColor.CGColor;
    this->webview.customUserAgent = [NSString
      stringWithFormat: @("Mozilla/5.0 (Macintosh; Intel Mac OS X %d_%d_%d) AppleWebKit/605.1.15 (KHTML, like Gecko) SocketRuntime/%s"),
        processInfo.operatingSystemVersion.majorVersion,
        processInfo.operatingSystemVersion.minorVersion,
        processInfo.operatingSystemVersion.patchVersion,
        ssc::runtime::version::VERSION_STRING.c_str()
    ];
    [this->webview setValue: @(0) forKey: @"drawsBackground"];
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    this->webview = [SSCWebView.alloc
      initWithFrame: frame
      configuration: configuration
    ];

    this->webview.scrollView.delegate = this->windowDelegate;
    this->webview.scrollView.contentInsetAdjustmentBehavior = UIScrollViewContentInsetAdjustmentNever;

    this->webview.autoresizingMask = (
      UIViewAutoresizingFlexibleWidth |
      UIViewAutoresizingFlexibleHeight
    );

    this->webview.customUserAgent = [NSString
      stringWithFormat: @("Mozilla/5.0 (iPhone; CPU iPhone OS %d_%d_%d like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/17.2 Mobile/15E148 Safari/604.1 SocketRuntime/%s"),
      processInfo.operatingSystemVersion.majorVersion,
      processInfo.operatingSystemVersion.minorVersion,
      processInfo.operatingSystemVersion.patchVersion,
      ssc::runtime::VERSION_STRING.c_str()
    ];
  #endif

    /*
    this->webview.allowsBackForwardNavigationGestures = (
      userConfig["webview_navigator_enable_navigation_gestures"] == "true"
    );
    */

  #if SOCKET_RUNTIME_PLATFORM_IOS
    this->webview.allowsBackForwardNavigationGestures = NO;
    if (!this->options.headless && userConfig["webview_navigator_enable_navigation_gestures"] == "true") {
      auto edgePanGesture = [[UIScreenEdgePanGestureRecognizer alloc]
        initWithTarget: this->windowDelegate
                action: @selector(handleEdgePanGesture:)
      ];
      edgePanGesture.edges = UIRectEdgeLeft;
      edgePanGesture.delegate = this->viewController;
      [this->viewController.view addGestureRecognizer: edgePanGesture];
      [this->webview addGestureRecognizer: edgePanGesture];
    }
  #else
    this->webview.allowsBackForwardNavigationGestures = userConfig["webview_navigator_enable_navigation_gestures"] == "true";
  #endif

    this->webview.UIDelegate = webview;
    this->webview.layer.opaque = NO;

    objc_setAssociatedObject(
      this->webview,
      "window",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    objc_setAssociatedObject(
      this->windowDelegate,
      "window",
      (id) this,
      OBJC_ASSOCIATION_ASSIGN
    );

    if (options.debug || isDebugEnabled()) {
      if (@available(macOS 13.3, iOS 16.4, tvOS 16.4, *)) {
        this->webview.inspectable = YES;
      }
    }

    if (options.width > 0 && options.height > 0) {
      this->setSize(options.width, options.height);
    } else if (options.width > 0) {
      this->setSize(options.width, window.frame.size.height);
    } else if (options.height > 0) {
      this->setSize(window.frame.size.width, options.height);
    }

    this->bridge->configureWebView(this->webview);
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    // Window style: titled, closable, minimizable
    uint style = NSWindowStyleMaskTitled | NSWindowStyleMaskFullSizeContentView;

    // Set window to be resizable
    if (options.resizable) {
      style |= NSWindowStyleMaskResizable;
    }

    if (options.closable) {
      style |= NSWindowStyleMaskClosable;
    }

    if (options.minimizable) {
      style |= NSWindowStyleMaskMiniaturizable;
    }

    this->window = [SSCWindow.alloc
        initWithContentRect: NSMakeRect(0, 0, options.width, options.height)
                  styleMask: style
                    backing: NSBackingStoreBuffered
                      defer: NO
    ];
    // this->window.appearance = [NSAppearance appearanceNamed: NSAppearanceNameVibrantDark];
    this->window.contentMinSize = NSMakeSize(options.minWidth, options.minHeight);
    this->window.titleVisibility = NSWindowTitleVisible;
    this->window.titlebarAppearsTransparent = true;
    // this->window.movableByWindowBackground = true;
    this->window.delegate = this->windowDelegate;
    this->window.opaque = YES;
    this->window.webview = this->webview;

    if (options.maximizable == false) {
      this->window.collectionBehavior = NSWindowCollectionBehaviorFullScreenAuxiliary;
    }

    // Add webview to window
    this->window.contentView = this->webview;

    if (options.frameless) {
      this->window.titlebarAppearsTransparent = YES;
      this->window.movableByWindowBackground = YES;
      style = NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskResizable;
      this->window.styleMask = style;
    } else if (options.utility) {
      style |= NSWindowStyleMaskBorderless;
      style |= NSWindowStyleMaskUtilityWindow;
      this->window.styleMask = style;
    }

    if (options.titlebarStyle == "hidden") {
      // hidden title bar and a full-size content window.
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskResizable;
      this->window.styleMask = style;
      this->window.titleVisibility = NSWindowTitleHidden;
    } else if (options.titlebarStyle == "hiddenInset") {
      // hidden titlebar with inset/offset window controls
      style |= NSWindowStyleMaskFullSizeContentView;
      style |= NSWindowStyleMaskTitled;
      style |= NSWindowStyleMaskResizable;

      this->window.styleMask = style;
      this->window.titleVisibility = NSWindowTitleHidden;

      auto x = 16.f;
      auto y = 42.f;

      if (options.windowControlOffsets.size() > 0) {
        auto parts = split(options.windowControlOffsets, 'x');
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

        [this->webview addSubview: titleBarView];
      } else {
        NSLog(@"Failed to retrieve standard window buttons.");
      }
    }

    if (options.aspectRatio.size() > 2) {
      const auto parts = split(options.aspectRatio, ':');
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
      if (options.backgroundColorDark.size()) {
        this->setBackgroundColor(options.backgroundColorDark);
        didSetBackgroundColor = true;
      }
    } else {
      if (options.backgroundColorLight.size()) {
        this->setBackgroundColor(options.backgroundColorLight);
        didSetBackgroundColor = true;
      }
    }

    if (!didSetBackgroundColor) {
      this->window.backgroundColor = NSColor.windowBackgroundColor;
    }

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

    if (options.index == 0) {
      if (options.headless || userConfig["application_agent"] == "true") {
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

      if (options.headless) {
        [NSApp activateIgnoringOtherApps: NO];
      } else {
        // Sets the app as the active app
        [NSApp activateIgnoringOtherApps: YES];
      }
    }
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    this->window = [SSCWindow.alloc initWithFrame: frame];
    this->viewController = [SSCWebViewController new];
    this->viewController.webview = this->webview;

    UIUserInterfaceStyle interfaceStyle = this->window.traitCollection.userInterfaceStyle;

    auto hasBackgroundDark = userConfig.count("window_background_color_dark") > 0;
    auto hasBackgroundLight = userConfig.count("window_background_color_light") > 0;

    if (interfaceStyle == UIUserInterfaceStyleDark && hasBackgroundDark) {
      this->setBackgroundColor(userConfig["window_background_color_dark"]);
    } else if (hasBackgroundLight) {
      this->setBackgroundColor(userConfig["window_background_color_light"]);
    } else {
      this->viewController.webview.backgroundColor = [UIColor systemBackgroundColor];
      this->window.backgroundColor = [UIColor systemBackgroundColor];
      this->viewController.webview.opaque = NO;
    }

    [this->viewController.view addSubview: this->webview];

    this->window.rootViewController = this->viewController;
    this->window.rootViewController.view.frame = frame;
  #endif

    this->position.x = this->window.frame.origin.x;
    this->position.y = this->window.frame.origin.y;
  }

  Window::~Window () {
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

    this->window = nullptr;
    this->webview = nullptr;
    this->processPool = nullptr;
    this->windowDelegate = nullptr;
  }

  ScreenSize Window::getScreenSize () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    const auto frame = NSScreen.mainScreen.frame;
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    const auto frame = UIScreen.mainScreen.bounds;
  #endif
    return ScreenSize {
      .width = (int) frame.size.width,
      .height = (int) frame.size.height
    };
  }

  void Window::show () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    if (this->options.index == 0) {
      if (options.headless || this->bridge->userConfig["application_agent"] == "true") {
        NSApp.activationPolicy = NSApplicationActivationPolicyAccessory;
      } else {
        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
      }
    }

    if (this->options.headless || this->bridge->userConfig["application_agent"] == "true") {
      [NSApp activateIgnoringOtherApps: NO];
    } else {
      [this->webview becomeFirstResponder];
      [this->window makeKeyAndOrderFront: nil];
      [NSApp activateIgnoringOtherApps: YES];
    }

    this->position.x = this->window.frame.origin.x;
    this->position.y = this->window.frame.origin.y;
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    [this->webview becomeFirstResponder];
    [this->window makeKeyAndVisible];
    this->window.hidden = NO;
    const auto app = App::sharedApplication();
    for (auto window : app->runtime.windowManager.windows) {
      if (window != nullptr && reinterpret_cast<Window*>(window.get()) != this) {
        window->hide();
      }
    }
  #endif
  }

  void Window::exit (int code) {
    isExiting = true;
    const auto callback = this->onExit;
    this->onExit = nullptr;
    if (callback != nullptr) {
      callback(code);
    }
  }

  void Window::kill () {}

  void Window::close (int code) {
    const auto app = App::sharedApplication();
    if (this->windowDelegate != nullptr) {
      objc_removeAssociatedObjects(this->windowDelegate);
    }

    if (this->webview != nullptr) {
    #if SOCKET_RUNTIME_PLATFORM_IOS
      const auto managedWindow = app->runtime.windowManager.getWindow(this->index);
      if (managedWindow) {
        const auto json = managedWindow->json();
        for (auto window : app->runtime.windowManager.windows) {
          if (window != nullptr && window->index != this->index) {
            window->eval(getEmitToRenderProcessJavaScript("windowclosed", json.str()));
          }
        }
      }
    #endif
      objc_removeAssociatedObjects(this->webview);
      [this->webview stopLoading];
      [this->webview.configuration.userContentController removeAllScriptMessageHandlers];
      [this->webview removeFromSuperview];
      this->webview.navigationDelegate = nullptr;
      this->webview.UIDelegate = nullptr;
      this->webview = nullptr;
    }

    if (this->window != nullptr) {
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      auto contentView = this->window.contentView;
      auto subviews = NSMutableArray.array;

      for (NSView* view in contentView.subviews) {
        if (view == this->webview) {
          this->webview = nullptr;
        }
        [view removeFromSuperview];
        [view release];
      }

      [this->window performClose: nullptr];
      this->window.webview = nullptr;
      this->window.delegate = nullptr;
      this->window.contentView = nullptr;

      if (this->window.titleBarView) {
        [this->window.titleBarView removeFromSuperview];
      #if !__has_feature(objc_arc)
        [this->window.titleBarView release];
      #endif
      }

      this->window.titleBarView = nullptr;
    #endif
      this->window = nullptr;
    }

   #if SOCKET_RUNTIME_PLATFORM_IOS
    this->viewController = nullptr;
   #endif
  }

  void Window::maximize () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    [this->window zoom: this->window];
  #endif
  }

  void Window::minimize () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    [this->window miniaturize: this->window];
  #endif
  }

  void Window::restore () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    [this->window deminiaturize: this->window];
  #endif
  }

  void Window::hide () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    if (this->window) {
      [this->window orderOut: this->window];
    }
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    if (this->window) {
      this->window.hidden = YES;
    }
  #endif
    JSON::Object json = JSON::Object::Entries {
      {"data", index}
    };
    this->eval(getEmitToRenderProcessJavaScript("windowhidden", json.str()));
  }

  void Window::eval (const String& source, const EvalCallback callback) {
    App::sharedApplication()->dispatch([=, this]() {
      if (this->webview != nullptr) {
        [this->webview
          evaluateJavaScript: @(source.c_str())
           completionHandler: ^(id result, NSError *error)
        {
          if (error) {
            debug("JavaScriptError: %@", error);

            if (callback != nullptr) {
              callback(JSON::Error(error.localizedDescription.UTF8String));
            }

            return;
          }

          if (callback != nullptr) {
            if ([result isKindOfClass: NSString.class]) {
              const auto value = String([result UTF8String]);
              if (value == "null" || value == "undefined") {
                callback(JSON::Null());
              } else if (value == "true") {
                callback(JSON::Boolean(true));
              } else if (value == "false") {
                callback(JSON::Boolean(false));
              } else {
                double number = 0.0f;

                try {
                  number = std::stod(value);
                } catch (...) {
                  callback(value);
                  return;
                }

                callback(number);
              }
            } else if ([result isKindOfClass: NSNumber.class]) {
              callback([result doubleValue]);
            }
          }
        }];
      }
    });
  }

  void Window::setSystemMenuItemEnabled (bool enabled, int barPos, int menuPos) {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
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
  #endif
  }

  void Window::navigate (const String& value) {
    App::sharedApplication()->dispatch([=, this]() {
      const auto url = [NSURL URLWithString: @(value.c_str())];

      if (url != nullptr && this->webview != nullptr) {
        if (String(url.scheme.UTF8String) == "file") {
          static const auto resourcesPath = filesystem::Resource::getResourcesPath();
          [this->webview loadFileURL: url
            allowingReadAccessToURL: [NSURL fileURLWithPath: @(resourcesPath.string().c_str())]
          ];
        } else {
          auto request = [NSMutableURLRequest requestWithURL: url];
          [this->webview loadRequest: request];
        }
      }
    });
  }

  const String Window::getTitle () const {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    if (this->window && this->window.title.UTF8String != nullptr) {
      return this->window.title.UTF8String;
    }
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    if (this->viewController && this->viewController.title.UTF8String != nullptr) {
      return this->viewController.title.UTF8String;
    }
  #endif

    return "";
  }

  void Window::setTitle (const String& title) {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    if (this->window) {
      this->window.title =  @(title.c_str());
    }
  #elif SOCKET_RUNTIME_PLATFORM_IOS
    if (this->viewController) {
      this->viewController.title = @(title.c_str());
    }
  #endif
  }

  Window::Size Window::getSize () {
    if (this->window == nullptr) {
      return Size {0, 0};
    }

    const auto frame = this->window.frame;

    this->size.height = frame.size.height;
    this->size.width = frame.size.width;

    return Size {
      .width = (int) frame.size.width,
      .height = (int) frame.size.height
    };
  }

  const Window::Size Window::getSize () const {
    if (this->window == nullptr) {
      return Size {0, 0};
    }

    const auto frame = this->window.frame;

    return Size {
      .width = (int) frame.size.width,
      .height = (int) frame.size.height
    };
  }

  void Window::setSize (int width, int height, int hints) {
    if (this->window) {
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      [this->window
        setFrame: NSMakeRect(0.f, 0.f, (float) width, (float) height)
         display: YES
         animate: NO
      ];

      [this->window center];
    #elif SOCKET_RUNTIME_PLATFORM_IOS
      auto frame = this->window.frame;
      frame.size.width = width;
      frame.size.height = height;
      this->window.frame = frame;
    #endif
    }

    this->size.height = height;
    this->size.width = width;
  }

  void Window::setPosition (float x, float y) {
    if (this->window) {
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      const auto point = NSPointFromCGPoint(CGPointMake(x, y));
      this->window.frameTopLeftPoint = point;
    #elif SOCKET_RUNTIME_PLATFORM_IOS
      auto frame = this->window.frame;
      frame.origin.x = x;
      frame.origin.y = y;
      this->window.frame = frame;
    #endif
    }

    this->position.x = x;
    this->position.y = y;
  }

  void Window::closeContextMenu () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    // TODO(@jwerle)
  #endif
  }

  void Window::closeContextMenu (const String &instanceId) {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    // TODO(@jwerle)
  #endif
  }

  void Window::showInspector () {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    if (this->webview) {
      // This is a private method on the webview, so we need to use
      // the pragma keyword to suppress the access warning.
      #pragma clang diagnostic ignored "-Wobjc-method-access"
      [[this->webview _inspector] show];
    }
  #endif
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
      CGFloat rgba[4] = { r / 255.0, g / 255.0, b / 255.0, a };
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      [this->window setBackgroundColor: [NSColor
        colorWithColorSpace: NSColorSpace.sRGBColorSpace
                 components: rgba
                      count: 4
      ]];
    #elif SOCKET_RUNTIME_PLATFORM_IOS
      auto color = [UIColor
        colorWithRed: rgba[0]
               green: rgba[1]
                blue: rgba[2]
               alpha: rgba[3]
      ];

      [this->window setBackgroundColor: color];
      [this->webview setBackgroundColor: color];
    #endif
    }
  }

  String Window::getBackgroundColor () {
    if (this->window) {
      const auto backgroundColor = this->window.backgroundColor;
      CGFloat r, g, b, a;
    #if SOCKET_RUNTIME_PLATFORM_MACOS
      const auto rgba = [backgroundColor colorUsingColorSpace: NSColorSpace.sRGBColorSpace];
    #elif SOCKET_RUNTIME_PLATFORM_IOS
      const auto rgba = [backgroundColor colorWithAlphaComponent: 1];
    #endif

      [rgba getRed: &r green: &g blue: &b alpha: &a];

      const auto string  = [NSString
        stringWithFormat: @"rgba(%.0f,%.0f,%.0f,%.1f)",
                          r * 255,
                          g * 255,
                          b * 255,
                          a
      ];

      return string.UTF8String;
    }

    return "";

  }

  void Window::setContextMenu (const String& instanceId, const String& menuSource) {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    const auto mouseLocation = NSEvent.mouseLocation;
    const auto contextMenu = [[NSMenu.alloc initWithTitle: @"contextMenu"] autorelease];
    const auto location = NSPointFromCGPoint(CGPointMake(mouseLocation.x, mouseLocation.y));
    const auto menuItems = split(menuSource, '\n');
    // remove the 'R' prefix as we'll use this value in the menu item "tag" property
    const auto id = std::stoi(instanceId.starts_with("R") ? instanceId.substr(1) : instanceId);

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
  #endif
  }

  void Window::setTrayMenu (const String& value) {
    this->setMenu(value, true);
  }

  void Window::setSystemMenu (const String& value) {
    this->setMenu(value, false);
  }

  void Window::setMenu (const String& menuSource, const bool& isTrayMenu) {
  #if SOCKET_RUNTIME_PLATFORM_MACOS
    auto app = App::sharedApplication();

    if (!app || menuSource.empty()) {
      return;
    }

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
    // menu = [[NSMenu alloc] initWithTitle: @""];

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
          [menuItem setTarget: nil];
          [menuItem setAction: NULL];
        }

        [menuItem setTag: 0]; // only contextMenu uses the tag
      }

      if (!isTrayMenu) {
        // create a top level menu item
        menuItem = [[NSMenuItem alloc] initWithTitle:nssTitle action:nil keyEquivalent: @""];
        [menu addItem: menuItem];
        // set its submenu
        [menuItem setSubmenu: ctx];
        [menuItem release];
        [ctx release];
      }
    }

    if (isTrayMenu) {
      menu.title = nssTitle;
      menu.delegate = (id) app->delegate; // bring the main window to the front when clicked
      menu.autoenablesItems = NO;

      auto bundlePath = [[[NSBundle mainBundle] resourcePath] UTF8String];
      auto cwd = fs::path(bundlePath);
      auto trayIconPath = this->bridge->userConfig["application_tray_icon"];

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

      const auto image = [NSImage.alloc initWithContentsOfFile: @(trayIconPath.c_str())];
      const auto newSize = NSMakeSize(32, 32);
      const auto resizedImage = [NSImage.alloc initWithSize: newSize];

      [resizedImage lockFocus];
      [image
        drawInRect: NSMakeRect(0, 0, newSize.width, newSize.height)
          fromRect: NSZeroRect
         operation: NSCompositingOperationCopy
          fraction: 1.0
      ];
      [resizedImage unlockFocus];

      app->delegate.statusItem.menu = menu;
      app->delegate.statusItem.button.image = resizedImage;
      app->delegate.statusItem.button.toolTip = nssTitle;
      [app->delegate.statusItem retain];
    } else {
      [NSApp setMainMenu: menu];
    }
  #endif
  }

  void Window::handleApplicationURL (const String& url) {
    JSON::Object json = JSON::Object::Entries {{
      "url", url
    }};

    this->bridge->emit("applicationurl", json.str());
  }
}
