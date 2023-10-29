#include <socket/extension.h>
#include "tests.hh"

static bool initialize (sapi_context_t* context, const void *data) {
  SSC::Tests::Harness harness;
  return harness.run("runtime-core-tests", [](auto t) {
    t.run(SSC::Tests::codec);
    t.run(SSC::Tests::config);
    t.run(SSC::Tests::env);
    t.run(SSC::Tests::ini);
    t.run(SSC::Tests::json);
    t.run(SSC::Tests::platform);
    t.run(SSC::Tests::preload);
    t.run(SSC::Tests::string);
    t.run(SSC::Tests::version);
  });
}

SOCKET_RUNTIME_REGISTER_EXTENSION("runtime-core-tests", initialize);
