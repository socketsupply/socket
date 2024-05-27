#include "trace.hh"

namespace SSC {
  Tracer::Tracer (const String& name)
    : name(name),
      spans(std::make_shared<SharedSpanCollection>())
  {}

  Tracer::Tracer (const Tracer& tracer)
    : name(tracer.name),
      spans(tracer.spans)
  {}

  Tracer::Tracer (Tracer&& tracer) noexcept
    : name(tracer.name),
      spans(std::move(tracer.spans))
  {}

  Tracer& Tracer::operator= (const Tracer& tracer) {
    if (this != &tracer) {
      this->name = tracer.name;
      this->spans = tracer.spans;
    }
    return *this;
  }

  Tracer& Tracer::operator= (Tracer&& tracer) noexcept {
    if (this != &tracer) {
      this->name = tracer.name;
      this->spans = std::move(tracer.spans);
    }
    return *this;
  }

  Tracer::SharedSpan Tracer::span (const String& name, const Span::ID id) {
    auto span = std::make_shared<Span>(*this, name);

    if (id > 0) {
      span->id = id;
    }

    do {
      Lock lock(this->mutex);
      const auto i = this->spans->size();
      this->spans->emplace_back(span);
      this->index[span->id] = i;
    } while (0);

    return span;
  }

  Tracer::SharedSpan Tracer::span (const Span::ID id) {
    Lock lock(this->mutex);
    if (this->index.contains(id)) {
      const auto i = this->index[id];
      if (i < this->spans->size()) {
        return this->spans->at(id);
      }
    }

    return nullptr;
  }

  size_t Tracer::size (bool onlyActive) const {
    size_t count = 0;
    for (const auto& span : *this->spans) {
      if (onlyActive && !span->ended) {
        continue;
      }

      count++;
    }

    return count;
  }

  JSON::Object Tracer::json () const {
    JSON::Array spans;

    for (const auto& span : *this->spans) {
      // only top level spans
      if (span->parent == nullptr) {
        spans.push(span->json());
      }
    }

    return JSON::Object::Entries {
      {"name", this->name},
      {"spans", spans}
    };
  }

  Tracer::Duration Tracer::Timing::now () {
    using namespace std::chrono;
    return duration_cast<Duration>(system_clock::now().time_since_epoch());
  }

  void Tracer::Timing::stop () {
    using namespace std::chrono;
    this->end = TimePoint(Timing::now());
    this->duration = duration_cast<milliseconds>(this->end.load() - this->start.load());
  }

  JSON::Object Tracer::Timing::json () const {
    using namespace std::chrono;
    const auto duration = duration_cast<milliseconds>(this->duration.load()).count();
    const auto start = time_point_cast<milliseconds>(this->start.load()).time_since_epoch().count();
    const auto end = time_point_cast<milliseconds>(this->end.load()).time_since_epoch().count();
    return JSON::Object::Entries {
      {"start", start},
      {"end", end},
      {"duration", duration}
    };
  }

  Tracer::Timing::Timing ()
    : start(TimePoint(Timing::now()))
  {}

  Tracer::Span::Span (Tracer& tracer, const String& name)
    : tracer(tracer),
      name(name)
  {}

  Tracer::Span::~Span () {
    this->end();
  }

  bool Tracer::Span::end () {
    if (this->ended) {
      return false;
    }

    this->timing.stop();
    this->ended = true;
    return true;
  }

  JSON::Object Tracer::Span::json () const {
    JSON::Array spans;

    for (const auto id : this->spans) {
      const auto span = this->tracer.span(id);
      if (span != nullptr) {
        spans.push(span->json());
      }
    }

    return JSON::Object::Entries {
      {"name", this->name},
      {"timing", this->timing.json()},
      {"spans", spans}
    };
  }

  Tracer::SharedSpan Tracer::Span::span (const String& name, const Span::ID id) {
    auto span = this->tracer.span(name, id);
    span->parent = this;
    return span;
  }

  long Tracer::Span::duration () const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(this->timing.duration.load()).count();
  }
}
