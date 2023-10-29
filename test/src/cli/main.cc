#include <socket/extension.h>
#include "tests.hh"

static bool initialize (sapi_context_t* context, const void *data) {
  SSC::Tests::Harness harness;
  return harness.run("cli-tests", [](auto t) {
    t.run(SSC::Tests::command);
  });
}

SOCKET_RUNTIME_REGISTER_EXTENSION("cli-tests", initialize);
