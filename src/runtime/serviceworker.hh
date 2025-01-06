#ifndef SOCKET_RUNTIME_SERVICEWORKER_H
#define SOCKET_RUNTIME_SERVICEWORKER_H

#include "ipc.hh"
#include "http.hh"
#include "bytes.hh"
#include "crypto.hh"
#include "string.hh"
#include "window.hh"
#include "webview.hh"
#include "platform.hh"

namespace ssc::runtime::serviceworker {
  // forward
  class Fetch;
  class Manager;
  class Container;

  using ID = uint64_t;
  using Origin = webview::Origin;

  struct Client : public webview::Client {
    using webview::Client::Client;
  };

  class Request : public http::Request {
    public:
      Client client;

      using http::Request::Request;
      inline const String str () const {
        return this->url.str();
      }
  };

  class Response : public http::Response {
    public:
      ID id = 0;
      int statusCode = 200;
      Client client;

      using http::Response::Response;
  };

  class Fetch {
    public:
      using Callback = Function<void(const Response)>;

      struct Options {
        Client client;
        bool waitForRegistrationToFinish = true;
      };

      bytes::BufferQueue writeQueue;
      Container& container;
      Callback callback;
      Response response;
      Request request;
      Options options;
      Mutex mutex;
      ID id = crypto::rand64();

      Fetch () = delete;
      Fetch (Container&, const Request&, const Options&);

      bool init (const Callback);
      bool write (const bytes::Buffer&);
      bool finish ();
  };

  class Registration {
    public:
      static String key (const String&, const URL&, const String& = "socket");
      struct Options {
        enum class Type { Classic, Module };

        Type type = Type::Module;
        String scriptURL;
        String scope;
        String scheme = "*";
        String serializedWorkerArgs = "";
        ID id = 0;
      };

      enum class State {
        None,
        Error,
        Registering,
        Registered,
        Installing,
        Installed,
        Activating,
        Activated
      };

      struct Storage {
        Map<String, String> data;
        const JSON::Object json () const;
        void set (const String& key, const String& value);
        const String get (const String& key) const;
        void remove (const String& key);
        void clear ();
      };

      ID id = 0;
      Storage storage;
      Atomic<State> state = State::None;
      Options options;
      Origin origin;

      Registration (
        const ID id,
        const State state,
        const Origin origin,
        const Options& options
      );

      Registration (const Registration&);
      Registration (Registration&&);
      ~Registration () = default;
      Registration& operator= (const Registration&);
      Registration& operator= (Registration&&);

      const JSON::Object json (bool includeSerializedWorkerArgs = false) const;
      bool isActive () const;
      bool isWaiting () const;
      bool isInstalling () const;
      const String getStateString () const;
  };

  class Protocols {
    public:
      struct Data {
        String json = ""; // we store JSON as a string here
      };

      struct Protocol {
        String scheme;
        Data data;
      };

      Container& container;
      Origin origin;
      Map<String, Protocol> mapping;
      Mutex mutex;

      Protocols (Container&);
      ~Protocols ();

      bool registerHandler (const String& scheme, const Data data = { "" });
      bool unregisterHandler (const String& scheme);
      const Data getHandlerData (const String& scheme);
      bool setHandlerData (const String& scheme, const Data data = { "" });
      bool hasHandler (const String& scheme);
      const String getServiceWorkerScope (const String& scheme);
      const Vector<String> getSchemes () const;
      const Vector<String> getProtocols () const;

  };

  class Container {
    public:
      SharedPointer<ipc::IBridge> bridge = nullptr;
      Atomic<bool> isReady = false;
      Mutex mutex;

      Protocols protocols;
      Origin origin;
      Map<String, Registration> registrations;
      Map<ID, SharedPointer<Fetch>> fetches;

      Container ();
      ~Container ();

      bool ready ();
      void init (SharedPointer<ipc::IBridge>);
      void reset ();
      const Registration& registerServiceWorker (const Registration::Options&);
      bool unregisterServiceWorker (ID);
      bool unregisterServiceWorker (String);
      void skipWaiting (ID);
      void updateState (ID, const String&);
      bool claimClients (const String& scope);
      bool fetch (const Request&, const Fetch::Options&, const Fetch::Callback);
  };

  class Server {
    public:
      struct Options {
        String origin;
        Map<String, String> userConfig;
        window::Window::Options window;
      };

      ID id = crypto::rand64();
      Origin origin;
      Mutex mutex;

      // config
      Map<String, String> userConfig;
      window::Window::Options windowOptions;

      // instance
      Container container;
      Manager& manager;

      SharedPointer<window::Manager::ManagedWindow> window = nullptr;
      SharedPointer<window::IBridge> bridge = nullptr;

      Server (Manager&, const Options&);
      bool init ();
      bool destroy ();
      bool fetch (const Request&, const Fetch::Options&, const Fetch::Callback);
  };

  class Manager {
    public:
      struct Options {
        window::Manager& windowManager;
      };

      Mutex mutex;
      Map<String, SharedPointer<Server>> servers;
      window::Manager& windowManager;
      context::RuntimeContext& context;

      Manager (context::RuntimeContext&, const Options&);
      bool fetch (const Request&, const Fetch::Options&, const Fetch::Callback);

      SharedPointer<Server> init (const String&, const Server::Options&);
      SharedPointer<Server> get (const String&);
      SharedPointer<Server> get (ipc::IBridge*);
      bool destroy (const String&);
  };

  inline const String normalizeScope (const String& scope) {
    auto normalized = string::trim(scope);

    if (normalized.size() == 0) {
      return "/";
    }

    if (normalized.ends_with("/") && normalized.size() > 1) {
      normalized = normalized.substr(0, normalized.size() - 1);
    }

    return normalized;
  }
}
#endif
