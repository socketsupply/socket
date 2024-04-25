#ifndef SSC_IPC_SCHEME_HANDLERS_H
#define SSC_IPC_SCHEME_HANDLERS_H

#include "../core/core.hh"
#include "../core/platform.hh"

namespace SSC::IPC {
  class Router;
  class SchemeHandlers;
}

namespace SSC::IPC {
  class SchemeHandlersInternals;
  class SchemeHandlers {
    private:
      SchemeHandlersInternals* internals = nullptr;

    public:
    #if SSC_PLATFORM_APPLE
      using Error = NSError;
    #elif SSC_PLATFORM_LINUX
      using Error = GError;
    #else
      using Error = char;
    #endif

      struct Client {
        uint64_t id = 0;
      };

      struct Body {
        size_t size = 0;
        SharedPointer<char*> bytes = nullptr;
      };

      struct RequestCallbacks {
        Function<void()> cancel;
        Function<void()> finish;
      };

      #if SSC_PLATFORM_APPLE
        using PlatformRequest = id<WKURLSchemeTask>;
        using PlatformResponse = NSHTTPURLResponse*;
      #elif SSC_PLATFORM_LINUX
        using PlatformRequest = WebKitURISchemeRequest*;
        using PlatformResponse = WebKitURISchemeResponse*;
      #elif SSC_PLATFORM_WINDOWS
        using PlatformRequest = ICoreWebView2WebResourceRequest*;
        using PlatformResponse = ICoreWebView2WebResourceResponse*;
      #else
        // TODO
      #endif

      struct Request {
        struct Options {
          String scheme = "";
          String method = "GET";
          String hostname = "";
          String pathname = "/";
          String query = "";
          String fragment = "";
          Headers headers;
          Function<bool(const Request*)> isCancelled;
        };

        struct Builder {
          String absoluteURL;
          Error* error = nullptr;
          UniquePointer<Request> request = nullptr;

          Builder (
            SchemeHandlers* handlers,
            PlatformRequest platformRequest
          );

          Builder& setScheme (const String& scheme);
          Builder& setMethod (const String& method);
          Builder& setHostname (const String& hostname);
          Builder& setPathname (const String& pathname);
          Builder& setQuery (const String& query);
          Builder& setFragment (const String& fragment);
          Builder& setHeader (const String& name, const Headers::Value& value);
          Builder& setHeaders (const Headers& headers);
          Builder& setHeaders (const Map& headers);

        #if SSC_PLATFORM_APPLE
          Builder& setHeaders (const NSDictionary<NSString*, NSString*>* headers);
          Builder& setBody (const NSData* data);
        #elif SSC_PLATFORM_LINUX
          Builder& setHeaders (const SoupMessageHeaders* headers);
          Builder& setBody (const GInputStream* stream);
        #endif

          Builder& setBody (const Body& body);
          Builder& setBody (size_t size, const char* bytes);
          Builder& setCallbacks (const RequestCallbacks& callbacks);
          Request& build ();
        };

        uint64_t id = rand64();
        String scheme = "";
        String method = "GET";
        String hostname = "";
        String pathname = "/";
        String query = "";
        String fragment = "";
        Headers headers;

        String origin = "";
        Map params;
        Body body;
        Client client;
        String originalURL;
        RequestCallbacks callbacks;

        Atomic<bool> finalized = false;
        Atomic<bool> cancelled = false;

        Error* error = nullptr;
        const Router* router = nullptr;
        SchemeHandlers* handlers = nullptr;
        PlatformRequest platformRequest;

        Request (
          SchemeHandlers* handlers,
          PlatformRequest platformRequest,
          const Options& options
        );

        Request (const Request&) noexcept;
        Request (Request&&) noexcept;
        ~Request ();
        Request& operator= (const Request&) noexcept;
        Request& operator= (Request&&) noexcept;

        const String getHeader (const String& name) const;
        bool hasHeader (const String& name) const;
        const String str () const;
        const String url () const;
        bool finalize ();
        JSON::Object json () const;
        bool isActive () const;
        bool isCancelled () const;
      };

      struct Response {
        struct Event {
          String name = "";
          String data = "";
          const String str () const noexcept;
          size_t count () const noexcept;
        };

        const Request request;
        uint64_t id = rand64();
        int statusCode = 200;
        Headers headers;
        Client client;
        Atomic<bool> finished = false;
        SchemeHandlers* handlers = nullptr;
        PlatformResponse platformResponse = nullptr;

        Response (
          const Request& request,
          int statusCode = 200,
          const Headers headers = {}
        );

        Response (const Response&) noexcept;
        Response (Response&&) noexcept;
        ~Response ();
        Response& operator= (const Response&) noexcept;
        Response& operator= (Response&&) noexcept;

        bool write (size_t size, const char* bytes);
        bool write (size_t size, SharedPointer<char*>);
        bool write (const String& source);
        bool write (const JSON::Any& json);
        bool write (const FileResource& resource);
        bool send (const String& source);
        bool send (const JSON::Any& json);
        bool send (const FileResource& resource);
        bool writeHead (int statusCode = 0, const Headers headers = {});
        bool finish ();
        void setHeader (const String& name, const Headers::Value& value);
        void setHeader (const String& name, size_t value);
        void setHeader (const String& name, int64_t value);
        void setHeader (const String& name, uint64_t value);
        void setHeader (const Headers::Header& header);
        void setHeaders (const Headers& headers);
        void setHeaders (const Map& headers);
      #if SSC_PLATFORM_APPLE
        void setHeaders (const NSDictionary<NSString*, NSString*>* headers);
      #endif
        const String getHeader (const String& name) const;
        bool hasHeader (const String& name) const;
        bool fail (const String& reason);
        bool fail (const Error* error);
        bool redirect (const String& location, int statusCode = 302);
      };

      using HandlerCallback = Function<void(Response&)>;
      using Handler = Function<void(
        const Request&,
        const Router*,
        RequestCallbacks& callbacks,
        HandlerCallback
      )>;
      using HandlerMap = std::map<String, Handler>;
      using RequestMap = std::map<uint64_t, Request>;

      struct Configuration {
      #if SSC_PLATFORM_APPLE
        WKWebViewConfiguration* webview = nullptr;
      #endif
      };

      Configuration configuration;
      Router* router = nullptr;
      HandlerMap handlers;
      RequestMap requests;
      Mutex mutex;

    #if SSC_PLATFORM_LINUX
      GInputStream* platformResponseStream = nullptr
    #endif

      SchemeHandlers (Router* router);
      ~SchemeHandlers ();

      void configure (const Configuration& configuration);
      bool hasHandlerForScheme (const String& scheme);
      bool registerSchemeHandler (const String& scheme, const Handler& handler);
      bool handleRequest (const Request& request, const HandlerCallback calllback = nullptr);
      bool isRequestActive (uint64_t id);
      bool isRequestCancelled (uint64_t id);
  };
}

#endif
