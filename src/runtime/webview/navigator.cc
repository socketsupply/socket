#include "../filesystem.hh"
#include "../runtime.hh"
#include "../config.hh"
#include "../string.hh"
#include "../app.hh"

#include "../webview.hh"

using namespace ssc::runtime;

using ssc::runtime::config::getDevHost;
using ssc::runtime::string::parseStringList;
using ssc::runtime::string::replace;
using ssc::runtime::string::split;
using ssc::runtime::string::trim;
using ssc::runtime::app::App;

#if SOCKET_RUNTIME_PLATFORM_APPLE
@implementation SSCNavigationDelegate
-    (void) webView: (WKWebView*) webview
  didFailNavigation: (WKNavigation*) navigation
          withError: (NSError*) error
{
  // TODO(@jwerle)
}

-               (void) webView: (WKWebView*) webview
  didFailProvisionalNavigation: (WKNavigation*) navigation
                     withError: (NSError*) error
{
  // TODO(@jwerle)
}

-                    (void) webView: (WKWebView*) webview
    decidePolicyForNavigationAction: (WKNavigationAction*) navigationAction
                    decisionHandler: (void (^)(WKNavigationActionPolicy)) decisionHandler
{
  using namespace ssc::runtime;
  if (
    webview != nullptr &&
    webview.URL != nullptr &&
    webview.URL.absoluteString.UTF8String != nullptr &&
    navigationAction != nullptr &&
    navigationAction.request.URL.absoluteString.UTF8String != nullptr
  ) {
    const auto currentURL = String(webview.URL.absoluteString.UTF8String);
    const auto requestedURL = String(navigationAction.request.URL.absoluteString.UTF8String);

    if (!self.navigator->handleNavigationRequest(currentURL, requestedURL)) {
      return decisionHandler(WKNavigationActionPolicyCancel);
    }
  }

  decisionHandler(WKNavigationActionPolicyAllow);
}

-                    (void) webView: (WKWebView*) webview
  decidePolicyForNavigationResponse: (WKNavigationResponse*) navigationResponse
                    decisionHandler: (void (^)(WKNavigationResponsePolicy)) decisionHandler
{
  decisionHandler(WKNavigationResponsePolicyAllow);
}
@end
#endif

namespace ssc::runtime::webview {
  Navigator::Location::Location (IBridge* bridge)
    : bridge(bridge),
      URL()
  {}

  void Navigator::Location::init () {}

  /**
   * .
   * ├── a-conflict-index
   * │             └── index.html
   * ├── a-conflict-index.html
   * ├── an-index-file
   * │             ├── a-html-file.html
   * │             └── index.html
   * ├── another-file.html
   * └── index.html
   *
   * Subtleties:
   * Direct file navigation always wins
   * /foo/index.html have precedent over foo.html
   * /foo redirects to /foo/ when there is a /foo/index.html
   *
   * '/' -> '/index.html'
   * '/index.html' -> '/index.html'
   * '/a-conflict-index' -> redirect to '/a-conflict-index/'
   * '/another-file' -> '/another-file.html'
   * '/another-file.html' -> '/another-file.html'
   * '/an-index-file/' -> '/an-index-file/index.html'
   * '/an-index-file' -> redirect to '/an-index-file/'
   * '/an-index-file/a-html-file' -> '/an-index-file/a-html-file.html'
   **/
  static const Navigator::Location::Resolution resolveLocationPathname (
    const String& pathname,
    const String& dirname
  ) {
    auto result = pathname;

    if (result.starts_with("/")) {
      result = result.substr(1);
    }

    // Resolve the full path
    const auto filename = (fs::path(dirname) / fs::path(result)).make_preferred();

    // 1. Try the given path if it's a file
    if (filesystem::Resource::isFile(filename)) {
      return Navigator::Location::Resolution {
        .pathname = "/" + replace(fs::relative(filename, dirname).string(), "\\\\", "/")
      };
    }

    // 2. Try appending a `/` to the path and checking for an index.html
    const auto index = filename / fs::path("index.html");
    if (filesystem::Resource::isFile(index)) {
      if (filename.string().ends_with("\\") || filename.string().ends_with("/")) {
        return Navigator::Location::Resolution {
          .pathname = "/" + replace(fs::relative(index, dirname).string(), "\\\\", "/"),
          .redirect = false
        };
      } else {
        return Navigator::Location::Resolution {
          .pathname = "/" + replace(fs::relative(filename, dirname).string(), "\\\\", "/") + "/",
          .redirect = true
        };
      }
    }

    // 3. Check if appending a .html file extension gives a valid file
    const auto html = Path(filename).replace_extension(".html");
    if (filesystem::Resource::isFile(html)) {
      return Navigator::Location::Resolution {
        .pathname = "/" + replace(fs::relative(html, dirname).string(), "\\\\", "/")
      };
    }

    // If no valid path is found, return empty string
    return Navigator::Location::Resolution {};
  }

  const Navigator::Location::Resolution Navigator::Location::resolve (const Path& pathname, const Path& dirname) {
    return this->resolve(pathname.string(), dirname.string());
  }

  const Navigator::Location::Resolution Navigator::Location::resolve (const String& pathname, const String& dirname) {
    for (const auto& entry : this->mounts) {
      if (pathname.starts_with(entry.second)) {
        const auto relative = replace(pathname, entry.second, "");
        auto resolution = resolveLocationPathname(relative, entry.first);
        if (resolution.pathname.size() > 0) {
          const auto filename = Path(entry.first) / resolution.pathname.substr(1);
          resolution.type = Navigator::Location::Resolution::Type::Mount;
          resolution.mount.filename = filename.string();
          return resolution;
        }
      }
    }

    auto resolution = resolveLocationPathname(pathname, dirname);
    if (resolution.pathname.size() > 0) {
      resolution.type = Navigator::Location::Resolution::Type::Resource;
    }
    return resolution;
  }

  bool Navigator::Location::Resolution::isUnknown () const {
    return this->type == Navigator::Location::Resolution::Type::Unknown;
  }

  bool Navigator::Location::Resolution::isResource () const {
    return this->type == Navigator::Location::Resolution::Type::Resource;
  }

  bool Navigator::Location::Resolution::isMount () const {
    return this->type == Navigator::Location::Resolution::Type::Mount;
  }

  void Navigator::Location::assign (const String& url) {
    this->set(url);
    this->bridge->navigate(url);
  }

  Navigator::Navigator (IBridge* bridge)
    : bridge(bridge),
      location(bridge)
  {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    this->navigationDelegate = [SSCNavigationDelegate new];
    this->navigationDelegate.navigator = this;
  #endif
  }

  Navigator::~Navigator () {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->navigationDelegate) {
      this->navigationDelegate.navigator = nullptr;

    #if !__has_feature(objc_arc)
      [this->navigationDelegate release];
    #endif
    }

    this->navigationDelegate = nullptr;
  #endif
  }

  void Navigator::init () {
    this->location.init();
  }

  void Navigator::configureWebView (WebView* webview) {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    webview.navigationDelegate = this->navigationDelegate;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    g_signal_connect(
      G_OBJECT(webview),
      "decide-policy",
      G_CALLBACK((+[](
        WebKitWebView* webview,
        WebKitPolicyDecision* decision,
        WebKitPolicyDecisionType decisionType,
        gpointer userData
      ) {
        const auto navigator = reinterpret_cast<Navigator*>(userData);
        auto window = navigator->bridge->context.getRuntime()->windowManager.getWindowForWebView(webview);

        if (!window) {
          webkit_policy_decision_ignore(decision);
          return false;
        }

        if (decisionType != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION) {
          webkit_policy_decision_use(decision);
          return true;
        }

        const auto navigation = WEBKIT_NAVIGATION_POLICY_DECISION(decision);
        const auto action = webkit_navigation_policy_decision_get_navigation_action(navigation);
        const auto request = webkit_navigation_action_get_request(action);
        const auto currentURL = String(webkit_web_view_get_uri(webview));
        const auto requestedURL = String(webkit_uri_request_get_uri(request));

        if (!navigator->handleNavigationRequest(currentURL, requestedURL)) {
          webkit_policy_decision_ignore(decision);
          return false;
        }

        webkit_policy_decision_use(decision);
        return true;
      })),
      this
    );
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    EventRegistrationToken token;
    webview->add_NavigationStarting(
      Microsoft::WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
        [=, this](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) {
          PWSTR source;
          PWSTR uri;

          args->get_Uri(&uri);
          webview->get_Source(&source);

          if (uri == nullptr || source == nullptr) {
            if (uri) CoTaskMemFree(uri);
            if (source) CoTaskMemFree(source);
            return E_POINTER;
          }

          const auto requestedURL = convertWStringToString(uri);
          const auto currentURL = convertWStringToString(source);

          if (!this->handleNavigationRequest(currentURL, requestedURL)) {
            args->put_Cancel(true);
          }

          CoTaskMemFree(uri);
          CoTaskMemFree(source);
          return S_OK;
        }
      ).Get(),
      &token
    );
  #endif
  }

  bool Navigator::handleNavigationRequest (
    const String& currentURL,
    const String& requestedURL
  ) {
    auto userConfig = this->bridge->userConfig;
    const auto links = parseStringList(userConfig["meta_application_links"], ' ');
    const auto window = this->bridge->context.getRuntime()->windowManager.getWindowForBridge(
      reinterpret_cast<window::IBridge*>(this->bridge)
    );
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
      if (window) {
        window->handleApplicationURL(requestedURL);
        return false;
      }

      // should be unreachable, but...
      return true;
    }

    if (
      userConfig["meta_application_protocol"].size() > 0 &&
      requestedURL.starts_with(userConfig["meta_application_protocol"]) &&
      !requestedURL.starts_with("socket://" + userConfig["meta_bundle_identifier"])
    ) {

      if (window) {
        window->handleApplicationURL(requestedURL);
        return false;
      }

      // should be unreachable, but...
      return true;
    }

    if (!this->isNavigationRequestAllowed(currentURL, requestedURL)) {
      debug("IPC::Navigation: A navigation request was ignored for: %s", requestedURL.c_str());
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

    if (requestedURL == "about:blank") {
      return true;
    }

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
      requestedURL.starts_with("node:") ||
      requestedURL.starts_with("npm:") ||
      requestedURL.starts_with("ipc:") ||
      requestedURL.starts_with(devHost)
    ) {
      return true;
    }

    return false;
  }

  void Navigator::configureMounts () {
    static const auto wellKnownPaths = filesystem::Resource::getWellKnownPaths();
    this->location.mounts = filesystem::Resource::getMountedPaths();

    for (const auto& entry : this->location.mounts) {
      const auto& path = entry.first;
      #if SOCKET_RUNTIME_PLATFORM_LINUX
        auto webContext = this->bridge->webContext;
        if (path != wellKnownPaths.home.string()) {
          webkit_web_context_add_path_to_sandbox(webContext, path.c_str(), false);
        }
      #endif
    }

  #if SOCKET_RUNTIME_PLATFORM_LINUX
    auto webContext = this->bridge->webContext;
    for (const auto& entry : wellKnownPaths.entries()) {
      if (filesystem::Resource::isDirectory(entry) && entry != wellKnownPaths.home) {
        webkit_web_context_add_path_to_sandbox(webContext, entry.c_str(), false);
      }
    }
  #endif
  }
}

#if SOCKET_RUNTIME_PLATFORM_ANDROID
extern "C" {
  jboolean ANDROID_EXTERNAL(ipc, Navigator, isNavigationRequestAllowed) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring currentURLString,
    jstring requestedURLString
  ) {
    auto app = App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window requested");
      return false;
    }

    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    const auto currentURL = android::StringWrap(attachment.env, currentURLString).str();
    const auto requestedURL = android::StringWrap(attachment.env, requestedURLString).str();

    return window->bridge->navigator.isNavigationRequestAllowed(currentURL, requestedURL);
  }
}
#endif
