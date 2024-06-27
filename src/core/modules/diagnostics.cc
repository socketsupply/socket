#include "../core.hh"
#include "diagnostics.hh"

namespace SSC {
  JSON::Object CoreDiagnostics::Diagnostic::Handles::json () const {
    auto ids = JSON::Array {};
    for (const auto id : this->ids) {
      ids.push(std::to_string(id));
    }
    return JSON::Object::Entries {
      {"count", this->count},
      {"ids", ids}
    };
  }

  void CoreDiagnostics::query (const QueryCallback& callback) const {
    this->core->dispatchEventLoop([=, this] () mutable {
      auto query = QueryDiagnostic {};

      // posts diagnostics
      do {
        Lock lock(this->core->postsMutex);
        query.posts.handles.count = this->core->posts.size();
        for (const auto& entry : this->core->posts) {
          query.posts.handles.ids.push_back(entry.first);
        }
      } while (0);

    #if !SOCKET_RUNTIME_PLATFORM_IOS
      // `childProcess` diagnostics
      do {
        Lock lock(this->core->childProcess.mutex);
        query.childProcess.handles.count = this->core->childProcess.handles.size();
        for (const auto& entry : this->core->childProcess.handles) {
          query.childProcess.handles.ids.push_back(entry.first);
        }
      } while (0);
    #endif

      // ai diagnostics
      do {
        Lock lock(this->core->ai.mutex);
        query.ai.llm.handles.count = this->core->ai.llms.size();
        for (const auto& entry : this->core->ai.llms) {
          query.ai.llm.handles.ids.push_back(entry.first);
        }
      } while (0);

      // fs diagnostics
      do {
        Lock lock(this->core->fs.mutex);
        query.fs.descriptors.handles.count = this->core->fs.descriptors.size();
        query.fs.watchers.handles.count = this->core->fs.watchers.size();

        for (const auto& entry : this->core->fs.descriptors) {
          query.fs.descriptors.handles.ids.push_back(entry.first);
        }

        for (const auto& entry : this->core->fs.watchers) {
          query.fs.watchers.handles.ids.push_back(entry.first);
        }
      } while (0);

      // timers diagnostics
      do {
        Lock lock(this->core->timers.mutex);
        for (const auto& entry : this->core->timers.handles) {
          const auto id = entry.first;
          const auto& timer = entry.second;
          if (timer->type == CoreTimers::Timer::Type::Timeout) {
            query.timers.timeout.handles.count++;
            query.timers.timeout.handles.ids.push_back(entry.first);
          } else if (timer->type == CoreTimers::Timer::Type::Interval) {
            query.timers.interval.handles.count++;
            query.timers.interval.handles.ids.push_back(entry.first);
          } else if (timer->type == CoreTimers::Timer::Type::Immediate) {
            query.timers.immediate.handles.count++;
            query.timers.immediate.handles.ids.push_back(entry.first);
          }
        }
      } while (0);

      // udp
      do {
        Lock lock(this->core->udp.mutex);
        query.udp.handles.count = this->core->udp.sockets.size();
        for (const auto& entry : this->core->udp.sockets) {
          query.udp.handles.ids.push_back(entry.first);
        }
      } while (0);

      // uv
      do {
        Lock lock(this->core->loopMutex);
        uv_metrics_info(&this->core->eventLoop, &query.uv.metrics);
        query.uv.idleTime = uv_metrics_idle_time(&this->core->eventLoop);
        query.uv.handles.count = this->core->eventLoop.active_handles;
        query.uv.activeRequests = this->core->eventLoop.active_reqs.count;
      } while (0);

      callback(query);
    });
  }

  void CoreDiagnostics::query (
    const String& seq,
    const CoreModule::Callback& callback
  ) const {
    this->core->dispatchEventLoop([=, this] () {
      this->query([=] (const auto query) {
        auto json = JSON::Object::Entries {
          {"source", "diagnostics.query"},
          {"data", query.json()}
        };
        callback(seq, json, Post {});
      });
    });
  }

  JSON::Object CoreDiagnostics::UVDiagnostic::json () const {
    return JSON::Object::Entries {
      {"metrics", JSON::Object::Entries {
        {"loopCount", this->metrics.loop_count},
        {"events", this->metrics.events},
        {"eventsWaiting", this->metrics.events_waiting},
      }},
      {"idleTime", this->idleTime},
      {"activeRequests", this->activeRequests}
    };
  }

  JSON::Object CoreDiagnostics::PostsDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::ChildProcessDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::AIDiagnostic::json () const {
    return JSON::Object::Entries {
      {"llm", this->llm.json()}
    };
  }

  JSON::Object CoreDiagnostics::AIDiagnostic::LLMDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::FSDiagnostic::json () const {
    return JSON::Object::Entries {
      {"watchers", this->watchers.json()},
      {"descriptors", this->descriptors.json()}
    };
  }

  JSON::Object CoreDiagnostics::FSDiagnostic::WatchersDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::FSDiagnostic::DescriptorsDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::TimersDiagnostic::json () const {
    return JSON::Object::Entries {
      {"timeout", this->timeout.json()},
      {"interval", this->interval.json()},
      {"immediate", this->immediate.json()}
    };
  }

  JSON::Object CoreDiagnostics::TimersDiagnostic::TimeoutDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::TimersDiagnostic::IntervalDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::TimersDiagnostic::ImmediateDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::UDPDiagnostic::json () const {
    return JSON::Object::Entries {
      {"handles", this->handles.json()}
    };
  }

  JSON::Object CoreDiagnostics::QueryDiagnostic::json () const {
    return JSON::Object::Entries {
      {"posts", this->posts.json()},
      {"childProcess", this->childProcess.json()},
      {"ai", this->ai.json()},
      {"fs", this->fs.json()},
      {"timers", this->timers.json()},
      {"udp", this->udp.json()},
      {"uv", this->uv.json()}
    };
  }
}
