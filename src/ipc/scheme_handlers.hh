#ifndef SOCKET_RUNTIME_IPC_SCHEME_HANDLERS_H
#define SOCKET_RUNTIME_IPC_SCHEME_HANDLERS_H

#include "../core/core.hh"
#include "../core/trace.hh"
#include "../core/webview.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../android/platform.hh"
#endif

#include "client.hh"

namespace SSC::IPC {
  class Bridge;
  class SchemeHandlers;
}

namespace SSC::IPC {
  /**
   * An opaque containere for platform internals
   */
  class SchemeHandlersInternals;

  /**
   * A container for registering scheme handlers attached to an `IPC::Bridge`
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

      struct Body {
        size_t size = 0;
        SharedPointer<char[]> bytes = nullptr;
      };

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
          Headers headers;
          Function<bool(const Request*)> isCancelled;
        };

        struct Builder {
          String absoluteURL;
          Error* error = nullptr;
          SharedPointer<Request> request = nullptr;

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

        #if SOCKET_RUNTIME_PLATFORM_APPLE
          Builder& setHeaders (const NSDictionary<NSString*, NSString*>* headers);
          Builder& setBody (const NSData* data);
        #elif SOCKET_RUNTIME_PLATFORM_LINUX
          Builder& setHeaders (const SoupMessageHeaders* headers);
          Builder& setBody (GInputStream* stream);
        #endif

          Builder& setBody (const Body& body);
          Builder& setBody (size_t size, const char* bytes);
          Builder& setCallbacks (const RequestCallbacks& callbacks);
          SharedPointer<Request> build ();
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

        Tracer tracer;

        Atomic<bool> finalized = false;
        Atomic<bool> cancelled = false;

        Error* error = nullptr;
        Bridge* bridge = nullptr;
        SchemeHandlers* handlers = nullptr;
        PlatformRequest platformRequest;

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
        Headers headers;
        Client client;

        Mutex mutex;
        Atomic<bool> finished = false;

        Vector<SharedPointer<char[]>> buffers;

        Tracer tracer;

        SchemeHandlers* handlers = nullptr;
        PlatformResponse platformResponse = nullptr;

      #if SOCKET_RUNTIME_PLATFORM_LINUX
        GInputStream* platformResponseStream = nullptr;
      #endif

        Response (
          SharedPointer<Request> request,
          int statusCode = 200,
          const Headers headers = {}
        );

        Response (const Response&) noexcept;
        Response (Response&&) noexcept;
        ~Response ();
        Response& operator= (const Response&) noexcept;
        Response& operator= (Response&&) noexcept;

        bool write (size_t size, SharedPointer<char[]>);
        bool write (const String& source);
        bool write (const JSON::Any& json);
        bool write (const FileResource& resource);
        bool write (const FileResource::ReadStream::Buffer& buffer);
        bool write (size_t size, const char* bytes);
        bool send (const String& source);
        bool send (const JSON::Any& json);
        bool send (const FileResource& resource);
        bool writeHead (int statusCode = 0, const Headers headers = {});
        bool finish ();
        void setHeader (const String& name, const Headers::Value& value);
        void setHeader (const String& name, size_t value);
        void setHeader (const String& name, int64_t value);
      #if !SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_PLATFORM_ANDROID
        void setHeader (const String& name, uint64_t value);
      #endif
        void setHeader (const Headers::Header& header);
        void setHeaders (const Headers& headers);
        void setHeaders (const Map& headers);
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
        const Bridge*,
        RequestCallbacks* callbacks,
        HandlerCallback
      )>;

      using HandlerMap = std::map<String, Handler>;
      using RequestMap = std::map<uint64_t, SharedPointer<Request>>;

      struct Configuration {
        WebViewSettings* webview;
      };

      Configuration configuration;
      HandlerMap handlers;

      Mutex mutex;
      Bridge* bridge = nullptr;
      RequestMap activeRequests;

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      Set<ComPtr<CoreWebView2CustomSchemeRegistration>> coreWebView2CustomSchemeRegistrations;
    #endif

      SchemeHandlers (Bridge* bridge);
      ~SchemeHandlers ();
      SchemeHandlers (const SchemeHandlers&) = delete;
      SchemeHandlers (SchemeHandlers&&) = delete;

      SchemeHandlers& operator= (const SchemeHandlers&) = delete;
      SchemeHandlers& operator= (SchemeHandlers&&) = delete;

      void init ();
      void configure (const Configuration& configuration);
      bool hasHandlerForScheme (const String& scheme);
      bool registerSchemeHandler (const String& scheme, const Handler& handler);
      bool handleRequest (SharedPointer<Request> request, const HandlerCallback calllback = nullptr);
      bool isRequestActive (uint64_t id);
      bool isRequestCancelled (uint64_t id);
      Handler getHandlerForScheme (const String& scheme);
  };
}
#endif
