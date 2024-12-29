#ifndef SOCKET_RUNTIME_WEBVIEW_SCHEME_HANDLERS_H
#define SOCKET_RUNTIME_WEBVIEW_SCHEME_HANDLERS_H

#include "../filesystem.hh"
#include "../platform.hh"
#include "../crypto.hh"
#include "../bytes.hh"
#include "../http.hh"

#include "client.hh"
#include "webview.hh"

namespace ssc::runtime::webview {
  class IBridge;
  /**
   * An opaque containere for platform internals
   */
  class SchemeHandlersInternals;

  /**
   * A container for registering scheme handlers attached to an `Bridge`
   * that can handle WebView requests for runtime and custom user defined
   * protocol schemes.
   */
  class SchemeHandlers {
    private:
      SchemeHandlersInternals* internals = nullptr;

    public:
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      using Error = NSError;
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      using Error = GError;
    #else
      using Error = char*;
    #endif

      struct Body : public bytes::Buffer {};

      struct RequestCallbacks {
        Function<void()> cancel = nullptr;
        Function<void()> finish = nullptr;
        Function<void(Error*)> fail = nullptr;
      };

      #if SOCKET_RUNTIME_PLATFORM_APPLE
        using PlatformRequest = id<WKURLSchemeTask>;
        using PlatformResponse = NSHTTPURLResponse*;
      #elif SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
        using PlatformRequest = WebKitURISchemeRequest*;
        using PlatformResponse = WebKitURISchemeResponse*;
      #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
        using PlatformRequest = ICoreWebView2WebResourceRequest*;
        using PlatformResponse = ICoreWebView2WebResourceResponse*;
      #elif SOCKET_RUNTIME_PLATFORM_ANDROID
        using PlatformRequest = jobject;
        using PlatformResponse = jobject;
      #else
        using PlatformRequest = void*;
        using PlatformResponse = void*;
      #endif

      struct Request {
        struct Options {
          String scheme = "";
          String method = "GET";
          String hostname = "";
          String pathname = "/";
          String query = "";
          String fragment = "";
          http::Headers headers;
          Function<bool(const Request*)> isCancelled;
        };

        struct Builder {
          String absoluteURL;
          Error* error = nullptr;
          SharedPointer<Request> request = nullptr;

        #if SOCKET_RUNTIME_PLATFORM_WINDOWS
          Builder (
            SchemeHandlers* handlers,
            PlatformRequest platformRequest,
            ICoreWebView2Environment* env
          );
        #else
          Builder (
            SchemeHandlers* handlers,
            PlatformRequest platformRequest
          );
        #endif

          Builder& setScheme (const String&);
          Builder& setMethod (const String&);
          Builder& setHostname (const String&);
          Builder& setPathname (const String&);
          Builder& setQuery (const String&);
          Builder& setFragment (const String&);
          Builder& setHeader (const String&, const http::Headers::Value&);
          Builder& setHeaders (const http::Headers&);
          Builder& setHeaders (const Map<String, String>&);

        #if SOCKET_RUNTIME_PLATFORM_APPLE
          Builder& setHeaders (const NSDictionary<NSString*, NSString*>*);
          Builder& setBody (const NSData*);
        #elif SOCKET_RUNTIME_PLATFORM_LINUX
          Builder& setHeaders (const SoupMessageHeaders*);
          Builder& setBody (GInputStream*);
        #endif

          Builder& setBody (const Body&);
          Builder& setBody (size_t, const char*);
          Builder& setCallbacks (const RequestCallbacks&);
          SharedPointer<Request> build ();
        };

        Client::ID id = crypto::rand64();
        String scheme = "";
        String method = "GET";
        String hostname = "";
        String pathname = "/";
        String query = "";
        String fragment = "";
        http::Headers headers;

        String origin = "";
        Map<String, String> params;
        Body body;
        Client client;
        String originalURL;
        RequestCallbacks callbacks;

        debug::Tracer tracer;

        Atomic<bool> finalized = false;
        Atomic<bool> cancelled = false;

        Error* error = nullptr;
        IBridge* bridge = nullptr;
        SchemeHandlers* handlers = nullptr;
        PlatformRequest platformRequest;

      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
        ICoreWebView2Environment* env = nullptr;
      #endif

        Request () = delete;
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

        SharedPointer<Request> request;
        uint64_t id = rand64();
        int statusCode = 200;
        http::Headers headers;
        Client client;

        Atomic<bool> finished = false;

        Vector<SharedPointer<char[]>> buffers;

        debug::Tracer tracer;

        SchemeHandlers* handlers = nullptr;
        PlatformResponse platformResponse = nullptr;

      #if SOCKET_RUNTIME_PLATFORM_LINUX
        GInputStream* platformResponseStream = nullptr;
      #elif SOCKET_RUNTIME_PLATFORM_WINDOWS
        IStream* platformResponseStream = nullptr;
      #endif

        Response (
          SharedPointer<Request> request,
          int statusCode = 200,
          const http::Headers headers = {}
        );

        Response (const Response&) noexcept;
        Response (Response&&) noexcept;
        ~Response ();
        Response& operator= (const Response&) noexcept;
        Response& operator= (Response&&) noexcept;

        bool write (size_t size, SharedPointer<char[]>);
        bool write (const String& source);
        bool write (const JSON::Any& json);
        bool write (const filesystem::Resource& resource);
        bool write (const filesystem::Resource::ReadStream::Buffer& buffer);
        bool write (size_t size, const char* bytes);
        bool send (const String& source);
        bool send (const JSON::Any& json);
        bool send (const filesystem::Resource& resource);
        bool writeHead (int statusCode = 0, const http::Headers headers = {});
        bool finish ();
        void setHeader (const String& name, const http::Headers::Value& value);
        void setHeader (const String& name, size_t value);
        void setHeader (const String& name, int64_t value);
      #if !SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_PLATFORM_ANDROID && !SOCKET_RUNTIME_PLATFORM_WINDOWS
        void setHeader (const String& name, uint64_t value);
      #endif
        void setHeader (const http::Headers::Header& header);
        void setHeaders (const http::Headers& headers);
        void setHeaders (const Map<String, String>& headers);
      #if SOCKET_RUNTIME_PLATFORM_APPLE
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
        const SharedPointer<Request>,
        const IBridge*,
        RequestCallbacks* callbacks,
        HandlerCallback
      )>;

      using HandlerMap = Map<String, Handler>;
      using RequestMap = Map<uint64_t, SharedPointer<Request>>;

      struct Configuration {
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS
        WebViewSettings webview;
      #else
        WebViewSettings* webview;
      #endif
      };

      Configuration configuration;
      HandlerMap handlers;

      Mutex mutex;
      IBridge* bridge = nullptr;
      RequestMap activeRequests;

      SchemeHandlers (IBridge* bridge);
      ~SchemeHandlers ();
      SchemeHandlers (const SchemeHandlers&) = delete;
      SchemeHandlers (SchemeHandlers&&) = delete;

      SchemeHandlers& operator= (const SchemeHandlers&) = delete;
      SchemeHandlers& operator= (SchemeHandlers&&) = delete;

      void init ();
      void configure (const Configuration& configuration);
      bool hasHandlerForScheme (const String& scheme);
      bool registerSchemeHandler (const String& scheme, const Handler& handler);
      bool handleRequest (
        SharedPointer<Request> request,
        const HandlerCallback calllback = nullptr
      );
      bool isRequestActive (uint64_t id);
      bool isRequestCancelled (uint64_t id);
      Handler getHandlerForScheme (const String& scheme);
  };
}
#endif
