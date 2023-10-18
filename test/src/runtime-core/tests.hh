#ifndef RUNTIME_CORE_TESTS_H
#define RUNTIME_CORE_TESTS_H

#include <functional>

#include <socket/extension.h>
#include "src/core/core.hh"

#undef assert

namespace SSC::Tests {
  class Harness;
  typedef void (TestRunner)(Harness& harness);

  class Harness {
    public:
      using Mutex = std::mutex;
      struct Options {
        bool resetContextAfterEachRun = false;
        bool isAsync = false;
        int pending = 0;
      };

      const Options options;
      Mutex mutex;
      Atomic<bool> isAsync = false;
      Atomic<int> pending = 0;
      Atomic<unsigned int> planned = 0;

      Harness ();
      Harness (const Options& options);

      bool assert (bool assertion, const String& message = "") const;
      bool assert (int64_t value, const String& message = "") const;
      bool assert (double value, const String& message = "") const;
      bool assert (void* value, const String& message = "") const;
      bool assert (const String& value, const String& message) const;

      bool equals (const String& left, const String& right, const String& message) const;
      bool equals (const char* left, const char* right, const String& message) const;
      bool equals (bool left, bool right, const String& message) const;
      bool equals (int64_t left, int64_t right, const String& message) const;
      bool equals (double left, double right, const String& message) const;
      bool equals (size_t  left, size_t right, const String& message) const;

      bool notEquals (const String& left, const String& right, const String& message) const;
      bool notEquals (const char* left, const char* right, const String& message) const;
      bool notEquals (int64_t left, int64_t right, const String& message) const;
      bool notEquals (double left, double right, const String& message) const;
      bool notEquals (size_t left, size_t right, const String& message) const;

      bool throws (std::function<void()> fn, const String& message) const;

      void comment (const String& comment) const;
      void label (const String& label) const;
      void log (const String& message) const;
      void log (const Map& message) const;
      void log (const Vector<String>& message) const;

      void plan (unsigned int count);

      bool test (const String& label, TestRunner scope);
      bool test (const String& label, bool isAsync, TestRunner scope);
      bool run (bool isAsync, const String& label, TestRunner runner);
      bool run (bool isAsync, TestRunner runner);
      bool run (TestRunner);
      void end ();
      void wait ();
  };

  // tests
  void codec (Harness&);
  void config (Harness&);
  void env (Harness&);
  void ini (Harness&);
  void json (Harness&);
  void platform (Harness&);
  void preload (Harness&);
  void string (Harness&);
  void version (Harness&);
}

#endif
