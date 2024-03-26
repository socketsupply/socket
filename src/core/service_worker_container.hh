#ifndef SSC_SERVICE_WORKER_CONTAINER_H
#define SSC_SERVICE_WORKER_CONTAINER_H

#include "platform.hh"
#include "types.hh"
#include "json.hh"

namespace SSC {
  // forward
  namespace IPC { class Bridge; }
  class Core;

  class ServiceWorkerContainer {
    public:
      using ID = uint64_t;

      struct RegistrationOptions {
        enum class Type { Classic, Module };

        Type type = Type::Module;
        String scope;
        String scriptURL;
        String scheme = "*";
        ID id = 0;
      };

      struct Client {
        ID id = 0;
        String preload;
      };

      struct Registration {
        enum class State {
          None,
          Error,
          Registered,
          Installing,
          Installed,
          Activating,
          Activated
        };

        ID id = 0;
        String scriptURL;
        Atomic<State> state = State::None;
        RegistrationOptions options;

        Registration (
          const ID id,
          const String& scriptURL,
          const State state,
          const RegistrationOptions& options
        );

        // les 5
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

      struct FetchBuffer {
        size_t size = 0;
        char* bytes = nullptr;
      };

      struct FetchRequest {
        String method;
        String scheme = "*";
        String host = "";
        String pathname;
        String query;
        Vector<String> headers;
        FetchBuffer buffer;
        Client client;

        const String str () const;
      };

      struct FetchResponse {
        uint64_t id = 0;
        int statusCode = 200;
        Vector<String> headers;
        FetchBuffer buffer;
        Client client;
      };

      using Registrations = std::map<String, Registration>;
      using FetchCallback = std::function<void(const FetchResponse)>;
      using FetchCallbacks = std::map<uint64_t, FetchCallback>;
      using FetchRequests = std::map<uint64_t, FetchRequest>;

      Mutex mutex;
      Core* core = nullptr;
      IPC::Bridge* bridge = nullptr;
      Registrations registrations;
      AtomicBool isReady = false;

      FetchRequests fetchRequests;
      FetchCallbacks fetchCallbacks;

      ServiceWorkerContainer (Core* core);
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
