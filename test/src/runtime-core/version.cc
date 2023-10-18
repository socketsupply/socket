#include "tests.hh"

namespace SSC::Tests {
  void version (Harness& t) {
    t.test("SSC::{VERSION_FULL_STRING,VERSION_HASH_STRING,VERSION_STRING}", [](auto t) {
      t.assert(VERSION_FULL_STRING, "VERSION_FULL_STRING is defined");
      t.assert(VERSION_HASH_STRING, "VERSION_HASH_STRING is defined");
      t.assert(VERSION_STRING, "VERSION_STRING is defined");
    });
  }
}
