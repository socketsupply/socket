#ifndef RUNTIME_CORE_TESTS_H
#define RUNTIME_CORE_TESTS_H

#include <functional>

#include <socket/extension.h>
#include "src/core/core.hh"
#include "harness.hh"

#undef assert

namespace SSC::Tests {
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
