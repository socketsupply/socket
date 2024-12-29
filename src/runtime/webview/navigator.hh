#ifndef SOCKET_RUNTIME_WEBVIEW_NAVIGATOR_H
#define SOCKET_RUNTIME_WEBVIEW_NAVIGATOR_H

#include "../filesystem.hh"
#include "../url.hh"

#include "webview.hh"

#if SOCKET_RUNTIME_PLATFORM_APPLE
@interface SSCNavigationDelegate : NSObject<WKNavigationDelegate>
@property (nonatomic) ssc::webview::Navigator* navigator;
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

namespace ssc::runtime::webview {
  class IBridge;
  class Navigator {
    public:
      struct Location : public URL {
        struct Mount {
          String filename; // root path on host file system
        };

        struct Resolution {
          enum class Type { Unknown, Resource, Mount };
          Type type = Type::Unknown;
          Mount mount;
          String pathname = "";
          bool redirect = false;

          bool isUnknown () const;
          bool isResource () const;
          bool isMount () const;
        };

        IBridge* bridge = nullptr;
        Map<String, String> workers;
        Map<String, String> mounts;

        Location () = delete;
        Location (const Location&) = delete;
        Location (IBridge* bridge);

        void init ();
        void assign (const String& url);

        const Resolution resolve (
          const Path& pathname,
          const Path& dirname = filesystem::Resource::getResourcesPath()
        );

        const Resolution resolve (
          const String& pathname,
          const String& dirname = filesystem::Resource::getResourcesPath().string()
        );
      };

      //ServiceWorkerContainer& serviceWorker;
      Location location;
      IBridge* bridge = nullptr;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCNavigationDelegate* navigationDelegate = nullptr;
    #endif

      Navigator () = delete;
      Navigator (const Navigator&) = delete;
      Navigator (IBridge* bridge);
      ~Navigator ();

      void init ();
      void configureWebView (WebView* object);
      bool handleNavigationRequest (const String& currentURL, const String& requestedURL);
      bool isNavigationRequestAllowed (const String& location, const String& requestURL);
      void configureMounts ();
  };
}
#endif
