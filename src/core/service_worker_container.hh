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
      struct RegistrationOptions {
        enum class Type { Classic, Module };

        Type type = Type::Module;
        String scope;
        String scriptURL;
      };

      struct Client {
        uint64_t id = 0;
      };

      struct Registration {
        enum class State {
          Error,
          Registered,
          Installing,
          Installed,
          Activating,
          Activated
        };

        uint64_t id = 0;
        String scriptURL;
        State state = State::Registered;
        RegistrationOptions options;
        Vector<Client> clients;
        const SSC::JSON::Object json () const;
      };

      struct FetchBuffer {
        size_t size = 0;
        char* bytes = nullptr;
      };

      struct FetchRequest {
        String pathname;
        String query;
        Vector<String> headers;
        Client client;
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

      Core* core = nullptr;
      IPC::Bridge* bridge = nullptr;
      Registrations registrations;
      FetchCallbacks fetches;

      ServiceWorkerContainer (Core* core);
      ~ServiceWorkerContainer ();

      void init (IPC::Bridge* bridge);
      const Registration registerServiceWorker (const RegistrationOptions& options);
      bool unregisterServiceWorker (uint64_t id);
      bool unregisterServiceWorker (String scopeOrScriptURL);
      void skipWaiting (uint64_t id);
      void updateState (uint64_t id, const String& stateString);

      bool fetch (FetchRequest request, FetchCallback callback);
  };
}

#endif
