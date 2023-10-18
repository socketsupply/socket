#include "tests.hh"

namespace SSC::Tests {
  void ini (Harness& t) {
    t.test("SSC::INI::parse", [] (auto t) {
      auto simple = SSC::INI::parse(R"INI(
        key = "value"
      )INI");

      t.equals(simple["key"], "value", "simple[key] == value");

      auto sections = SSC::INI::parse(R"INI(
        [section-1]
        key = "value"

        [section-2]
        key = "value"

        [section-3]
        key = "value"
      )INI");

      t.equals(sections["section-1_key"], "value", "sections[section-1_key] == value");
      t.equals(sections["section-2_key"], "value", "sections[section-2_key] == value");
      t.equals(sections["section-3_key"], "value", "sections[section-3_key] == value");

      auto subsections = SSC::INI::parse(R"INI(
        [section-1]
        key = "value"
        [.subsection]
        key = "value"

        [section-2]
        key = "value"
        [.subsection]
        key = "value"

        [section-3]
        key = "value"
        [.subsection]
        key = "value"
      )INI");

      t.equals(subsections["section-1_key"], "value", "subsections[section-1_key] == value");
      t.equals(subsections["section-2_key"], "value", "subsections[section-2_key] == value");
      t.equals(subsections["section-3_key"], "value", "subsections[section-3_key] == value");

      t.equals(subsections["section-1_subsection_key"], "value", "subsections[section-1_subsection_key] == value");
      t.equals(subsections["section-2_subsection_key"], "value", "subsections[section-2_subsection_key] == value");
      t.equals(subsections["section-3_subsection_key"], "value", "subsections[section-3_subsection_key] == value");

      auto arrays = SSC::INI::parse(R"INI(
        [numbers]
        array[] = 1
        array[] = 2
        array[] = 3

        [strings]
        array[] = "hello"
        array[] = world
      )INI");

      t.equals(arrays["numbers_array"], "1 2 3", "arrays[numbers_array] == 1 2 3");
      t.equals(arrays["strings_array"], "hello world", "arrays[strings_array] == hello world");

      auto dotsyntax = SSC::INI::parse(R"INI(
        [a.b.c.d.e.f]
        g = "value"

        [a.b.c.d.e]
        [.f.g.h]
        i = "value"
      )INI", ".");

      t.equals(dotsyntax["a.b.c.d.e.f.g"], "value", "dotsyntax[a.b.c.d.e.f.g] == value");
      t.equals(dotsyntax["a.b.c.d.e.f.g.h.i"], "value", "dotsyntax[a.b.c.d.e.f.g.h.i] == value");
    });
  }
}
