#ifndef SOCKET_RUNTIME_WEBVIEW_WEBVIEW_H
#define SOCKET_RUNTIME_WEBVIEW_WEBVIEW_H

#include "../unique_client.hh"
#include "../platform.hh"
#include "../options.hh"
#include "../config.hh"
#include "../http.hh"
#include "../json.hh"

namespace ssc::runtime::webview {
  class Navigator;
}

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
@property (nonatomic) ssc::runtime::webview::Navigator* navigator;
-    (void) webView: (WKWebView*) webView
  didFailNavigation: (WKNavigation*) navigation
          withError: (NSError*) error;

-               (void) webView: (WKWebView*) webView
  didFailProvisionalNavigation: (WKNavigation*) navigation
                     withError: (NSError*) error;

-                  (void) webView: (WKWebView*) webview
  decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                  decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler;

-                    (void) webView: (WKWebView*) webView
  decidePolicyForNavigationResponse: (WKNavigationResponse*) navigationResponse
                    decisionHandler: (void (^)(WKNavigationResponsePolicy)) decisionHandler;

-      (void) webView: (WKWebView*) webView
  didFinishNavigation: (WKNavigation*) navigation;
@end
@interface SSCWebView :
#if SOCKET_RUNTIME_PLATFORM_IOS
  WKWebView<WKUIDelegate>
  @property (strong, nonatomic) NSLayoutConstraint* keyboardHeightConstraint;
  @property (strong, nonatomic) UIRefreshControl* refreshControl;
  - (void) handlePullToRefresh: (UIRefreshControl*) refreshControl;
  - (instancetype) initWithFrame: (CGRect) frame
                   configuration: (WKWebViewConfiguration*) configuration
       withRefreshControlEnabled: (BOOL) refreshControlEnabled;
#else
  WKWebView<
    WKUIDelegate,
    NSDraggingDestination,
    NSFilePromiseProviderDelegate,
    NSDraggingSource
  >

  @property (nonatomic) NSPoint initialWindowPos;
  @property (nonatomic) CGFloat contentHeight;
  @property (nonatomic) CGFloat radius;
  @property (nonatomic) CGFloat margin;
  @property (nonatomic) BOOL shouldDrag;
#endif

#if SOCKET_RUNTIME_PLATFORM_MACOS
- (instancetype) initWithFrame: (NSRect) frameRect
                 configuration: (WKWebViewConfiguration*) configuration
                        radius: (CGFloat) radius
                        margin: (CGFloat) margin;

  -   (NSDragOperation) draggingSession: (NSDraggingSession *) session
  sourceOperationMaskForDraggingContext: (NSDraggingContext) context;

  -             (void) webView: (WKWebView*) webView
    runOpenPanelWithParameters: (WKOpenPanelParameters*) parameters
              initiatedByFrame: (WKFrameInfo*) frame
             completionHandler: (void (^)(NSArray<NSURL*>*)) completionHandler;
#endif

#if SOCKET_RUNTIME_PLATFORM_MACOS || (SOCKET_RUNTIME_PLATFORM_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)

  -                                      (void) webView: (WKWebView*) webView
   requestDeviceOrientationAndMotionPermissionForOrigin: (WKSecurityOrigin*) origin
                                       initiatedByFrame: (WKFrameInfo*) frame
                                        decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler;

  -                        (void) webView: (WKWebView*) webView
   requestMediaCapturePermissionForOrigin: (WKSecurityOrigin*) origin
                         initiatedByFrame: (WKFrameInfo*) frame
                                     type: (WKMediaCaptureType) type
                          decisionHandler: (void (^)(WKPermissionDecision decision)) decisionHandler;
#endif

  -                     (void) webView: (WKWebView*) webView
    runJavaScriptAlertPanelWithMessage: (NSString*) message
                      initiatedByFrame: (WKFrameInfo*) frame
                     completionHandler: (void (^)(void)) completionHandler;

  -                       (void) webView: (WKWebView*) webView
    runJavaScriptConfirmPanelWithMessage: (NSString*) message
                        initiatedByFrame: (WKFrameInfo*) frame
                       completionHandler: (void (^)(BOOL result)) completionHandler;
@end

#if SOCKET_RUNTIME_PLATFORM_IOS
@interface SSCWebViewController : UIViewController<UIGestureRecognizerDelegate>
  @property (nonatomic, strong) SSCWebView* webview;

-                         (BOOL) gestureRecognizer: (UIGestureRecognizer*) gestureRecognizer
shouldRecognizeSimultaneouslyWithGestureRecognizer: (UIGestureRecognizer*) otherGestureRecognizer;

- (BOOL) gestureRecognizer: (UIGestureRecognizer*) gestureRecognizer
        shouldReceiveTouch: (UITouch*) touch;
@end

#endif
#endif

namespace ssc::runtime::webview {
  using Headers = http::Headers;

#if SOCKET_RUNTIME_PLATFORM_ANDROID
  class CoreAndroidWebView;
  class CoreAndroidWebViewSettings;
#endif

#if SOCKET_RUNTIME_PLATFORM_APPLE
  using WebView = SSCWebView;
  using WebViewSettings = WKWebViewConfiguration;
#elif SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
  using WebView = WebKitWebView;
  using WebViewSettings = WebKitSettings;
#elif SOCKET_RUNTIME_PLATFORM_WINDOWS
  using WebView = ICoreWebView2;
  using WebViewSettings = Microsoft::WRL::ComPtr<CoreWebView2EnvironmentOptions>;
#elif SOCKET_RUNTIME_PLATFORM_ANDROID
  using WebView = CoreAndroidWebView;
  using WebViewSettings = CoreAndroidWebViewSettings;
#else
  struct WebView;
  struct WebViewSettings;
#endif
}
#endif
