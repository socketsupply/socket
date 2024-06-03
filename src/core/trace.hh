#ifndef SOCKET_RUNTIME_CORE_TRACE_H
#define SOCKET_RUNTIME_CORE_TRACE_H

#include "../platform/platform.hh"
#include "json.hh"

namespace SSC {
  /**
   * The `Tracer` class manages multiple `Tracer::Span` instances, allowing
   * spans to be created and tracked across multiple threads.
   * It ensures thread-safe access to the spans and provides support for
   * copying and moving, making it suitable for use in async and
   * multi-threaded contexts.
   */
  class Tracer {
    public:
      // forward
      class Span;

      /**
       * A high resolution time time point used by the `Tracer::Span` class.
       */
      using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;
      /**
       * A tracer duration quantized to milliseconds
       */
      using Duration = std::chrono::milliseconds;

      /**
       * A shared pointer `Tracer::Span` type
       */
      using SharedSpan = SharedPointer<Span>;

      /**
       * A collection of `SharedSpan` instances
       */
      using SharedSpanCollection = Vector<SharedSpan>;

      /**
       * A mapping type of `Span::ID` to `Vector` index for fast span access
       */
      using SharedSpanColletionIndex = std::map<uint64_t, size_t>;

      /**
       * A `Tracer` ID type
       */
      using ID = uint64_t;

      /**
       * Iterator for a `Tracer`
       */
      using Iterator = SharedSpanCollection::const_iterator;

      /**
       * A container for `Tracer` timing.
       */
      struct Timing {
        Atomic<TimePoint> start;
        Atomic<TimePoint> end;
        Atomic<Duration> duration;
        static Duration now ();
        Timing ();
        void stop ();
        JSON::Object json () const;
      };

      /**
       * The `Tracer::Span` class represents a "time span", tracking its start
       * time and providing the ability to end it, either automatically upon
       * destruction or manually via the end() method. It is thread-safe and
       * ensures that the duration is printed only once.
       */
      class Span {
        public:
          using ID = uint64_t;

          /**
           * Iterator for a `Span`
           */
          using Iterator = Vector<SharedSpan>::const_iterator;

          /**
           * A unique ID for this `Span`.
           */
          ID id = rand64();

          /**
           * The name of this span
           */
          String name;

          /**
           * The span timing.
           */
          Timing timing;

          /**
           * This value is `true` if the span has ended
           */
          Atomic<bool> ended = false;

          /**
           * A strong reference to the `Tracer` that created this `Span`.
           */
          Tracer& tracer;

          /**
           * A weak pointer to a parent `Span` that created this `Span`.
           * This value may be `nullptr`.
           */
          Span* parent = nullptr;

          /**
           * A vector of `Span::ID` that point to a `Span` instance in a
           * `Tracer`.
           */
          Vector<ID> spans;

          /**
           * Initializes a new span with the given name and starts the timer.
           */
          Span (Tracer& tracer, const String& name);

          /**
           * Ends the span, storing the duration if it has not been ended
           * already.
           */
          ~Span ();

          /**
           * Ends the span, storing the duration, and ensures thread-safe
           * access. If the span has already been ended, this method does
           * nothing.
           *
           * This function returns `true` if the span ended for the first time,
           * otherwise `false`
           */
          bool end ();

          /**
           * Computed JSON representation of this `Tracer::Span` instance.
           */
          JSON::Object json () const;

          /**
           * Creates a new `Span` with the given name, adds it to the
           * collection of spans, and returns a "shared span" (a shared pointer
           * to the span). This functinon ensures thread-safe access to the
           * collection. The returned `Span` is a "child" of this `Span`.
           */
          SharedSpan span (const String& name, const ID id = 0);

          /**
           * Returns the computed duration for an "ended" `Span`
           */
          long duration () const;
      };

      /**
       * A collection of shared spans owned by this `Tracer` instance.
       */
      SharedPointer<SharedSpanCollection> spans = nullptr;

      /**
       * The shared span collection index
       */
      SharedSpanColletionIndex index;

      /**
       * Used for thread synchronization
       */
      Mutex mutex;

      /**
       * The name of the `Tracer` instance.
       */
      String name;

      /**
       * A unique ID for this `Tracer`.
       */
      ID id = rand64();

      /**
       * Initializes a new Tracer instance with an empty collection of spans.
       */
      Tracer (const String& name);
      // copy
      Tracer (const Tracer&);
      // move
      Tracer (Tracer&&) noexcept;
      ~Tracer () = default;

      // copy
      Tracer& operator= (const Tracer&);
      // move
      Tracer& operator= (Tracer&&) noexcept;

      /**
       * Creates a new `Tracer::Span` with the given name, adds it to the
       * collection of spans, and returns a "shared span" (a shared pointer
       * to the span). This functinon ensures thread-safe access to the
       * collection.
       */
      SharedSpan span (const String& name, const Span::ID id = 0);

      /**
       * Gets a span by `Span::ID`. This function _will not_ create a new
       * `Span`, but instead return a "null" `SharedSpan`.
       */
      SharedSpan span (const Span::ID id);

      /**
       * The number of spans in this tracer. This function accepts an
       * `onlyActive` boolean to get the computed "active" (not ended)
       * spans in a trace.
       */
      size_t size (bool onlyActive = false) const noexcept;

      /**
       * Computed JSON representation of this `Tracer` instance and its
       * `Tracer::Span` children.
       */
      JSON::Object json () const;

      /**
       * Create or return a span by name to "begin" a span
       */
      const SharedSpan begin (const String& name);

      /**
       * Ends a named span if one exists.
       */
      bool end (const String& name);

      /**
       * Get the beginning of iterator to the vector `Span` instances.
       */
      const Iterator begin () const noexcept;

      /**
       * Get the end of iterator to the vector of `Span` instances.
       */
      const Iterator end () const noexcept;

      /**
       * Clears all `Span` entries in the `Tracer`.
       */
      const bool clear () noexcept;
  };
}

#endif
