#include "../platform.hh"
#include "../window.hh"
#include "../bridge.hh"
#include "../config.hh"
#include "../debug.hh"
#include "../http.hh"
#include "../app.hh"

#include "../webview.hh"

using namespace ssc::runtime;
using namespace ssc::runtime::webview;
using ssc::runtime::url::decodeURIComponent;
using ssc::runtime::config::isDebugEnabled;
using ssc::runtime::config::getUserConfig;
using ssc::runtime::config::getDevHost;
using ssc::runtime::http::toHeaderCase;
using ssc::runtime::string::toUpperCase;
using ssc::runtime::string::toLowerCase;
using ssc::runtime::string::split;
using ssc::runtime::string::trim;
using ssc::runtime::string::tmpl;
using ssc::runtime::app::App;

#if SOCKET_RUNTIME_PLATFORM_APPLE
using Task = id<WKURLSchemeTask>;

@class SSCWebView;
@interface SSCInternalWKURLSchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) ssc::runtime::webview::SchemeHandlers* handlers;

-     (void) webView: (SSCWebView*) webview
  startURLSchemeTask: (id<WKURLSchemeTask>) task;

-    (void) webView: (SSCWebView*) webview
  stopURLSchemeTask: (id<WKURLSchemeTask>) task;
@end

@implementation SSCInternalWKURLSchemeHandler
{
  Mutex mutex;
  UnorderedMap<Task, uint64_t> tasks;
}

- (void) enqueueTask: (Task) task
       withRequestID: (uint64_t) id
{
  Lock lock(mutex);
  if (task != nullptr && !tasks.contains(task)) {
    tasks.emplace(task, id);
  }
}

- (void) finalizeTask: (Task) task {
  Lock lock(mutex);
  if (task != nullptr && tasks.contains(task)) {
    tasks.erase(task);
  }
}

- (bool) waitingForTask: (Task) task {
  Lock lock(mutex);
  return task != nullptr && tasks.contains(task);
}

-    (void) webView: (SSCWebView*) webview
  stopURLSchemeTask: (Task) task
{
  if (tasks.contains(task)) {
    const auto id = tasks[task];
    if (self.handlers->isRequestActive(id)) {
      auto request = self.handlers->activeRequests[id];
      request->cancelled = true;
      if (request->callbacks.cancel != nullptr) {
        request->callbacks.cancel();
      }
    }
  }

  [self finalizeTask: task];
}

-     (void) webView: (SSCWebView*) webview
  startURLSchemeTask: (Task) task
{
  if (self.handlers == nullptr) {
    auto& userConfig = self.handlers->bridge->userConfig;
    const auto bundleIdentifier = userConfig.contains("meta_bundle_identifier")
      ? userConfig.at("meta_bundle_identifier")
      : "";

    [task didFailWithError: [NSError
         errorWithDomain: @(bundleIdentifier.c_str())
                    code: 1
                userInfo: @{NSLocalizedDescriptionKey: @("SchemeHandlers::Response: Request is in an invalid state")}
    ]];
    return;
  }

  auto request = SchemeHandlers::Request::Builder(self.handlers, task)
    .setMethod(toUpperCase(task.request.HTTPMethod.UTF8String))
    // copies all headers
    .setHeaders(task.request.allHTTPHeaderFields)
    // copies request body
    .setBody(task.request.HTTPBody)
    .build();

  [self enqueueTask: task withRequestID: request->id];
  const auto handled = self.handlers->handleRequest(request, [=](const auto& response) {
    [self finalizeTask: task];
  });

  if (!handled) {
    auto response = SchemeHandlers::Response(request, 404);
    response.finish();
    [self finalizeTask: task];
  }
}
@end
#elif SOCKET_RUNTIME_PLATFORM_LINUX
static const auto MAX_URI_SCHEME_REQUEST_BODY_BYTES = 4 * 1024 * 1024;
static void onURISchemeRequest (WebKitURISchemeRequest* schemeRequest, gpointer userData) {
  static auto globalUserConfig = getUserConfig();
  static auto app = App::sharedApplication();

  if (!app) {
    const auto quark = g_quark_from_string(globalUserConfig["meta_bundle_identifier"].c_str());
    const auto error = g_error_new(quark, 1, "SchemeHandlers::Request: Missing WindowManager in request");
    webkit_uri_scheme_request_finish_error(schemeRequest, error);
    return;
  }

  auto webview = webkit_uri_scheme_request_get_web_view(schemeRequest);
  auto window = app->runtime.windowManager.getWindowForWebView(webview);

  if (!window) {
    const auto quark = g_quark_from_string(globalUserConfig["meta_bundle_identifier"].c_str());
    const auto error = g_error_new(quark, 1, "SchemeHandlers::Request: Missing Window in request");
    webkit_uri_scheme_request_finish_error(schemeRequest, error);
    return;
  }

  auto bridge = window->bridge;
  auto request = ssc::runtime::webview::SchemeHandlers::Request::Builder(&bridge->schemeHandlers, schemeRequest)
    .setMethod(String(webkit_uri_scheme_request_get_http_method(schemeRequest)))
    // copies all request soup headers
    .setHeaders(webkit_uri_scheme_request_get_http_headers(schemeRequest))
    // reads and copies request stream body
    .setBody(webkit_uri_scheme_request_get_http_body(schemeRequest))
    .build();

  const auto handled = bridge->schemeHandlers.handleRequest(request, [=](const auto& response) mutable {
  });

  if (!handled) {
    auto response = SchemeHandlers::Response(request, 404);
    response.finish();
  }
}
#elif SOCKET_RUNTIME_PLATFORM_ANDROID
extern "C" {
  jboolean ANDROID_EXTERNAL(ipc, SchemeHandlers, handleRequest) (
    JNIEnv* env,
    jobject self,
    jint index,
    jobject requestObject
  ) {
    auto app = app::App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window requested");
      return false;
    }

    const auto method = android::StringWrap(env, (jstring) CallClassMethodFromAndroidEnvironment(
      env,
      Object,
      requestObject,
      "getMethod",
      "()Ljava/lang/String;"
    )).str();

    const auto headers = android::StringWrap(env, (jstring) CallClassMethodFromAndroidEnvironment(
      env,
      Object,
      requestObject,
      "getHeaders",
      "()Ljava/lang/String;"
    )).str();

    const auto requestBodyByteArray = (jbyteArray) CallClassMethodFromAndroidEnvironment(
      env,
      Object,
      requestObject,
      "getBody",
      "()[B"
    );

    const auto requestBodySize = requestBodyByteArray != nullptr
      ? env->GetArrayLength(requestBodyByteArray)
      : 0;

    const auto bytes = requestBodySize > 0
      ? new char[requestBodySize]{0}
      : nullptr;

    if (requestBodyByteArray) {
      env->GetByteArrayRegion(
        requestBodyByteArray,
        0,
        requestBodySize,
        (jbyte*) bytes
      );
    }

    const auto requestObjectRef = env->NewGlobalRef(requestObject);
    const auto request = webview::SchemeHandlers::Request::Builder(
      &window->bridge->schemeHandlers,
      requestObjectRef
    )
      .setMethod(method)
      // copies all request soup headers
      .setHeaders(headers)
      // reads and copies request stream body
      .setBody(requestBodySize, bytes)
      .build();

    const auto handled = window->bridge->schemeHandlers.handleRequest(request, [=](const auto& response) {
      if (bytes) {
        delete [] bytes;
      }

      const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
      attachment.env->DeleteGlobalRef(requestObjectRef);
    });

    if (!handled) {
      env->DeleteGlobalRef(requestObjectRef);
    }

    return handled;
  }

  jboolean ANDROID_EXTERNAL(ipc, SchemeHandlers, hasHandlerForScheme) (
    JNIEnv* env,
    jobject self,
    jint index,
    jstring schemeString
  ) {
    auto app = app::App::sharedApplication();

    if (!app) {
      ANDROID_THROW(env, "Missing 'App' in environment");
      return false;
    }

    const auto window = app->runtime.windowManager.getWindow(index);

    if (!window) {
      ANDROID_THROW(env, "Invalid window requested");
      return false;
    }

    const auto scheme = android::StringWrap(env, schemeString).str();
    return window->bridge->schemeHandlers.hasHandlerForScheme(scheme);
  }
}
#endif

namespace ssc::runtime::webview {
  class SchemeHandlersInternals {
    public:
      SchemeHandlers* handlers = nullptr;
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCInternalWKURLSchemeHandler* schemeHandler = nullptr;
    #endif

      SchemeHandlersInternals (SchemeHandlers* handlers) {
        this->handlers = handlers;
      #if SOCKET_RUNTIME_PLATFORM_APPLE
        this->schemeHandler = [SSCInternalWKURLSchemeHandler new];
        this->schemeHandler.handlers = handlers;
      #endif
      }

      ~SchemeHandlersInternals () {
      #if SOCKET_RUNTIME_PLATFORM_APPLE
        if (this->schemeHandler != nullptr) {
          this->schemeHandler.handlers = nullptr;
        #if !__has_feature(objc_arc)
          [this->schemeHandler release];
        #endif
          this->schemeHandler = nullptr;
        }
      #endif
      }
  };
}

namespace ssc::runtime::webview {
#if SOCKET_RUNTIME_PLATFORM_LINUX
  static Map<String, Set<String>> globallyRegisteredSchemesForLinux;
#endif

  SchemeHandlers::SchemeHandlers (IBridge* bridge)
    : bridge(bridge)
  {
    this->internals = new SchemeHandlersInternals(this);
  }

  SchemeHandlers::~SchemeHandlers () {
    for (auto& entry : this->activeRequests) {
      entry.second->handlers = nullptr;
    }
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    this->configuration.webview = nullptr;
  #endif

    if (this->internals != nullptr) {
      delete this->internals;
      this->internals = nullptr;
    }
  }

  void SchemeHandlers::init () {}

  void SchemeHandlers::configure (const Configuration& configuration) {
    static const auto devHost = getDevHost();
    this->configuration = configuration;
  }

  bool SchemeHandlers::hasHandlerForScheme (const String& scheme) {
    Lock lock(this->mutex);
    return this->handlers.contains(scheme);
  }

  SchemeHandlers::Handler SchemeHandlers::getHandlerForScheme (const String& scheme) {
    Lock lock(this->mutex);
    return this->handlers.contains(scheme)
      ? this->handlers.at(scheme)
      : SchemeHandlers::Handler {};
  }

  bool SchemeHandlers::registerSchemeHandler (const String& scheme, const Handler& handler) {
    if (scheme.size() == 0 || this->hasHandlerForScheme(scheme)) {
      return false;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (this->configuration.webview == nullptr) {
      return false;
    }

    [this->configuration.webview
      setURLSchemeHandler: this->internals->schemeHandler
             forURLScheme: @(scheme.c_str())
    ];

  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    const auto bundleIdentifier = this->bridge->userConfig["meta_bundle_identifier"];
    if (!globallyRegisteredSchemesForLinux.contains(bundleIdentifier)) {
      globallyRegisteredSchemesForLinux[bundleIdentifier] = Set<String> {};
    }

    auto& schemes = globallyRegisteredSchemesForLinux.at(bundleIdentifier);
    // schemes are registered for the globally shared defaut context,
    // despite the `SchemeHandlers` instance bound to a `Router`
    // we'll select the correct `Router` in the callback which will give
    // access to the `SchemeHandlers` that should handle the
    // request and provide a response
    if (
      std::find(
        schemes.begin(),
        schemes.end(),
        scheme
      ) == schemes.end()
    ) {
      schemes.insert(scheme);
      auto context = this->bridge->webContext;
      auto security = webkit_web_context_get_security_manager(context);
      webkit_web_context_register_uri_scheme(
        this->bridge->webContext,
        scheme.c_str(),
        onURISchemeRequest,
        nullptr,
        nullptr
      );
      webkit_security_manager_register_uri_scheme_as_secure(
        security,
        scheme.c_str()
      );
      webkit_security_manager_register_uri_scheme_as_cors_enabled(
        security,
        scheme.c_str()
      );
    }
  #endif

    this->handlers.insert_or_assign(scheme, handler);
    return true;
  }

  bool SchemeHandlers::handleRequest (
    SharedPointer<Request> request,
    const HandlerCallback callback
  ) {
    // request was not finalized, likely not from a `Request::Builder`
    if (request == nullptr || !request->finalized) {
      return false;
    }

    if (this->isRequestActive(request->id)) {
      return false;
    }

    // respond with a 404 if somehow we are trying to respond to a
    // request scheme we do not know about, we do not need to call
    // `request.finalize()` as we'll just respond to the request right away
    if (!this->hasHandlerForScheme(request->scheme)) {
      auto response = webview::SchemeHandlers::Response(request, 404);
      // make sure the response was finished first
      response.finish();

      // notify finished, even for 404
      if (response.request->callbacks.finish != nullptr) {
        response.request->callbacks.finish();
      }

      if (callback != nullptr) {
        callback(response);
      }

      return true;
    }

    const auto handler = this->getHandlerForScheme(request->scheme);
    const auto id = request->id;

    // fail if there is somehow not a handler for this request scheme
    if (handler == nullptr) {
      return false;
    }

    do {
      Lock lock(this->mutex);
      this->activeRequests.emplace(request->id, request);
    } while (0);

    if (request->error != nullptr) {
      auto response = webview::SchemeHandlers::Response(request, 500);
      response.fail(request->error);
      do {
        Lock lock(this->mutex);
        this->activeRequests.erase(id);
      } while (0);
      return true;
    }

    auto span = request->tracer.span("handler");

    this->bridge->dispatch([this, id, span, handler, callback, request] () mutable {
      if (request != nullptr && request->isActive() && !request->isCancelled()) {
        handler(request, this->bridge, &request->callbacks, [this, id, span, callback](auto& response) mutable {
          // make sure the response was finished before
          // calling the `callback` function below
          response.finish();

          // notify finished
          if (response.request->callbacks.finish != nullptr) {
            response.request->callbacks.finish();
          }

          if (callback != nullptr) {
            callback(response);
          }

          span->end();

          do {
            Lock lock(this->mutex);
            this->activeRequests.erase(id);
          } while (0);
        });
      }
    });

    return true;
  }

  bool SchemeHandlers::isRequestActive (uint64_t id) {
    Lock lock(this->mutex);
    return this->activeRequests.contains(id);
  }

  bool SchemeHandlers::isRequestCancelled (uint64_t id) {
    Lock lock(this->mutex);
    return (
      id > 0 &&
      this->activeRequests.contains(id) &&
      this->activeRequests.at(id) != nullptr &&
      this->activeRequests.at(id)->cancelled
    );
  }

  SchemeHandlers::Request::Builder::Builder (
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    SchemeHandlers* handlers,
    PlatformRequest platformRequest,
    ICoreWebView2Environment* env
  #else
    SchemeHandlers* handlers,
    PlatformRequest platformRequest
  #endif
  ) {
    const auto userConfig = handlers->bridge->userConfig;
    const auto bundleIdentifier = userConfig.contains("meta_bundle_identifier")
      ? userConfig.at("meta_bundle_identifier")
      : "";

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    this->absoluteURL = platformRequest.request.URL.absoluteString.UTF8String;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    this->absoluteURL = webkit_uri_scheme_request_get_uri(platformRequest);
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    LPWSTR requestURI;
    platformRequest->get_Uri(&requestURI);
    this->absoluteURL = convertWStringToString(requestURI);
    if (
      this->absoluteURL.starts_with("socket://") &&
      !this->absoluteURL.starts_with("socket://" + bundleIdentifier) &&
      !this->absoluteURL.starts_with("socket://" + toLowerCase(bundleIdentifier))
    ) {
      this->absoluteURL = String("socket://") + this->absoluteURL.substr(8);
      if (this->absoluteURL.ends_with("/")) {
        this->absoluteURL = this->absoluteURL.substr(0, this->absoluteURL.size() - 1);
      }
    }
    CoTaskMemFree(requestURI);
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    auto app = app::App::sharedApplication();
    auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
    this->absoluteURL = android::StringWrap(attachment.env, (jstring) CallClassMethodFromAndroidEnvironment(
      attachment.env,
      Object,
      platformRequest,
      "getUrl",
      "()Ljava/lang/String;"
    )).str();
  #endif

    const auto url = URL::Components::parse(this->absoluteURL);

    this->request = std::make_shared<Request>(
      handlers,
      platformRequest,
      Request::Options {
        .scheme = url.scheme
      }
    );

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    this->request->env = env;
  #endif

    this->request->client = handlers->bridge->client;

    // build request URL components from parsed URL components
    this->request->originalURL = this->absoluteURL;
    this->request->hostname = url.authority;
    this->request->pathname = url.pathname;
    this->request->query = url.query;
    this->request->fragment = url.fragment;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setScheme (const String& scheme) {
    this->request->scheme = scheme;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setMethod (const String& method) {
    this->request->method = method;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHostname (const String& hostname) {
    this->request->hostname = hostname;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setPathname (const String& pathname) {
    this->request->pathname = pathname;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setQuery (const String& query) {
    this->request->query = query;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setFragment (const String& fragment) {
    this->request->fragment = fragment;
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeader (
    const String& name,
    const Headers::Value& value
  ) {
    this->request->headers.set(name, value.string);
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (const Headers& headers) {
    for (const auto& entry : headers) {
      this->request->headers.set(entry);
    }
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (
    const Map<String, String>& headers
  ) {
    for (const auto& entry : headers) {
      this->request->headers.set(entry.first, entry.second);
    }
    return *this;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (
    const NSDictionary<NSString*, NSString*>* headers
  ) {
    if (headers == nullptr) {
      return *this;
    }

    for (NSString* key in headers) {
      const auto value = [headers objectForKey: key];
      if (value != nullptr) {
        this->request->headers.set(key.UTF8String, value.UTF8String);
      }
    }

    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (const NSData* data) {
    if (data != nullptr && data.length > 0 && data.bytes != nullptr) {
      return this->setBody(data.length, reinterpret_cast<const unsigned char*>(data.bytes));
    }
    return *this;
  }
#elif SOCKET_RUNTIME_PLATFORM_LINUX
  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (
    const SoupMessageHeaders* headers
  ) {
    if (headers) {
      soup_message_headers_foreach(
        const_cast<SoupMessageHeaders*>(headers),
        [](const char* name, const char* value, gpointer userData) {
          auto request = reinterpret_cast<SchemeHandlers::Request*>(userData);
          request->headers.set(name, value);
        },
        this->request.get()
      );
    }

    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (
    GInputStream* stream
  ) {
    if (stream == nullptr) {
      return *this;
    }

    if (
      this->request->method == "POST" ||
      this->request->method == "PUT" ||
      this->request->method == "PATCH"
    ) {
      GError* error = nullptr;
      unsigned char tmp[MAX_URI_SCHEME_REQUEST_BODY_BYTES] = {0};
      size_t size = 0;
      const auto success = g_input_stream_read_all(
        stream,
        reinterpret_cast<gchar*>(tmp),
        MAX_URI_SCHEME_REQUEST_BODY_BYTES,
        &size,
        nullptr,
        &this->error
      );
      this->request->body = bytes::Buffer::from(tmp, size);
    }
    return *this;
  }
#endif

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (const bytes::Buffer& body) {
    if (
      this->request->method == "POST" ||
      this->request->method == "PUT" ||
      this->request->method == "PATCH"
    ) {
      this->request->body = body;
    }
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (size_t size, const unsigned char* bytes) {
    if (this->request->method == "POST" || this->request->method == "PUT" || this->request->method == "PATCH") {
      if (size > 0 && bytes != nullptr) {
        this->request->body = bytes::Buffer::from(bytes, size);
      }
    }
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setCallbacks (const RequestCallbacks& callbacks) {
    this->request->callbacks = callbacks;
    return *this;
  }

  SharedPointer<SchemeHandlers::Request> SchemeHandlers::Request::Builder::build () {
    this->request->error = this->error;
    this->request->finalize();
    return this->request;
  }

  SchemeHandlers::Request::Request (
    SchemeHandlers* handlers,
    PlatformRequest platformRequest,
    const Options& options
  )
    : handlers(handlers),
      bridge(handlers->bridge),
      scheme(options.scheme),
      method(options.method),
      hostname(options.hostname),
      pathname(options.pathname),
      query(options.query),
      fragment(options.fragment),
      headers(options.headers),
      tracer("webview::SchemeHandlers::Request")
  {
    this->platformRequest = platformRequest;
  }

  SchemeHandlers::Request::~Request () {
    this->platformRequest = nullptr;
  }

  static void copyRequest (
    SchemeHandlers::Request* destination,
    const SchemeHandlers::Request& source
  ) noexcept {
    destination->id = source.id;
    destination->scheme = source.scheme;
    destination->method = source.method;
    destination->hostname = source.hostname;
    destination->pathname = source.pathname;
    destination->query = source.query;
    destination->fragment = source.fragment;

    destination->headers = source.headers;
    destination->body = source.body;

    destination->client = source.client;
    destination->callbacks = source.callbacks;
    destination->originalURL = source.originalURL;

    if (destination->finalized) {
      destination->origin = source.origin;
      destination->params = source.params;
    }

    destination->finalized = source.finalized.load();
    destination->cancelled  = source.cancelled.load();

    destination->bridge = source.bridge;
    destination->tracer = source.tracer;
    destination->handlers = source.handlers;
    destination->platformRequest = source.platformRequest;

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    destination->env = source.env;
  #endif
  }

  SchemeHandlers::Request::Request (const Request& request) noexcept
    : tracer("webview::SchemeHandlers::Request")
  {
    copyRequest(this, request);
  }

  SchemeHandlers::Request::Request (Request&& request) noexcept
    : tracer("SchemeHandlers::Request")
  {
    copyRequest(this, request);
  }

  SchemeHandlers::Request& SchemeHandlers::Request::operator= (const Request& request) noexcept {
    copyRequest(this, request);
    return *this;
  }

  SchemeHandlers::Request& SchemeHandlers::Request::operator= (Request&& request) noexcept {
    copyRequest(this, request);
    return *this;
  }

  bool SchemeHandlers::Request::hasHeader (const String& name) const {
    if (this->headers.has(name)) {
      return true;
    }

    return false;
  }

  const String SchemeHandlers::Request::getHeader (const String& name) const {
    return this->headers.get(name).value.string;
  }

  const String SchemeHandlers::Request::url () const {
    return this->str();
  }

  const String SchemeHandlers::Request::str () const {
    if (this->hostname.size() > 0) {
      return trim(
        this->scheme +
        "://" +
        this->hostname +
        this->pathname +
        (this->query.size() ? "?" + this->query : "") +
        (this->fragment.size() ? "#" + this->fragment : "")
      );
    }

    return trim(
      this->scheme +
      ":" +
      this->pathname.substr(1) +
      (this->query.size() ? "?" + this->query : "") +
      (this->fragment.size() ? "#" + this->fragment : "")
    );
  }

  bool SchemeHandlers::Request::finalize () {
    if (this->finalized) {
      return false;
    }

    if (this->hasHeader("runtime-client-id")) {
      try {
        this->client.id = std::stoull(this->getHeader("runtime-client-id"));
      } catch (...) {}
    }

    for (const auto& entry : split(this->query, '&')) {
      const auto parts = split(entry, '=');
      if (parts.size() == 2) {
        const auto key = decodeURIComponent(trim(parts[0]));
        const auto value = decodeURIComponent(trim(parts[1]));
        this->params.insert_or_assign(key, value);
      }
    }

    this->finalized = true;
    this->origin = this->scheme + "://" + this->hostname;
    return true;
  }

  bool SchemeHandlers::Request::isActive () const {
    auto app = app::App::sharedApplication();
    auto window = app->runtime.windowManager.getWindowForBridge(
      dynamic_cast<window::IBridge*>(this->bridge)
    );

    // only a scheme handler owned by this bridge and attached to a
    // window should be considered "active"
    // scheme handlers SHOULD only work windows that have a navigator
    if (window != nullptr && this->handlers != nullptr) {
      return this->handlers->isRequestActive(this->id);
    }

    return false;
  }

  bool SchemeHandlers::Request::isCancelled () const {
    auto app = app::App::sharedApplication();
    auto window = app->runtime.windowManager.getWindowForBridge(
      dynamic_cast<window::IBridge*>(this->bridge)
    );

    if (window != nullptr && this->handlers != nullptr) {
      return this->handlers->isRequestCancelled(this->id);
    }

    return false;
  }

  JSON::Object SchemeHandlers::Request::json () const {
    return JSON::Object::Entries {
      {"scheme", this->scheme},
      {"method", this->method},
      {"hostname", this->hostname},
      {"pathname", this->pathname},
      {"query", this->query},
      {"fragment", this->fragment},
      {"headers", this->headers.json()},
      {"client", JSON::Object::Entries {
        {"id", this->client.id}
      }}
    };
  }

  SchemeHandlers::Response::Response (
    SharedPointer<Request> request,
    int statusCode,
    const Headers headers
  ) : request(request),
      handlers(request->handlers),
      client(request->client),
      id(request->id),
      tracer("webview::SchemeHandlers::Response")
  {
    const auto defaultHeaders = split(
      this->request->bridge->userConfig.contains("webview_headers")
        ? this->request->bridge->userConfig.at("webview_headers")
        : "",
      '\n'
    );

    if (isDebugEnabled()) {
      this->setHeader("cache-control", "no-cache");
    }

    this->setHeader("connection", "keep-alive");
    this->setHeader("access-control-allow-origin", "*");
    this->setHeader("access-control-allow-headers", "*");
    this->setHeader("access-control-allow-methods", "*");
    this->setHeader("access-control-allow-credentials", "true");

    if (request->method == "OPTIONS") {
      this->setHeader("allow", "GET, POST, PATCH, PUT, DELETE, HEAD");
    }

    for (const auto& entry : defaultHeaders) {
      const auto parts = split(trim(entry), ':');
      this->setHeader(parts[0], parts[1]);
    }
  }

  static void copyResponse (
    SchemeHandlers::Response* destination,
    const SchemeHandlers::Response& source
  ) noexcept {
    destination->id = source.id;
    destination->client = source.client;
    destination->headers = source.headers;
    destination->finished = source.finished.load();
    destination->handlers = source.handlers;
    destination->statusCode = source.statusCode;
    destination->buffers = source.buffers;
    destination->platformResponse = source.platformResponse;
  }

  SchemeHandlers::Response::Response (const Response& response) noexcept
    : request(response.request),
      tracer("SchemeHandlers::Response")
  {
    copyResponse(this, response);
  }

  SchemeHandlers::Response::Response (Response&& response) noexcept
    : request(response.request),
      tracer("SchemeHandlers::Response")
  {
    copyResponse(this, response);
  }

  SchemeHandlers::Response::~Response () {}

  SchemeHandlers::Response& SchemeHandlers::Response::operator= (const Response& response) noexcept {
    copyResponse(this, response);
    return *this;
  }

  SchemeHandlers::Response& SchemeHandlers::Response::operator= (Response&& response) noexcept {
    copyResponse(this, response);
    return *this;
  }

  bool SchemeHandlers::Response::writeHead (int statusCode, const Headers headers) {
    // fail if already finished
    if (this->finished) {
      debug("SchemeHandlers::Response: Failed to write head. Already finished");
      return false;
    }

    if (
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      return false;
    }

    // fail if head of response is already created
    if (this->platformResponse != nullptr) {
      debug("SchemeHandlers::Response: Failed to write head as it was already written");
      return false;
    }

    if (this->request->platformRequest == nullptr) {
      debug("SchemeHandlers::Response: Failed to write head. Request is in an invalid state");
      return false;
    }

    if (statusCode >= 100 && statusCode < 600) {
      this->statusCode = statusCode;
    }

    for (const auto& header : headers) {
      this->setHeader(header);
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE || SOCKET_RUNTIME_PLATFORM_LINUX || SOCKET_RUNTIME_PLATFORM_ANDROID
    // webkit status codes cannot be in the range of 300 >= statusCode < 400
    if (this->statusCode >= 300 && this->statusCode < 400) {
      this->statusCode = 200;
    }
  #endif

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    auto headerFields= [NSMutableDictionary dictionary];
    for (const auto& entry : this->headers) {
      headerFields[@(entry.name.c_str())] = @(entry.value.c_str());
    }

    auto platformRequest = this->request->platformRequest;
    if (platformRequest != nullptr && platformRequest.request != nullptr) {
      const auto url = platformRequest.request.URL;
      if (url != nullptr) {
        @try {
          this->platformResponse = [[NSHTTPURLResponse alloc]
            initWithURL: platformRequest.request.URL
             statusCode: this->statusCode
            HTTPVersion: @"HTTP/1.1"
           headerFields: headerFields
          ];
        } @catch (::id) {
          return false;
        }

        [platformRequest didReceiveResponse: this->platformResponse];
        return true;
      }
    }
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    const auto contentLength = this->getHeader("content-length");
    gint64 size = -1;

    if (contentLength.size() > 0) {
      try {
       size = std::stol(contentLength);
      } catch (...) {}
    }

    if (this->platformResponseStream == nullptr) {
      this->platformResponseStream = g_memory_input_stream_new();
    }

    this->platformResponse = webkit_uri_scheme_response_new(this->platformResponseStream, size);

    auto requestHeaders = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
    for (const auto& entry : this->headers) {
      soup_message_headers_append(requestHeaders, entry.name.c_str(), entry.value.c_str());
    }

    webkit_uri_scheme_response_set_http_headers(this->platformResponse, requestHeaders);

    if (this->hasHeader("content-type")) {
      const auto contentType = this->getHeader("content-type");
      if (contentType.size() > 0) {
        webkit_uri_scheme_response_set_content_type(this->platformResponse, contentType.c_str());
      }
    }

    const auto statusText = http::getStatusText(this->statusCode);

    webkit_uri_scheme_response_set_status(
      this->platformResponse,
      this->statusCode,
      statusText.size() > 0 ? statusText.c_str() : nullptr
    );

    return true;
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    const auto statusText = http::getStatusText(this->statusCode);
    this->platformResponseStream = SHCreateMemStream(nullptr, 0);
    const auto result = this->request->env->CreateWebResourceResponse(
      this->platformResponseStream,
      this->statusCode,
      convertStringToWString(statusText).c_str(),
      convertStringToWString(this->headers.str()).c_str(),
      &this->platformResponse
    );

    return result == S_OK;
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    const auto app = app::App::sharedApplication();
    const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);

    this->platformResponse = attachment.env->NewGlobalRef(
      CallClassMethodFromAndroidEnvironment(
        attachment.env,
        Object,
        this->request->platformRequest,
        "getResponse",
        "()Lsocket/runtime/ipc/SchemeHandlers$Response;"
      )
    );

    for (const auto& header : this->headers) {
      const auto name = attachment.env->NewStringUTF(toHeaderCase(header.name).c_str());
      const auto value = attachment.env->NewStringUTF(header.value.c_str());

      CallVoidClassMethodFromAndroidEnvironment(
        attachment.env,
        this->platformResponse,
        "setHeader",
        "(Ljava/lang/String;Ljava/lang/String;)V",
        name,
        value
      );

      attachment.env->DeleteLocalRef(name);
      attachment.env->DeleteLocalRef(value);
    }

    const auto statusText = attachment.env->NewStringUTF(http::getStatusText(this->statusCode).c_str());

    CallVoidClassMethodFromAndroidEnvironment(
      attachment.env,
      this->platformResponse,
      "setStatus",
      "(ILjava/lang/String;)V",
      this->statusCode,
      statusText
    );

    attachment.env->DeleteLocalRef(statusText);

    return true;
  #endif
    return false;
  }

  bool SchemeHandlers::Response::write (const bytes::Buffer& buffer) {
    return this->write(buffer.size(), buffer.pointer());
  }

  bool SchemeHandlers::Response::write (
    size_t size,
    SharedPointer<unsigned char[]> bytes
  ) {
    if (
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      debug("SchemeHandlers::Response: Write attemped for request that is no longer active or cancelled");
      return false;
    }

    if (!this->hasHeader("content-type")) {
      this->setHeader("content-type", "application/octet-stream");
    }

    if (!this->platformResponse) {
      // set 'content-length' header if response was not created
      this->setHeader("content-length", size);
      if (!this->writeHead()) {
        debug(
          "SchemeHandlers::Response: Failed to write head for %s",
          this->request->str().c_str()
        );
        return false;
      }
    }

    if (size > 0 && bytes != nullptr) {
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      const auto data = [NSData dataWithBytes: bytes.get() length: size];
      @try {
        [this->request->platformRequest didReceiveData: data];
      } @catch (::id) {
        return false;
      }
      return true;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      const auto tmp = new unsigned char[size]{0};
      memcpy(tmp, bytes.get(), size);
      g_memory_input_stream_add_data(
        reinterpret_cast<GMemoryInputStream*>(this->platformResponseStream),
        reinterpret_cast<const void*>(tmp),
        (gssize) size,
        [](auto p) {
          const auto tmp = reinterpret_cast<unsigned char*>(p);
          delete [] tmp;
        }
      );
      return true;
    #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
      return S_OK == this->platformResponseStream->Write(
        reinterpret_cast<const void*>(bytes.get()),
        (ULONG) size,
        nullptr
      );
    #elif SOCKET_RUNTIME_PLATFORM_ANDROID
      const auto app = app::App::sharedApplication();
      const auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);
      const auto byteArray = attachment.env->NewByteArray(size);

      attachment.env->SetByteArrayRegion(
        byteArray,
        0,
        size,
        (jbyte *) bytes.get()
      );

      CallVoidClassMethodFromAndroidEnvironment(
        attachment.env,
        this->platformResponse,
        "write",
        "([B)V",
        byteArray
      );

      if (byteArray != nullptr) {
        attachment.env->DeleteLocalRef(byteArray);
      }

      return true;
    #endif
    }

    return false;
  }

  bool SchemeHandlers::Response::write (const String& source) {
    const auto size = source.size();

    if (size == 0) {
      return false;
    }

    auto bytes = std::make_shared<unsigned char[]>(size);
    memset(bytes.get(), 0, size);
    memcpy(bytes.get(), source.data(), size);
    return this->write(size, bytes);
  }

  bool SchemeHandlers::Response::write (size_t size, const unsigned char* input) {
    if (size == 0) {
      return false;
    }

    auto bytes = std::make_shared<unsigned char[]>(size);
    memcpy(bytes.get(), input, size);
    return this->write(size, bytes);
  }

  bool SchemeHandlers::Response::write (const JSON::Any& json) {
    this->setHeader("content-type", "application/json");
    return this->write(json.str());
  }

  bool SchemeHandlers::Response::write (const filesystem::Resource& resource) {
    auto responseResource = filesystem::Resource(resource);
    auto app = app::App::sharedApplication();

    const auto contentLength = responseResource.size();
    const auto contentType = responseResource.mimeType();

    if (contentType.size() > 0 && !this->hasHeader("content-type")) {
      this->setHeader("content-type", contentType);
    }

    if (contentLength > 0) {
      this->setHeader("content-length", contentLength);
    }

    if (contentLength > 0) {
      this->writeHead();
      return this->write(contentLength, responseResource.read());
    }

    return false;
  }

  bool SchemeHandlers::Response::write (
    const filesystem::Resource::ReadStream::Buffer& buffer
  ) {
    return this->write(buffer.size, buffer.bytes);
  }

  bool SchemeHandlers::Response::send (const String& source) {
    return this->write(source) && this->finish();
  }

  bool SchemeHandlers::Response::send (const JSON::Any& json) {
    return this->write(json) && this->finish();
  }

  bool SchemeHandlers::Response::send (const filesystem::Resource& resource) {
    return this->write(resource) && this->finish();
  }

  bool SchemeHandlers::Response::finish () {
    // fail if already finished
    if (this->finished) {
      return false;
    }

    if (
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      return false;
    }

    if (!this->platformResponse) {
      if (!this->writeHead() || !this->platformResponse) {
        return false;
      }
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    @try {
      [this->request->platformRequest didFinish];
    } @catch (::id) {}
  #if !__has_feature(objc_arc)
    [this->platformResponse release];
  #endif
    this->platformResponse = nullptr;
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    if (this->request->platformRequest) {
      auto platformRequest = this->request->platformRequest;
      auto platformResponse = this->platformResponse;
      auto platformResponseStream = this->platformResponseStream;

      this->platformResponseStream = nullptr;
      this->platformResponse = nullptr;

      webkit_uri_scheme_request_finish_with_response(
        platformRequest,
        platformResponse
      );

      g_object_unref(platformResponseStream);
      g_input_stream_close_async(
        platformResponseStream,
        G_PRIORITY_DEFAULT,
        nullptr,
        [](auto stream, auto res, auto _) {
          g_object_unref(stream);
        },
        nullptr
      );
    }
  #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
    this->platformResponseStream = nullptr;
    // TODO(@jwerle): move more `WebResourceRequested` logic to here
  #elif SOCKET_RUNTIME_PLATFORM_ANDROID
    if (this->platformResponse != nullptr) {
      auto app = app::App::sharedApplication();
      auto attachment = android::JNIEnvironmentAttachment(app->runtime.android.jvm);

      CallVoidClassMethodFromAndroidEnvironment(
        attachment.env,
        this->platformResponse,
        "finish",
        "()V"
      );

      this->platformResponse = nullptr;
      attachment.env->DeleteGlobalRef(this->platformResponse);
    }
  #else
    this->platformResponse = nullptr;
  #endif

    this->finished = true;
    return true;
  }

  void SchemeHandlers::Response::setHeader (const String& name, const Headers::Value& value) {
    auto app = App::sharedApplication();
    const auto bridge = this->request->handlers->bridge;
    if (toLowerCase(name) == "referer") {
      if (bridge->navigator.location.workers.contains(value.string)) {
        const auto workerLocation = bridge->navigator.location.workers[value.string];
        this->headers[name] = workerLocation;
        return;
      } else {
        for (const auto& entry : app->runtime.serviceWorkerManager.servers) {
          if (entry.second->bridge->navigator.location.workers.contains(value.string)) {
            const auto workerLocation = entry.second->bridge->navigator.location.workers[value.string];
            this->headers[name] = workerLocation;
            return;
          }
        }
      }
    }

    this->headers[name] = value.string;
  }

  void SchemeHandlers::Response::setHeader (const Headers::Header& header) {
    this->setHeader(header.name, header.value.string);
  }

  void SchemeHandlers::Response::setHeader (const String& name, size_t value) {
    this->setHeader(name, std::to_string(value));
  }

  void SchemeHandlers::Response::setHeader (const String& name, int64_t value) {
    this->setHeader(name, std::to_string(value));
  }

#if !SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_PLATFORM_ANDROID && !SOCKET_RUNTIME_PLATFORM_WINDOWS
  void SchemeHandlers::Response::setHeader (const String& name, uint64_t value) {
    this->setHeader(name, std::to_string(value));
  }
#endif

  void SchemeHandlers::Response::setHeaders (const Headers& headers) {
    for (const auto& header : headers) {
      this->setHeader(header);
    }
  }

  const String SchemeHandlers::Response::getHeader (const String& name) const {
    return this->headers.get(name).value.string;
  }

  bool SchemeHandlers::Response::hasHeader (const String& name) const {
    return this->headers.has(name);
  }

  bool SchemeHandlers::Response::fail (const Error* error) {
  #if SOCKET_RUNTIME_PLATFORM_APPLE
    if (error.localizedDescription != nullptr) {
      return this->fail(error.localizedDescription.UTF8String);
    } else if (error.localizedFailureReason != nullptr) {
      return this->fail(error.localizedFailureReason.UTF8String);
    }
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    if (error != nullptr && error->message != nullptr) {
      return this->fail(error->message);
    }
  #endif

    return this->fail("Request failed for an unknown reason");
  }

  bool SchemeHandlers::Response::fail (const String& reason) {
    const auto bundleIdentifier = this->request->bridge->userConfig.contains("meta_bundle_identifier")
      ? this->request->bridge->userConfig.at("meta_bundle_identifier")
      : "";

    if (
      this->finished ||
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      return false;
    }

  #if SOCKET_RUNTIME_PLATFORM_APPLE
    const auto error = [NSError
      errorWithDomain: @(bundleIdentifier.c_str())
                code: 1
            userInfo: @{NSLocalizedDescriptionKey: @(reason.c_str())}
    ];

    @try {
      [this->request->platformRequest didFailWithError: error];
    } @catch (::id e) {
      // ignore possible 'NSInternalInconsistencyException'
      return false;
    }

    // notify fail callback
    if (this->request->callbacks.fail != nullptr) {
      this->request->callbacks.fail(error);
    }
  #elif SOCKET_RUNTIME_PLATFORM_LINUX
    const auto quark = g_quark_from_string(bundleIdentifier.c_str());
    if (!quark) {
      return false;
    }

    const auto error = g_error_new(quark, 1, "%s", reason.c_str());

    if (error == nullptr) {
      return false;
    }

    if (this->request && WEBKIT_IS_URI_SCHEME_REQUEST(this->request->platformRequest)) {
      webkit_uri_scheme_request_finish_error(this->request->platformRequest, error);
    } else {
      return false;
    }

    // notify fail callback
    if (this->request->callbacks.fail != nullptr) {
      this->request->callbacks.fail(error);
    }
  #else
    // XXX(@jwerle): there doesn't appear to be a way to notify a failure for all platforms
    this->finished = true;
    return false;
  #endif

    this->finished = true;
    return true;
  }

  bool SchemeHandlers::Response::redirect (const String& location, int statusCode) {
    static constexpr auto redirectSourceTemplate = R"S(
      <meta http-equiv="refresh" content="0; url='{{url}}'" />
    )S";

    // if head was already written, then we cannot perform a redirect
    if (this->platformResponse) {
      return false;
    }

    if (location.starts_with("/")) {
      this->setHeader("location", this->request->origin + location);
    } else if (location.starts_with(".")) {
      this->setHeader("location", this->request->origin + location.substr(1));
    } else {
      this->setHeader("location", location);
    }

    if (!this->writeHead(statusCode)) {
      return false;
    }

    if (this->request->method != "HEAD" && this->request->method != "OPTIONS") {
      const auto content = tmpl(
        redirectSourceTemplate,
        {{"url", location}}
      );

      if (!this->write(content)) {
        return false;
      }
    }

    return this->finish();
  }

  const String SchemeHandlers::Response::Event::str () const noexcept {
    if (this->name.size() > 0 && this->data.size() > 0) {
      return (
        String("event: ") + this->name + "\n" +
        String("data: ") + this->data + "\n"
        "\n"
      );
    }

    if (this->name.size() > 0) {
      return  String("event: ") + this->name + "\n\n";
    }

    if (this->data.size() > 0) {
      return  String("data: ") + this->data + "\n\n";
    }

    return "";
  }

  size_t SchemeHandlers::Response::Event::count () const noexcept {
    if (this->name.size() > 0 && this->data.size() > 0) {
      return 2;
    } else if (this->name.size() > 0 || this->data.size() > 0) {
      return 1;
    } else {
      return 0;
    }
  }
}
