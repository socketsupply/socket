#ifndef SSC_PROTOCOL_HANDLERS_H
#define SSC_PROTOCOL_HANDLERS_H

#include "platform.hh"
#include "types.hh"
#include "json.hh"

namespace SSC {
  // forward
  namespace IPC { class Bridge; }
  class Core;

  class ProtocolHandlers {
    public:
      struct Data {
        String json = ""; // we store JSON as a string here
      };

      struct Protocol {
        String scheme;
        Data data;
      };

      using Mapping = std::map<String, Protocol>;

      Mapping mapping;
      Mutex mutex;
      Core* core = nullptr;

      ProtocolHandlers (Core* core);
      ~ProtocolHandlers ();

      bool registerHandler (const String& scheme, const Data data = { "" });
      bool unregisterHandler (const String& scheme);
      const Data getHandlerData (const String& scheme);
      bool setHandlerData (const String& scheme, const Data data = { "" });
      bool hasHandler (const String& scheme);
  };
}
#endif
