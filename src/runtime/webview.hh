#ifndef SOCKET_RUNTIME_WEBVIEW_H
#define SOCKET_RUNTIME_WEBVIEW_H

#include "filesystem.hh"
#include "platform.hh"
#include "ipc.hh"

#include "webview/preload.hh"
#include "webview/webview.hh"
#include "webview/client.hh"

namespace ssc::runtime::bridge {
  // forward
  class Bridge;
}

namespace ssc::runtime::serviceworker {
  // forward
  class Server;
}

namespace ssc::runtime::webview {
  // forward
  class IBridge;
  class Navigator;

  class Origin : public URL {
    public:
      using URL::URL;
      String name () const;
  };

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

        Navigator& navigator;
        Map<String, String> workers;
        Map<String, String> mounts;

        Location (Navigator&);
        Location () = delete;
        Location (const Location&) = delete;
        Location (Location&&) = delete;

        Location& operator = (const Location&) = delete;
        Location& operator = (Location&&) = delete;

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

      SharedPointer<serviceworker::Server> serviceWorkerServer = nullptr;
      Location location;
      bridge::Bridge& bridge;

    #if SOCKET_RUNTIME_PLATFORM_APPLE
      SSCNavigationDelegate* navigationDelegate = nullptr;
    #endif

      // owned by bridge
      Navigator (bridge::Bridge&);
      Navigator () = delete;
      Navigator (const Navigator&) = delete;
      Navigator (Navigator&&) = delete;
      ~Navigator ();

      Navigator& operator = (const Navigator&) = delete;
      Navigator& operator = (Navigator&&) = delete;

      void init ();
      void configureWebView (WebView* object);
      bool handleNavigationRequest (const String& currentURL, const String& requestedURL);
      bool isNavigationRequestAllowed (const String& location, const String& requestURL);
      void configureMounts ();
  };

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

          Builder& setBody (const bytes::Buffer&);
          Builder& setBody (size_t, const unsigned char*);
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
        bytes::Buffer body;
        Client client;
        String originalURL;
        RequestCallbacks callbacks;

        debug::Tracer tracer;

        Atomic<bool> finalized = false;
        Atomic<bool> cancelled = false;

        Error* error = nullptr;
        SchemeHandlers* handlers;
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
        uint64_t id = crypto::rand64();
        int statusCode = 200;
        http::Headers headers;
        Client client;
        mutable Mutex mutex;

        Atomic<bool> finished = false;

        Vector<SharedPointer<unsigned char[]>> buffers;

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

        bool write (const bytes::Buffer&);
        bool write (size_t size, SharedPointer<unsigned char[]>);
        bool write (const String& source);
        bool write (const JSON::Any& json);
        bool write (const filesystem::Resource& resource);
        bool write (const filesystem::Resource::ReadStream::Buffer& buffer);
        bool write (size_t size, const unsigned char* bytes);
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
        const bridge::Bridge&,
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
      bridge::Bridge& bridge;
      RequestMap activeRequests;

      // owned by bridge
      SchemeHandlers (bridge::Bridge&);
      ~SchemeHandlers ();
      SchemeHandlers () = delete;
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

  class IBridge : public ipc::IBridge {
    public:
    #if SOCKET_RUNTIME_PLATFORM_LINUX && !SOCKET_RUNTIME_DESKTOP_EXTENSION
      WebKitWebContext* webContext = nullptr;
    #endif

      IBridge (
        context::Dispatcher& dispatcher,
        context::RuntimeContext& context,
        const Client& client,
        Map<String, String> userConfig
      ) : ipc::IBridge(
            dispatcher,
            context,
            dynamic_cast<const ipc::Client&>(client),
            userConfig
          )
      {}

      virtual bool navigate (const String& url) = 0;
  };
}
#endif
