#include "scheme_handlers.hh"
#include "ipc.hh"

using namespace SSC;
using namespace SSC::IPC;

namespace SSC::IPC {
  static struct {
    SchemeHandlers::RequestMap map;
    Mutex mutex;
  } requests;
}

#if SSC_PLATFORM_APPLE
using Task = id<WKURLSchemeTask>;

@class SSCBridgedWebView;
@interface SSCInternalWKURLSchemeHandler : NSObject<WKURLSchemeHandler>
@property (nonatomic) SSC::IPC::SchemeHandlers* handlers;

-     (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (id<WKURLSchemeTask>) task;

-    (void) webView: (SSCBridgedWebView*) webview
  stopURLSchemeTask: (id<WKURLSchemeTask>) task;
@end

@implementation SSCInternalWKURLSchemeHandler
{
  Mutex mutex;
  std::unordered_map<Task, uint64_t> tasks;
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

-    (void) webView: (SSCBridgedWebView*) webview
  stopURLSchemeTask: (Task) task
{
  Lock lock(requests.mutex);
  if (tasks.contains(task)) {
    const auto id = tasks[task];
    if (requests.map.contains(id)) {
      auto& request = requests.map.at(id);
      request.cancelled = true;
      if (request.callbacks.cancel != nullptr) {
        request.callbacks.cancel();
      }
    }
  }

  [self finalizeTask: task];
}

-     (void) webView: (SSCBridgedWebView*) webview
  startURLSchemeTask: (Task) task
{
  if (self.handlers == nullptr) {
    static auto userConfig = SSC::getUserConfig();
    const auto bundleIdentifier = userConfig.contains("meta_bundle_identifier")
      ? userConfig.at("meta_bundle_identifier")
      : "";

    [task didFailWithError: [NSError
         errorWithDomain: @(bundleIdentifier.c_str())
                    code: 1
                userInfo: @{NSLocalizedDescriptionKey: @("Request is invalid state")}
    ]];
    return;
  }

  auto request = IPC::SchemeHandlers::Request::Builder(self.handlers, task)
    .setMethod(toUpperCase(task.request.HTTPMethod.UTF8String))
    // copies all headers
    .setHeaders(task.request.allHTTPHeaderFields)
    // copies request body
    .setBody(task.request.HTTPBody)
    .build();

  [self enqueueTask: task withRequestID: request.id];
  const auto handled = self.handlers->handleRequest(request, [=](const auto& response) {
    [self finalizeTask: task];
  });

  if (!handled) {
    auto response = IPC::SchemeHandlers::Response(request, 404);
    response.finish();
    [self finalizeTask: task];
    return;
  }
}
@end
#elif SSC_PLATFORM_LINUX
static const auto MAX_URI_SCHEME_REQUEST_BODY_BYTES = 4 * 1024 * 1024;
static void onURISchemeRequest (WebKitURISchemeRequest* schemeRequest, gpointer userData) {
  static auto windowManager = App::instance()->getWindowManager();
  static auto userConfig = SSC::getUserConfig();
  auto webview = webkit_uri_scheme_request_get_web_view(schemeRequest);

  if (!windowManager) {
    const auto quark = g_quark_from_string(userConfig["meta_bundle_identifier"].c_str());
    const auto error = g_error_new(quark, 1, "Missing WindowManager in request");
    webkit_uri_scheme_request_finish_error(schemeRequest, error);
    return;
  }

  IPC::Router* router = nullptr;

  for (auto& window : windowManager->windows) {
    if (
      window != nullptr &&
      window->bridge != nullptr &&
      WEBKIT_WEB_VIEW(window->webview) == webview
    ) {
      router = &window->bridge->router;
      break;
    }
  }

  if (!router) {
    const auto quark = g_quark_from_string(userConfig["meta_bundle_identifier"].c_str());
    const auto error = g_error_new(quark, 1, "Missing IPC bridge router in request");
    webkit_uri_scheme_request_finish_error(schemeRequest, error);
    return;
  }

  auto request = IPC::SchemeHandlers::Request::Builder(router->schemeHandlers, schemeRequest)
    .setMethod(String(webkit_uri_scheme_request_get_http_method(platformRequest)))
    // copies all request soup headers
    .setHeaders(webkit_uri_scheme_request_get_http_headers(request))
    // reads and copies request stream body
    .setBody(webkit_uri_scheme_request_get_http_body(schemeRequest))
    .build();

  const auto handled = router->schemeHandlers.handleRequest(request, [=](const auto response) {
    // TODO(@jwerle): handle uri scheme response
  });

  if (!handled) {
    auto response = IPC::SchemeHandlers::Response(request, 404);
    callback(response);
    return;
  }
}

#endif

namespace SSC::IPC {
  class SchemeHandlersInternals {
    public:
      SchemeHandlers* handlers = nullptr;
    #if SSC_PLATFORM_APPLE
      SSCInternalWKURLSchemeHandler* schemeHandler = nullptr;
    #endif

      SchemeHandlersInternals (SchemeHandlers* handlers) {
        this->handlers = handlers;
      #if SSC_PLATFORM_APPLE
        this->schemeHandler = [SSCInternalWKURLSchemeHandler new];
        this->schemeHandler.handlers = handlers;
      #endif
      }

      ~SchemeHandlersInternals () {
      #if SSC_PLATFORM_APPLE
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

namespace SSC::IPC {
#if SSC_PLATFORM_LINUX
  static Set<String> globallyRegisteredSchemesForLinux;
#endif

  static const std::map<int, String> STATUS_CODES = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a Teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {509, "Bandwidth Limit Exceeded"},
    {510, "Not Extended"},
{511, "Network Authentication Required"}
  };

  SchemeHandlers::SchemeHandlers (Router* router) {
    this->router = router;
    this->internals = new SchemeHandlersInternals(this);
  }

  SchemeHandlers::~SchemeHandlers () {
  #if SSC_PLATFORM_APPLE
    this->configuration.webview = nullptr;
  #endif

    if (this->internals != nullptr) {
      delete this->internals;
      this->internals = nullptr;
    }
  }

  void SchemeHandlers::configure (const Configuration& configuration) {
    static const auto devHost = SSC::getDevHost();
    this->configuration = configuration;
  #if SSC_PLATFORM_APPLE
    if (SSC::isDebugEnabled() && devHost.starts_with("http:")) {
      [configuration.webview.processPool
        performSelector: @selector(_registerURLSchemeAsSecure:)
        withObject: @"http"
      ];
    }
  #elif SSC_PLATFORM_LINUX
    if (SSC::isDebugEnabled() && devHost.starts_with("http:")) {
      auto webContext = webkit_web_context_get_default();
      auto security = webkit_web_context_get_security_manager(webContext);

      webkit_security_manager_register_uri_scheme_as_display_isolated(security, "http");
      webkit_security_manager_register_uri_scheme_as_cors_enabled(security, "http");
      webkit_security_manager_register_uri_scheme_as_secure(security, "http");
      webkit_security_manager_register_uri_scheme_as_local(security, "http");
    }
  #endif
  }

  bool SchemeHandlers::hasHandlerForScheme (const String& scheme) {
    return this->handlers.contains(scheme);
  }

  bool SchemeHandlers::registerSchemeHandler (const String& scheme, const Handler& handler) {
    if (scheme.size() == 0 || this->hasHandlerForScheme(scheme)) {
      return false;
    }

  #if SSC_PLATFORM_APPLE
    if (this->configuration.webview == nullptr) {
      return false;
    }

    [this->configuration.webview
      setURLSchemeHandler: this->internals->schemeHandler
             forURLScheme: @(scheme.c_str())
    ];

  #elif SSC_PLATFORM_LINUX
    // schemes are registered for the globally shared defaut context,
    // despite the `SchemeHandlers` instance bound to a `Router`
    // we'll select the correct `Router` in the callback which will give
    // access to the `SchemeHandlers` that should handle the
    // request and provide a response
    if (
      std::find(
        globallyRegisteredSchemesForLinux.begin(),
        globallyRegisteredSchemesForLinux.end(),
        scheme
      ) != globallyRegisteredSchemesForLinux.end()
    ) {
      globallyRegisteredSchemesForLinux.insert(scheme);
      auto context = webkit_web_context_get_default();
      auto security = webkit_web_context_get_security_manager(context);
      webkit_web_context_register_uri_scheme(
        context,
        scheme.c_str(),
        onURISchemeRequest
        nullptr,
        nullptr
      );
    }
  #elif SSC_PLATFORM_WINDOWS
    static const int MAX_ALLOWED_SCHEME_ORIGINS = 64;
    static const int MAX_CUSTOM_SCHEME_REGISTRATIONS = 64;

    auto registration = Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(
      convertStringToWString(scheme).c_str()
    );

    Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options;
    ICoreWebView2CustomSchemeRegistration* registrations[MAX_CUSTOM_SCHEME_REGISTRATIONS] = {};
    const WCHAR* allowedOrigins[MAX_ALLOWED_SCHEME_ORIGINS] = {};
    int i = 0;

    for (const auto& entry : this->handlers) {
      allowedOrigins[i++] = convertStringToWString(entry.first + "://*").c_str();
    }

    registration->put_HasAuthorityComponent(TRUE);
    registration->SetAllowedOrigins(this->handlers.size(), allowedOrigins);

    this->coreWebView2CustomSchemeRegistrations.insert(registration);

    if (this->configuration.webview.As(&options) != S_OK) {
      return false;
    }

    for (const auto& registration : this->coreWebView2CustomSchemeRegistrations) {
      registrations[registrationsCount++] = registration.Get();
    }

    options->SetCustomSchemeRegistrations(
      registrationsCount,
      static_cast<ICoreWebView2CustomSchemeRegistration**>(registrations)
    );

  #endif

    this->handlers.insert_or_assign(scheme, handler);
    return true;
  }

  bool SchemeHandlers::handleRequest (
    const Request& request,
    const HandlerCallback callback
  ) {
    Lock lock(requests.mutex);
    // request was not finalized, likely not from a `Request::Builder`
    if (!request.finalized) {
      return false;
    }

    if (requests.map.contains(request.id)) {
      return false;
    }

    // respond with a 404 if somehow we are trying to respond to a
    // request scheme we do not know about, we do not need to call
    // `request.finalize()` as we'll just respond to the request right away
    if (!this->handlers.contains(request.scheme)) {
      auto response = IPC::SchemeHandlers::Response(request, 404);
      // make sure the response was finished first
      response.finish();

      // notify finished, even for 404
      if (response.request.callbacks.finish != nullptr) {
        response.request.callbacks.finish();
      }

      if (callback != nullptr) {
        callback(response);
      }

      return true;
    }

    const auto handler = this->handlers.at(request.scheme);

    // fail if there is somehow not a handler for this request scheme
    if (handler == nullptr) {
      return false;
    }

    if (request.error != nullptr) {
      auto response = IPC::SchemeHandlers::Response(request, 500);
      response.fail(request.error);
      return true;
    }

    const auto result = requests.map.insert_or_assign(request.id, std::move(request));
    const auto id = request.id;

    // stored request reference
    auto& req = result.first->second;

    handler(req, this->router, req.callbacks, [this, id, callback](auto& response) {
      // make sure the response was finished before
      // calling the `callback` function below
      response.finish();

      // notify finished
      if (response.request.callbacks.finish != nullptr) {
        response.request.callbacks.finish();
      }

      if (callback != nullptr) {
        callback(response);
      }

      Lock lock(requests.mutex);
      requests.map.erase(id);
    });

    return true;
  }

  bool SchemeHandlers::isRequestActive (uint64_t id) {
    Lock lock(requests.mutex);
    return requests.map.contains(id);
  }

  bool SchemeHandlers::isRequestCancelled (uint64_t id) {
    Lock lock(requests.mutex);
    return (
      id > 0 &&
      requests.map.contains(id) &&
      requests.map.at(id).cancelled
    );
  }

  #if SSC_PLATFORM_APPLE
    void SchemeHandlers::configureWebView (SSCBridgeWebView* webview) {
    }
  #elif SSC_PLATFORM_LINUX
    void SchemeHandlers::configureWebView (WebKitWebView* webview) {
    }
  #elif SSC_PLATFORM_WINDOWS
    void SchemeHandlers::configureWebView (ICoreWebView2* webview) {
    }
  #endif

  SchemeHandlers::Request::Builder::Builder (
    SchemeHandlers* handlers,
    PlatformRequest platformRequest
  ) {
  #if SSC_PLATFORM_APPLE
    this->absoluteURL = platformRequest.request.URL.absoluteString.UTF8String;
  #elif SSC_PLATFORM_LINUX
    this->absoluteURL = webkit_uri_scheme_request_get_uri(schemeRequest);
  #elif SSC_PLATFORM_WINDOWS
    LPWSTR requestURI;
    platformRequest->get_Uri(&requestURI);
    this->absoluteURL = convertWStringToString(requestURI);
    CoTaskMemFree(requestURI);
  #endif

    const auto userConfig = handlers->router->bridge->userConfig;
    const auto url = URL::Components::parse(this->absoluteURL);
    const auto bundleIdentifier = userConfig.contains("meta_bundle_identifier")
      ? userConfig.at("meta_bundle_identifier")
      : "";

    this->request.reset(new Request(
      handlers,
      platformRequest,
      Request::Options {
        .scheme = url.scheme
      }
    ));

    // default client id, can be overloaded with 'runtime-client-id' header
    this->request->client.id = handlers->router->bridge->id;

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

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (const Map& headers) {
    for (const auto& entry : headers) {
      this->request->headers.set(entry.first, entry.second);
    }
    return *this;
  }

#if SSC_PLATFORM_APPLE
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
      return this->setBody(data.length, reinterpret_cast<const char*>(data.bytes));
    }
    return *this;
  }
#elif SSC_PLATFORM_LINUX
  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setHeaders (
    const SoupMessageHeaders* headers
  ) {
    if (headers == nullptr) {
      return *this;
    }

    soup_message_headers_foreach(
      headers,
      [](auto name, auto value, auto userData) {
        auto request = reinterpret_cast<SchemeHandlers::Request*>(userData);
        request->headers.set(name, value);
      },
      this->request
    );

    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (
    const GInputStream* stream
  ) {
    if (stream == nullptr) {
      return *this;
    }

    if (this->request->method == "POST" || this->request->method == "PUT" || this->request->method == "PATCH") {
      GError* error = nullptr;
      this->request->body->bytes = std::make_shared<char*>(new char[MAX_URI_SCHEME_REQUEST_BODY_BYTES]{0});
      const auto success = g_input_stream_read_all(
        stream,
        *this->request.body.bytes
        MAX_URI_SCHEME_REQUEST_BODY_BYTES,
        &this->request->body.size,
        nullptr,
        &this->error
      );
    }
    return *this;
  }
#endif

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (const Body& body) {
    if (this->request->method == "POST" || this->request->method == "PUT" || this->request->method == "PATCH") {
      this->request->body = body;
    }
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setBody (size_t size, const char* bytes) {
    if (this->request->method == "POST" || this->request->method == "PUT" || this->request->method == "PATCH") {
      if (size > 0 && bytes != nullptr) {
        this->request->body.size = size;
        this->request->body.bytes = std::make_shared<char*>(new char[size]{0});
        memcpy(
          *this->request->body.bytes,
          bytes,
          this->request->body.size
        );
      }
    }
    return *this;
  }

  SchemeHandlers::Request::Builder& SchemeHandlers::Request::Builder::setCallbacks (const RequestCallbacks& callbacks) {
    this->request->callbacks = callbacks;
    return *this;
  }

  SchemeHandlers::Request& SchemeHandlers::Request::Builder::build () {
    this->request->error = this->error;
    this->request->finalize();
    return *this->request;
  }

  SchemeHandlers::Request::Request (
    SchemeHandlers* handlers,
    PlatformRequest platformRequest,
    const Options& options
  )
    : handlers(handlers),
      router(handlers->router),
      scheme(options.scheme),
      method(options.method),
      hostname(options.hostname),
      pathname(options.pathname),
      query(options.query),
      fragment(options.fragment),
      headers(options.headers)
  {
    this->platformRequest = platformRequest;
  }

  SchemeHandlers::Request::~Request () {}

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

    destination->router = source.router;
    destination->handlers = source.handlers;
    destination->platformRequest = source.platformRequest;
  }

  SchemeHandlers::Request::Request (const Request& request) noexcept {
    copyRequest(this, request);
  }

  SchemeHandlers::Request::Request (Request&& request) noexcept {
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
      return (
        this->scheme +
        "://" +
        this->hostname +
        this->pathname +
        (this->query.size() ? "?" + this->query : "") +
        (this->fragment.size() ? "#" + this->fragment : "")
      );
    }

    return (
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
    return this->handlers != nullptr && this->handlers->isRequestActive(this->id);
  }

  bool SchemeHandlers::Request::isCancelled () const {
    return this->handlers != nullptr && this->handlers->isRequestCancelled(this->id);
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
    const Request& request,
    int statusCode,
    const Headers headers
  ) : request(std::move(request)),
      handlers(request.handlers),
      client(request.client),
      id(request.id)
  {
    const auto defaultHeaders = split(
      this->request.router->bridge->userConfig["webview_headers"],
      '\n'
    );

    if (SSC::isDebugEnabled()) {
      this->setHeader("cache-control", "no-cache");
    }

    this->setHeader("access-control-allow-origin", "*");
    this->setHeader("access-control-allow-methods", "*");
    this->setHeader("access-control-allow-headers", "*");
    this->setHeader("access-control-allow-credentials", "true");

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
    destination->platformResponse = source.platformResponse;
  }

  SchemeHandlers::Response::Response (const Response& response) noexcept
    : request(response.request)
  {
    copyResponse(this, response);
  }

  SchemeHandlers::Response::Response (Response&& response) noexcept
    : request(response.request)
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
      return false;
    }

    if (this->request.platformRequest == nullptr) {
      return false;
    }

    if (statusCode >= 100 && statusCode < 600) {
      this->statusCode = statusCode;
    }

    for (const auto& header : headers) {
      this->setHeader(header);
    }

  #if SSC_PLATFORM_APPLE || SSC_PLATFORM_LINUX
    // webkit status codes cannot be in the range of 300 >= statusCode < 400
    if (this->statusCode >= 300 && this->statusCode < 400) {
      this->statusCode = 200;
    }
  #endif

  #if SSC_PLATFORM_APPLE
    auto headerFields= [NSMutableDictionary dictionary];
    for (const auto& entry : this->headers) {
      headerFields[@(entry.name.c_str())] = @(entry.value.c_str());
    }

    auto platformRequest = this->request.platformRequest;
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
  #elif SSC_PLATFORM_LINUX
    const auto contentLength = this->headers.has("content-length")
      ? this->headers.get("content-length")
      : String("");

    gint64 size = -1;
    if (contentLength.size() > 0) {
      try {
       size = std::stol(contentLength);
      } catch (...) {
      }
    }

    this->platformResponseStream = g_memory_input_stream_new();
    this->platformResponse = webkit_uri_scheme_response_new(stream, size);

    auto headers = soup_message_headers_new(SOUP_MESSAGE_HEADERS_RESPONSE);
    for (const auto& entry : this->headers) {
      soup_message_headers_append(headers, entry.name.c_str(), entry.value.c_str());
    }

    webkit_uri_scheme_response_set_http_headers(this->platformResponse, headers);

    if (this->headers.has("content-type")) {
      const auto contentType = this->headers.get("content-type");
      if (contentType.size() > 0) {
        webkit_uri_scheme_response_set_content_type(this->platformResponse, contentType.c_str());
      }
    }

    const auto statusText = String(
      STATUS_CODES.contains(this->statusCode)
        ? STATUS_CODES.at(this->statusCode)
        : ""
    );

    webkit_uri_scheme_response_set_status(
      this->platformResponse,
      this->statusCode,
      statusText.size() > 0 ? statusText.c_str() : nullptr
    );

  #elif SSC_PLATFORM_WINDOWS
  #endif
    return false;
  }

  bool SchemeHandlers::Response::write (size_t size, const char* bytes) {
    if (
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      return false;
    }

    if (!this->platformResponse) {
      // set 'content-length' header if response was not created
      this->setHeader("content-length", size);
      if (!this->writeHead()) {
        return false;
      }
    }

    if (size > 0 && bytes != nullptr) {
    #if SSC_PLATFORM_APPLE
      const auto data = [NSData dataWithBytes: bytes length: size];
      @try {
        [this->request.platformRequest didReceiveData: data];
      } @catch (::id) {
        return false;
      }
      return true;
    #elif SSC_PLATFORM_LINUX
      g_memory_input_stream_add_data(
        this->platformResponseStream,
        const_cast<void*>(reinterpret_cast<const void*>(bytes)),
        size,
        nullptr
      );
      return true;
    #elif SSC_PLATFORM_WINDOWS
    #endif
    }

    return false;
  }

  bool SchemeHandlers::Response::write (size_t size, SharedPointer<char*> bytes) {
    if (bytes != nullptr && size > 0) {
      return this->write(size, *bytes);
    }

    return false;
  }

  bool SchemeHandlers::Response::write (const String& source) {
    if (!this->hasHeader("content-type")) {
      this->setHeader("content-type", "application/octet-stream");
    }
    return this->write(source.size(), source.data());
  }

  bool SchemeHandlers::Response::write (const JSON::Any& json) {
    this->setHeader("content-type", "application/json");
    return this->write(json.str());
  }

  bool SchemeHandlers::Response::write (const FileResource& resource) {
    auto responseResource = FileResource(resource.path.string());
    const auto contentLength = responseResource.size();
    const auto contentType = responseResource.mimeType();

    if (contentType.size() > 0) {
      this->setHeader("content-type", contentType);
    }

    if (contentLength > 0) {
      this->setHeader("content-length", contentLength);
    }

    if (contentLength > 0) {
      const auto data = responseResource.read();
      return this->write(responseResource.size(), data);
    }

    return false;
  }

  bool SchemeHandlers::Response::send (const String& source) {
    return this->write(source) && this->finish();
  }

  bool SchemeHandlers::Response::send (const JSON::Any& json) {
    return this->write(json) && this->finish();
  }

  bool SchemeHandlers::Response::send (const FileResource& resource) {
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
      if (!this->writeHead()) {
        return false;
      }
    }

  #if SSC_PLATFORM_APPLE
    @try {
      [this->request.platformRequest didFinish];
    } @catch (::id) {}
  #if !__has_feature(objc_arc)
    [this->platformResponse release];
  #endif
  #elif SSC_PLATFORM_LINUX
    webkit_uri_scheme_request_finish_with_response(
      this->request.platformRequest,
      this->platformResponse
    );
    g_object_unref(this->platformResponseStream);
    this->platformResponseStream = nullptr;
  #elif SSC_PLATFORM_WINDOWS
  #else
  #endif

    this->platformResponse = nullptr;
    this->finished = true;
    return true;
  }

  void SchemeHandlers::Response::setHeader (const String& name, const Headers::Value& value) {
    const auto router = this->request.handlers->router;
    if (toLowerCase(name) == "referer") {
      if (router->location.workers.contains(value.string)) {
        const auto workerLocation = router->location.workers[value.string];
        this->headers[name] = workerLocation;
        return;
      } else if (router->core->serviceWorker.bridge->router.location.workers.contains(value.string)) {
        const auto workerLocation = router->core->serviceWorker.bridge->router.location.workers[value.string];
        this->headers[name] = workerLocation;
        return;
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

  void SchemeHandlers::Response::setHeader (const String& name, uint64_t value) {
    this->setHeader(name, std::to_string(value));
  }

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
  #if SSC_PLATFORM_APPLE
    if (error.localizedDescription != nullptr) {
      return this->fail(error.localizedDescription.UTF8String);
    } else if (error.localizedFailureReason != nullptr) {
      return this->fail(error.localizedFailureReason.UTF8String);
    } else {
      return this->fail("Request failed for an unknown reason");
    }
  #elif SSC_PLATFORM_LINUX
    if (error != nullptr && error->message != nullptr) {
      return this->fail(error->message);
    }
  #elif SSC_PLATFORM_WINDOWS
  #else
    return this->fail(String(error));
  #endif
  }

  bool SchemeHandlers::Response::fail (const String& reason) {
    const auto bundleIdentifier = this->request.router->bridge->userConfig["meta_bundle_identifier"];
    if (
      this->finished ||
      !this->handlers->isRequestActive(this->id) ||
      this->handlers->isRequestCancelled(this->id)
    ) {
      return false;
    }

  #if SSC_PLATFORM_APPLE
    @try {
      [this->request.platformRequest
        didFailWithError: [NSError
         errorWithDomain: @(bundleIdentifier.c_str())
                    code: 1
                userInfo: @{NSLocalizedDescriptionKey: @(reason.c_str())}
      ]];
    } @catch (::id e) {
      // ignore possible 'NSInternalInconsistencyException'
      return false;
    }
  #elif SSC_PLATFORM_LINUX
    const auto quark = g_quark_from_string(bundleIdentifier.c_str());
    if (quark == nullptr) {
      return false;
    }

    const auto error = g_error_new(quark 1, reason.c_str());

    if (error == nullptr) {
      return false;
    }

    webkit_uri_scheme_request_finish_error(this->platformRequest, error);
  #elif SSC_PLATFORM_WINDOWS
  #else
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
      this->setHeader("location", this->request.origin + location);
    } else if (location.starts_with(".")) {
      this->setHeader("location", this->request.origin + location.substr(1));
    } else {
      this->setHeader("location", location);
    }

    if (!this->writeHead(statusCode)) {
      return false;
    }

    if (this->request.method != "HEAD" && this->request.method != "OPTIONS") {
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
