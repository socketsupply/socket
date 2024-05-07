#ifndef SSC_WINDOW_WEBVIEW_H
#define SSC_WINDOW_WEBVIEW_H

#include "../core/types.hh"
#include "../core/platform.hh"

namespace SSC {
  // forward
  class Window;
}

#if SSC_PLATFORM_APPLE
@interface SSCBridgedWebView :
#if SSC_PLATFORM_IOS
  WKWebView<WKUIDelegate>
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

#if SSC_PLATFORM_MACOS
  -   (NSDragOperation) draggingSession: (NSDraggingSession *) session
  sourceOperationMaskForDraggingContext: (NSDraggingContext) context;

  -             (void) webView: (WKWebView*) webView
    runOpenPanelWithParameters: (WKOpenPanelParameters*) parameters
              initiatedByFrame: (WKFrameInfo*) frame
             completionHandler: (void (^)(NSArray<NSURL*>*)) completionHandler;
#endif

#if SSC_PLATFORM_MACOS || (SSC_PLATFORM_IOS && __IPHONE_OS_VERSION_MIN_REQUIRED >= __IPHONE_15)

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
#endif

namespace SSC {
#if SSC_PLATFORM_APPLE
  using WebView = SSCBridgedWebView;
  using WebViewSettings = WKWebViewConfiguration;
#elif SSC_PLATFORM_LINUX
  using WebView = WebKitWebView;
  using WebViewSettings = WebKitSettings;
#elif SSC_PLATFORM_WINDOWS
  using WebView = ICoreWebView2;
  using WebViewSettings = ComPtr<CoreWebView2EnvironmentOptions>;
#else
  struct WebView;
  struct WebViewSettings;
#endif
}

#endif
