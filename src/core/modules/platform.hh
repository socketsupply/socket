#ifndef SOCKET_RUNTIME_CORE_MODULE_PLATFORM_H
#define SOCKET_RUNTIME_CORE_MODULE_PLATFORM_H

#include "../module.hh"

#if SOCKET_RUNTIME_PLATFORM_ANDROID
#include "../../platform/android.hh"
#endif

namespace SSC {
  class Core;
  class CorePlatform : public CoreModule {
    public:
      Atomic<bool> wasFirstDOMContentLoadedEventDispatched = false;

    #if SOCKET_RUNTIME_PLATFORM_ANDROID
      Android::JVMEnvironment jvm;
      Android::Activity activity = nullptr;
    #endif

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
