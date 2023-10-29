#include "tests.hh"

namespace SSC::Tests {
  void env (Harness& t) {
    t.test("SSC::Env::get()", [](auto t) {
        const auto TEST_INJECTED_VARIABLE = SSC::Env::get("TEST_INJECTED_VARIABLE");
        const auto HOME = SSC::Env::get("HOME");
        t.equals(
          TEST_INJECTED_VARIABLE,
          "TEST_INJECTED_VARIABLE",
          "TEST_INJECTED_VARIABLE env var is set"
        );

        t.assert(HOME, "HOME env var is set");
    });
  }
}
