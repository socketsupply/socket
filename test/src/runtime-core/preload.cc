#include "tests.hh"

namespace SSC::Tests {
  void preload (Harness& t) {
    t.assert(createPreload(WindowOptions {}), "createPreload() returns non-empty string");;
  }
}
