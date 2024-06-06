#ifndef SOCKET_RUNTIME_CORE_MODULE_CHILD_PROCESS_H
#define SOCKET_RUNTIME_CORE_MODULE_CHILD_PROCESS_H

#include "../module.hh"
#include "../process.hh"

namespace SSC {
  class Core;
  class CoreChildProcess : public CoreModule {
    public:
      using ID = uint64_t;
      using Handles = std::map<ID, SharedPointer<Process>>;

      struct SpawnOptions {
        String cwd;
        const Vector<String> env;
        bool allowStdin = true;
        bool allowStdout = true;
        bool allowStderr = true;
      };

      struct ExecOptions {
        String cwd;
        const Vector<String> env;
        bool allowStdout = true;
        bool allowStderr = true;
        uint64_t timeout = 0;
      #if SOCKET_RUNTIME_PLATFORM_WINDOWS || SOCKET_RUNTIME_PLATFORM_IOS
        int killSignal = 0; // unused
      #else
        int killSignal = SIGTERM;
      #endif
      };

      Handles handles;
      Mutex mutex;

      CoreChildProcess (Core* core)
        : CoreModule(core)
      {}

      void shutdown ();
      void exec (
        const String& seq,
        ID id,
        const Vector<String> args,
        const ExecOptions options,
        const CoreModule::Callback& callback
      );

      void spawn (
        const String& seq,
        ID id,
        const Vector<String> args,
        const SpawnOptions options,
        const CoreModule::Callback& callback
      );

      void kill (
        const String& seq,
        ID id,
        int signal,
        const CoreModule::Callback& callback
      );

      void write (
        const String& seq,
        ID id,
        SharedPointer<char[]> buffer,
        size_t size,
        const CoreModule::Callback& callback
      );
  };
}
#endif
