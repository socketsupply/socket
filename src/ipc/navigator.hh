#ifndef SSC_IPC_NAVIGATOR_H
#define SSC_IPC_NAVIGATOR_H

#include "../core/types.hh"
#include "../core/config.hh"
#include "../core/platform.hh"

namespace SSC::IPC {
  // forward
  class Bridge;
  class Navigator;
}

#if SSC_PLATFORM_APPLE
@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
@property (nonatomic) SSC::IPC::Navigator* navigator;
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
@end
#endif

namespace SSC::IPC {
  class Navigator {
    public:
      Bridge* bridge = nullptr;
    #if SSC_PLATFORM_APPLE
      SSCNavigationDelegate* navigationDelegate = nullptr;
    #endif

      Navigator (Bridge* bridge);
      ~Navigator ();

      bool handleNavigationRequest (const String& currentURL, const String& requestedURL);
      bool isNavigationRequestAllowed (const String& location, const String& requestURL);
  };
}

#endif
