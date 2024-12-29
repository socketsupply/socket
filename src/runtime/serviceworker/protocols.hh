#ifndef SOCKET_RUNTIME_SERVICE_WORKER_PROTOCOLS_H
#define SOCKET_RUNTIME_SERVICE_WORKER_PROTOCOLS_H

#include "../core/json.hh"

namespace SSC {
  class ServiceWorkerContainer;
  class ServiceWorkerProtocols {
    public:
      struct Data {
        String json = ""; // we store JSON as a string here
      };

      struct Protocol {
        String scheme;
        Data data;
      };

      using Mapping = Map<String, Protocol>;

      ServiceWorkerContainer* serviceWorkerContainer;
      Mapping mapping;
      Mutex mutex;

      ServiceWorkerProtocols (ServiceWorkerContainer* serviceWorkerContainer);
      ~ServiceWorkerProtocols ();

      bool registerHandler (const String& scheme, const Data data = { "" });
      bool unregisterHandler (const String& scheme);
      const Data getHandlerData (const String& scheme);
      bool setHandlerData (const String& scheme, const Data data = { "" });
      bool hasHandler (const String& scheme);
      const String getServiceWorkerScope (const String& scheme);
      const Vector<String> getSchemes () const;
      const Vector<String> getProtocols () const;
  };
}
#endif
