#ifndef SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_H
#define SOCKET_RUNTIME_SERVICE_WORKER_CONTAINER_H

#include "../core/headers.hh"
#include "../core/json.hh"
#include "../ipc/client.hh"

#include "protocols.hh"

namespace SSC {
  // forward
  namespace IPC { class Bridge; }
  class Core;

  class ServiceWorkerContainer {
    public:
      using ID = IPC::Client::ID;

      struct Client {
        ID id = 0;
        int index = 0;
      };

      struct RegistrationOptions {
        enum class Type { Classic, Module };

        Type type = Type::Module;
        String scope;
        String scriptURL;
        String scheme = "*";
        ID id = 0;
      };

      struct Registration {
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
          Map data;
          const JSON::Object json () const;
          void set (const String& key, const String& value);
          const String get (const String& key) const;
          void remove (const String& key);
          void clear ();
        };

        ID id = 0;
        String scriptURL;
        Storage storage;
        Atomic<State> state = State::None;
        RegistrationOptions options;

        Registration (
          const ID id,
          const String& scriptURL,
          const State state,
          const RegistrationOptions& options
        );

        Registration (const Registration&);
        Registration (Registration&&);
        ~Registration () = default;
        Registration& operator= (const Registration&);
        Registration& operator= (Registration&&);

        const JSON::Object json () const;
        bool isActive () const;
        bool isWaiting () const;
        bool isInstalling () const;
        const String getStateString () const;
      };

      struct FetchBody {
        size_t size = 0;
        SharedPointer<char[]> bytes = nullptr;
      };

      struct FetchRequest {
        String method;
        String scheme = "*";
        String hostname = "";
        String pathname;
        String query;
        Headers headers;
        FetchBody body;
        Client client;

        const String str () const;
      };

      struct FetchResponse {
        uint64_t id = 0;
        int statusCode = 200;
        Headers headers;
        FetchBody body;
        Client client;
      };

      using Registrations = std::map<String, Registration>;
      using FetchCallback = std::function<void(const FetchResponse)>;
      using FetchCallbacks = std::map<uint64_t, FetchCallback>;
      using FetchRequests = std::map<uint64_t, FetchRequest>;

      SharedPointer<Core> core = nullptr;
      IPC::Bridge* bridge = nullptr;
      AtomicBool isReady = false;
      Mutex mutex;

      ServiceWorkerProtocols protocols;
      Registrations registrations;

      FetchRequests fetchRequests;
      FetchCallbacks fetchCallbacks;

      static ServiceWorkerContainer* sharedContainer ();
      ServiceWorkerContainer (SharedPointer<Core> core);
      ~ServiceWorkerContainer ();

      void init (IPC::Bridge* bridge);
      void reset ();
      const Registration& registerServiceWorker (const RegistrationOptions& options);
      bool unregisterServiceWorker (uint64_t id);
      bool unregisterServiceWorker (String scopeOrScriptURL);
      void skipWaiting (uint64_t id);
      void updateState (uint64_t id, const String& stateString);
      bool fetch (const FetchRequest& request, FetchCallback callback);
      bool claimClients (const String& scope);
  };
}
#endif
