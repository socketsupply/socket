#ifndef SOCKET_RUNTIME_CORE_SERVICES_PROCESS_H
#define SOCKET_RUNTIME_CORE_SERVICES_PROCESS_H

#include "../../ipc.hh"
#include "../../core.hh"
#include "../../process.hh"
#include "../../queued_response.hh"

#include "timers.hh"

namespace ssc::runtime::core::services {
  class Process : public core::Service {
    public:
      using ID = uint64_t;
      using Handles = Map<ID, SharedPointer<runtime::Process>>;

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
      Timers timers;
      Mutex mutex;

      Process (const Options& options)
        : core::Service(options),
          timers(options)
      {}

      void shutdown ();
      void exec (const ipc::Message::Seq&, ID, const Vector<String>, const ExecOptions, const Callback);
      void spawn (const ipc::Message::Seq&, ID, const Vector<String>, const SpawnOptions, const Callback);
      void kill (const ipc::Message::Seq&, ID, int, const Callback);
      void write (const ipc::Message::Seq&, ID, SharedPointer<char[]>, size_t, const Callback);
  };
}
#endif
