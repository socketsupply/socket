#include "diagnostics.hh"
#include "../services.hh"

namespace ssc::runtime::core::services {
  JSON::Object Diagnostics::Diagnostic::Handles::json () const {
    auto ids = JSON::Array {};
    for (const auto id : this->ids) {
      ids.push(std::to_string(id));
    }
    return JSON::Object::Entries {
      {"count", this->count},
      {"ids", ids}
    };
  }

  void Diagnostics::query (const QueryCallback callback) const {
    this->loop.dispatch([=, this] () mutable {
      auto query = QueryDiagnostic {};

      // queued responses diagnostics
      do {
        Lock lock(this->services.mutex);
        query.queuedResponses.handles.count = this->context.queuedResponses.size();
        for (const auto& entry : this->context.queuedResponses) {
          query.queuedResponses.handles.ids.push_back(entry.first);
        }
      } while (0);

    #if !SOCKET_RUNTIME_PLATFORM_IOS
      // `childProcess` diagnostics
      do {
        Lock lock(this->services.process.mutex);
        query.childProcess.handles.count = this->services.process.handles.size();
        for (const auto& entry : this->services.process.handles) {
          query.childProcess.handles.ids.push_back(entry.first);
        }
      } while (0);
    #endif

      // ai diagnostics
      do {
        Lock lock(this->services.ai.mutex);
        query.ai.llm.handles.count = this->services.ai.llms.size();
        for (const auto& entry : this->services.ai.llms) {
          query.ai.llm.handles.ids.push_back(entry.first);
        }
      } while (0);

      // fs diagnostics
      do {
        Lock lock(this->services.fs.mutex);
        query.fs.descriptors.handles.count = this->services.fs.descriptors.size();
        query.fs.watchers.handles.count = this->services.fs.watchers.size();

        for (const auto& entry : this->services.fs.descriptors) {
          query.fs.descriptors.handles.ids.push_back(entry.first);
        }

        for (const auto& entry : this->services.fs.watchers) {
          query.fs.watchers.handles.ids.push_back(entry.first);
        }
      } while (0);

      // timers diagnostics
      do {
        Lock lock(this->services.timers.mutex);
        for (const auto& entry : this->services.timers.handles) {
          const auto id = entry.first;
          const auto& timer = entry.second;
          if (timer->type == Timers::Timer::Type::Timeout) {
            query.timers.timeout.handles.count++;
            query.timers.timeout.handles.ids.push_back(entry.first);
          } else if (timer->type == Timers::Timer::Type::Interval) {
            query.timers.interval.handles.count++;
            query.timers.interval.handles.ids.push_back(entry.first);
          } else if (timer->type == Timers::Timer::Type::Immediate) {
            query.timers.immediate.handles.count++;
            query.timers.immediate.handles.ids.push_back(entry.first);
          }
        }
      } while (0);

      // udp
      do {
        Lock lock(this->services.udp.mutex);
        query.udp.handles.count = this->services.udp.manager.sockets.size();
        for (const auto& entry : this->services.udp.manager.sockets) {
          query.udp.handles.ids.push_back(entry.first);
        }
      } while (0);

      // conduit
      do {
        Lock lock(this->services.conduit.mutex);
        query.conduit.handles.count = this->services.conduit.clients.size();
        query.conduit.isActive = this->services.conduit.isActive();
        for (const auto& entry : this->services.conduit.clients) {
          query.conduit.handles.ids.push_back(entry.first);
        }
      } while (0);

      // uv
      do {
        Lock lock(this->loop.mutex);
        uv_metrics_info(this->loop.get(), &query.uv.metrics);
        query.uv.idleTime = uv_metrics_idle_time(this->loop.get());
        query.uv.handles.count = this->loop.get()->active_handles;
        query.uv.activeRequests = this->loop.get()->active_reqs.count;
      } while (0);

      callback(query);
    });
  }

  void Diagnostics::query (
    const String& seq,
    const Callback callback
  ) const {
    this->loop.dispatch([=, this] () {
      this->query([=] (const auto query) {
        auto json = JSON::Object::Entries {
          {"source", "diagnostics.query"},
          {"data", query.json()}
        };
        callback(seq, json, QueuedResponse {});
      });
    });
  }

  JSON::Object Diagnostics::UVDiagnostic::json () const {
    return JSON::Object::Entries {
      {"metrics", JSON::Object::Entries {
        {"loopCount", this->metrics.loop_count},
        {"events", this->metrics.events},
        {"eventsWaiting", this->metrics.events_waiting},
      }},
      {"idleTime", this->idleTime},
      {"activeRequests", this->activeRequests},
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::QueuedResponsesDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::ChildProcessDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::AIDiagnostic::json () const {
    return JSON::Object::Entries {
      {"llm", this->llm.json()}
    };
  }

  JSON::Object Diagnostics::AIDiagnostic::LLMDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::FSDiagnostic::json () const {
    return JSON::Object::Entries {
      {"watchers", this->watchers.json()},
      {"descriptors", this->descriptors.json()}
    };
  }

  JSON::Object Diagnostics::FSDiagnostic::WatchersDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::FSDiagnostic::DescriptorsDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::TimersDiagnostic::json () const {
    return JSON::Object::Entries {
      {"timeout", this->timeout.json()},
      {"interval", this->interval.json()},
      {"immediate", this->immediate.json()}
    };
  }

  JSON::Object Diagnostics::TimersDiagnostic::TimeoutDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::TimersDiagnostic::IntervalDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::TimersDiagnostic::ImmediateDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::UDPDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object Diagnostics::ConduitDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()},
      {"isActive", this->isActive}
    };
  }

  JSON::Object Diagnostics::QueryDiagnostic::json () const {
    return JSON::Object::Entries {
      {"queuedResponses", this->queuedResponses.json()},
      {"childProcess", this->childProcess.json()},
      {"ai", this->ai.json()},
      {"fs", this->fs.json()},
      {"timers", this->timers.json()},
      {"udp", this->udp.json()},
      {"uv", this->uv.json()},
      {"conduit", this->conduit.json()}
    };
  }
}
