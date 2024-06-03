#ifndef SOCKET_RUNTIME_CORE_MODULE_PLATFORM_H
#define SOCKET_RUNTIME_CORE_MODULE_PLATFORM_H

#include "../module.hh"

namespace SSC {
  class Core;
  class CorePlatform : public CoreModule {
    public:
      Atomic<bool> wasFirstDOMContentLoadedEventDispatched = false;

      CorePlatform (Core* core)
        : CoreModule(core)
      {}

      void event (
        const String& seq,
        const String& event,
        const String& data,
        const String& frameType,
        const String& frameSource,
        const CoreModule::Callback& callback
      );

      void notify (
        const String& seq,
        const String& title,
        const String& body,
        const CoreModule::Callback& callback
      ) const;

      void openExternal (
        const String& seq,
        const String& value,
        const CoreModule::Callback& callback
      ) const;

      void revealFile (
        const String& seq,
        const String& value,
        const CoreModule::Callback& callback
      ) const;
  };
}
#endif
