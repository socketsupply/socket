#include <socket/extension.h>
#include "tests.hh"

bool initialize (sapi_context_t* context, const void *data) {
  bool success = true;
  SSC::Tests::Harness harness;
  if (!harness.run(SSC::Tests::codec)) success = false;
  if (!harness.run(SSC::Tests::config)) success = false;
  if (!harness.run(SSC::Tests::env)) success = false;
  if (!harness.run(SSC::Tests::ini)) success = false;
  if (!harness.run(SSC::Tests::json)) success = false;
  if (!harness.run(SSC::Tests::platform)) success = false;
  if (!harness.run(SSC::Tests::preload)) success = false;
  if (!harness.run(SSC::Tests::version)) success = false;
  return success;
}

SOCKET_RUNTIME_REGISTER_EXTENSION("runtime-core-tests", initialize);
