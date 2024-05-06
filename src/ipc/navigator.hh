#ifndef SSC_IPC_NAVIGATOR_H
#define SSC_IPC_NAVIGATOR_H

#include "../core/url.hh"
#include "../core/types.hh"
#include "../core/config.hh"
#include "../core/platform.hh"
#include "../core/resource.hh"
#include "../core/service_worker_container.hh"
#include "../window/webview.hh"

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

        Bridge* bridge = nullptr;
        Map workers;
        Map mounts;

        Location () = delete;
        Location (const Location&) = delete;
        Location (Bridge* bridge);

        void init ();
        void assign (const String& url);

        const Resolution resolve (
          const Path& pathname,
          const Path& dirname = FileResource::getResourcesPath()
        );

        const Resolution resolve (
          const String& pathname,
          const String& dirname = FileResource::getResourcesPath().string()
        );
      };

      ServiceWorkerContainer& serviceWorker;
      Location location;
      Bridge* bridge = nullptr;

    #if SSC_PLATFORM_APPLE
      SSCNavigationDelegate* navigationDelegate = nullptr;
    #endif

      Navigator () = delete;
      Navigator (const Navigator&) = delete;
      Navigator (Bridge* bridge);
      ~Navigator ();

      void init ();
      void configureWebView (WebView* object);
      bool handleNavigationRequest (const String& currentURL, const String& requestedURL);
      bool isNavigationRequestAllowed (const String& location, const String& requestURL);
  };
}

#endif
