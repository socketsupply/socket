#ifndef RUNTIME_CORE_TESTS_H
#define RUNTIME_CORE_TESTS_H

#include <functional>

#include <socket/extension.h>
#include "src/core/types.hh"
#include "./ok.hh"

namespace SSC::Tests {
  class Harness;
  typedef void (TestRunner)(const Harness& harness);

  class Harness {
    public:
      Harness ();

      bool assert (bool assertion, const String& message = "") const;
      bool assert (int64_t value, const String& message = "") const;
      bool assert (double value, const String& message = "") const;
      bool assert (void* value, const String& message = "") const;
      bool assert (const String& value, const String& message) const;
      bool equals (const char* left, const char* right, const String& message) const;
      bool equals (const String& left, const String& right, const String& message) const;
      bool equals (const int64_t left, const int64_t right, const String& message) const;
      bool equals (const double left, const double right, const String& message) const;
      bool throws (std::function<void()> fn, const String& message) const;

      void comment (const String& comment) const;
      void label (const String& label) const;
      bool test (const String& label, TestRunner scope) const;
      bool test (const String& label, bool isAsync, TestRunner scope) const;
      void log (const String& message) const;
      bool run (bool isAsync, const String& label, TestRunner runner) const;
      bool run (bool isAsync, TestRunner runner) const;
      bool run (TestRunner) const;
      void end () const;
  };

  // tests
  void codec (const Harness&);
  void config (const Harness&);
  void env (const Harness&);
  void ini (const Harness&);
  void json (const Harness&);
  void platform (const Harness&);
  void preload (const Harness&);
  void version (const Harness&);
}

#endif
