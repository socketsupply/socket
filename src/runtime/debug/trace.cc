#include "../debug.hh"

namespace ssc::runtime::debug {
  Tracer::Tracer (const String& name)
    : name(name),
      spans(std::make_shared<SharedSpanCollection>())
  {}

  Tracer::Tracer (const Tracer& tracer)
    : name(tracer.name),
      spans(tracer.spans)
  {}

  Tracer::Tracer (Tracer&& tracer) noexcept
    : name(std::move(tracer.name)),
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
      this->name = std::move(tracer.name);
      this->spans = std::move(tracer.spans);
      tracer.name = "";
      tracer.spans = nullptr;
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
        return this->spans->at(i);
      }
    }

    return nullptr;
  }

  size_t Tracer::size (bool onlyActive) const noexcept {
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
      {"id", this->id},
      {"name", this->name},
      {"spans", spans},
      {"type", "Tracer"}
    };
  }

  const Tracer::SharedSpan Tracer::begin (const String& name) {
    Lock lock(this->mutex);

    for (const auto& span : *this->spans) {
      if (span->name == name) {
        return span;
      }
    }

    return this->span(name);
  }

  bool Tracer::end (const String& name) {
    Lock lock(this->mutex);
    for (const auto& span : *this->spans) {
      if (span->name == name) {
        span->end();
        return true;
      }
    }

    return false;
  }

  const Tracer::Iterator Tracer::begin () const noexcept {
    return this->spans->begin();
  }

  const Tracer::Iterator Tracer::end () const noexcept {
    return this->spans->end();
  }

  const bool Tracer::clear () noexcept {
    Lock lock(this->mutex);
    if (this->spans->size() > 0) {
      this->spans->clear();
      return true;
    }
    return false;
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
      {"duration", duration},
      {"type", "Timing"}
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
      {"id", this->id},
      {"name", this->name},
      {"timing", this->timing.json()},
      {"spans", spans},
      {"type", "Span"},
      {"tracer", JSON::Object::Entries {
        {"id", this->tracer.id},
        {"name", this->tracer.name}
      }}
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
