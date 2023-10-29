#ifndef CLI_TESTS_H
#define CLI_TESTS_H

#include <functional>

#include <socket/extension.h>
#include "src/core/core.hh"
#include "../runtime-core/harness.hh"

#undef assert

namespace SSC::Tests {
  void command (Harness&);
  void program (Harness&);
}

#endif
