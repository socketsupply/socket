#ifndef SOCKET_RUNTIME_CORE_SERVICES_DIAGNOSTICS_H
#define SOCKET_RUNTIME_CORE_SERVICES_DIAGNOSTICS_H

#include "../../core.hh"
#include "../../ipc.hh"

namespace ssc::runtime::core::services {
  class Diagnostics : public core::Service {
    public:
      struct Diagnostic {
        using ID = uint64_t;
        struct Handles {
          size_t count = 0;
          Vector<ID> ids;
          JSON::Object json () const;
        };

        virtual JSON::Object json () const = 0;
      };

      struct UVDiagnostic : public Diagnostic {
        uv_metrics_t metrics; // various uv metrics
        Handles handles; // active uv loop handles
        uint64_t idleTime = 0;
        uint64_t activeRequests = 0;
        JSON::Object json () const override;
      };

      struct QueuedResponsesDiagnostic : public Diagnostic {
        Handles handles;
        JSON::Object json () const override;
      };

      struct ChildProcessDiagnostic : public Diagnostic {
        Handles handles;
        JSON::Object json () const override;
      };

      struct AIDiagnostic : public Diagnostic {
        struct LLMDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        LLMDiagnostic llm;

        JSON::Object json () const override;
      };

      struct FSDiagnostic : public Diagnostic {
        struct WatchersDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        struct DescriptorsDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        WatchersDiagnostic watchers;
        DescriptorsDiagnostic descriptors;
        JSON::Object json () const override;
      };

      struct TimersDiagnostic : public Diagnostic {
        struct TimeoutDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        struct IntervalDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        struct ImmediateDiagnostic : public Diagnostic {
          Handles handles;
          JSON::Object json () const override;
        };

        TimeoutDiagnostic timeout;
        IntervalDiagnostic interval;
        ImmediateDiagnostic immediate;

        JSON::Object json () const override;
      };

      struct UDPDiagnostic : public Diagnostic {
        Handles handles;
        JSON::Object json () const override;
      };

      struct ConduitDiagnostic : public Diagnostic {
        Handles handles;
        bool isActive;
        JSON::Object json () const override;
      };

      struct QueryDiagnostic : public Diagnostic {
        QueuedResponsesDiagnostic queuedResponses;
        ChildProcessDiagnostic childProcess;
        AIDiagnostic ai;
        FSDiagnostic fs;
        TimersDiagnostic timers;
        UDPDiagnostic udp;
        UVDiagnostic uv;
        ConduitDiagnostic conduit;

        JSON::Object json () const override;
      };

      using QueryCallback = Function<void(const QueryDiagnostic)>;

      Diagnostics (const Options& options)
        : core::Service(options)
      {}

      void query (const QueryCallback) const;
      void query (const ipc::Message::Seq&, const Callback) const;
  };
}
#endif
