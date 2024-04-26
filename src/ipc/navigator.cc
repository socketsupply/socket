#include "navigator.hh"
#include "router.hh"
#include "bridge.hh"

#if SSC_PLATFORM_APPLE
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
  using namespace SSC;
  if (
    webview != nullptr &&
    webview.URL != nullptr &&
    webview.URL.absoluteString.UTF8String != nullptr &&
    navigationAction != nullptr &&
    navigationAction.request.URL.absoluteString.UTF8String != nullptr
  ) {
    const auto currentURL = String(webview.URL.absoluteString.UTF8String);
    const auto requestURL = String(navigationAction.request.URL.absoluteString.UTF8String);

    if (!self.navigator->handleNavigationRequest(currentURL, requestURL)) {
      return decisionHandler(WKNavigationActionPolicyCancel);
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

#endif

namespace SSC::IPC {
  Navigator::Navigator (Bridge* bridge)
    : bridge(bridge)
  {
  #if SSC_PLATFORM_APPLE
    this->navigationDelegate = [SSCNavigationDelegate new];
    this->navigationDelegate.navigator = this;
  #endif
  }

  Navigator::~Navigator () {
  #if SSC_PLATFORM_APPLE
  #if !__has_feature(objc_arc)
    [this->navigationDelegate release];
  #endif
    this->navigationDelegate = nullptr;
  #endif
  }

  #if SSC_PLATFORM_LINUX
  void Navigator::configureWebView (WebKitWebView* webview) {
    g_signal_connect(
      G_OBJECT(webview),
      "decide-policy",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPolicyDecision* decision,
        WebKitPolicyDecisionType decisionType,
        gpointer userData
      ) {
        auto navigator = reinterpret_cast<Navigator*>(userData);

        if (decisionType != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
          webkit_policy_decision_use(decision);
          return true;
        }

        const auto navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        const auto action = webkit_navigation_policy_decision_get_navigation_action(navigation);
        const auto request = webkit_navigation_action_get_request(action);
        const auto currentURL = String(webkit_web_view_get_uri(webview));
        const auto requestedURL = String(webkit_uri_request_get_uri(request)

        if (!navigator->handleNavigationRequest(currentURL, requestedURL)) {
          webkit_policy_decision_ignore(decision);
          return false;
        }

        return true;
      })),
      this
    );
  }
  #elif SSC_PLATFORM_WINDOWS
  void Navigator::configureWebView (ICoreWebView2* webview) {
    EventRegistrationToken tokenNavigation;
    webview->add_NavigationStarting(
      Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
        [this, &](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs *event) {
          PWSTR source;
          PWSTR uri;

          event->get_Uri(&uri);
          webview->get_Source(&source);

          if (uri == nullptr || source == nullptr) {
            if (uri) CoTaskMemFree(uri);
            if (source) CoTaskMemFree(source);
            return E_POIINTER;
          }

          const auto requestedURL = convertWStringToString(uri);
          const auto currentURL = convertWStringToString(source);

          if (!this->handleNavigationRequest(currentURL, requestedURL)) {
            event->put_Cancel(true);
          }

          CoTaskMemFree(uri);
          CoTaskMemFree(source);
          return S_OK;
        }
      ).Get(),
      &tokenNavigation
    );
  }
#endif

  bool Navigator::handleNavigationRequest (
    const String& currentURL,
    const String& requestedURL
  ) {
    auto userConfig = this->bridge->userConfig;
    const auto links = parseStringList(userConfig["meta_application_links"], ' ');
    const auto applinks = parseStringList(userConfig["meta_application_links"], ' ');
    const auto currentURLComponents = URL::Components::parse(currentURL);

    bool hasAppLink = false;
    if (applinks.size() > 0 && currentURLComponents.authority.size() > 0) {
      const auto host = currentURLComponents.authority;
      for (const auto& applink : applinks) {
        const auto parts = split(applink, '?');
        if (host == parts[0]) {
          hasAppLink = true;
          break;
        }
      }
    }

    if (hasAppLink) {
      SSC::JSON::Object json = SSC::JSON::Object::Entries {{
        "url", requestedURL
      }};

      this->bridge->router.emit("applicationurl", json.str());
      return false;
    }

    if (
      userConfig["meta_application_protocol"].size() > 0 &&
      requestedURL.starts_with(userConfig["meta_application_protocol"]) &&
      !requestedURL.starts_with("socket://" + userConfig["meta_bundle_identifier"])
    ) {

      SSC::JSON::Object json = SSC::JSON::Object::Entries {{
        "url", requestedURL
      }};

      this->bridge->router.emit("applicationurl", json.str());
      return false;
    }

    if (!this->isNavigationRequestAllowed(currentURL, requestedURL)) {
      debug("Navigation was ignored for: %s", requestedURL.c_str());
      return false;
    }

    return true;
  }

  bool Navigator::isNavigationRequestAllowed (
    const String& currentURL,
    const String& requestedURL
  ) {
    static const auto devHost = getDevHost();
    auto userConfig = this->bridge->userConfig;
    const auto allowed = split(trim(userConfig["webview_navigator_policies_allowed"]), ' ');

    for (const auto& entry : split(userConfig["webview_protocol-handlers"], " ")) {
      const auto scheme = replace(trim(entry), ":", "");
      if (requestedURL.starts_with(scheme + ":")) {
        return true;
      }
    }

    for (const auto& entry : userConfig) {
      const auto& key = entry.first;
      if (key.starts_with("webview_protocol-handlers_")) {
        const auto scheme = replace(replace(trim(key), "webview_protocol-handlers_", ""), ":", "");;
        if (requestedURL.starts_with(scheme + ":")) {
          return true;
        }
      }
    }

    for (const auto& entry : allowed) {
      String pattern = entry;
      pattern = replace(pattern, "\\.", "\\.");
      pattern = replace(pattern, "\\*", "(.*)");
      pattern = replace(pattern, "\\.\\.\\*", "(.*)");
      pattern = replace(pattern, "\\/", "\\/");

      try {
        std::regex regex(pattern);
        std::smatch match;

        if (std::regex_match(requestedURL, match, regex, std::regex_constants::match_any)) {
          return true;
        }
      } catch (...) {}
    }

    if (
      requestedURL.starts_with("socket:") ||
      requestedURL.starts_with("npm:") ||
      requestedURL.starts_with(devHost)
    ) {
      return true;
    }

    return false;
  }
}
