#include "tests.hh"

namespace SSC::Tests {
  void string (Harness& t) {
    t.test("SSC::replace()", [](auto t) {
      t.comment("TODO");
    });

    t.test("SSC::tmpl()", [](auto t) {
      t.comment("TODO");
    });

    t.test("SSC::trim()", [](auto t) {
      t.comment("TODO");
    });

    t.test("SSC::convertStringToWString()", [](auto t) {
      t.comment("TODO");
    });

    t.test("SSC::convertWStringToString()", [](auto t) {
      t.comment("TODO");
    });

    t.test("SSC::split(const String&, const String&)", [](auto t) {
      const auto items = split("a && b && c && d && e", " && ");

      t.equals(items[0], "a", "items[0] == a");
      t.equals(items[1], "b", "items[1] == b");
      t.equals(items[2], "c", "items[2] == c");
      t.equals(items[3], "d", "items[3] == d");
      t.equals(items[4], "e", "items[4] == e");
    });

    t.test("SSC::split(const String&, char)", [](auto t) {
      const auto items = split("a|b|c|d|e", '|');

      t.equals(items[0], "a", "items[0] == a");
      t.equals(items[1], "b", "items[1] == b");
      t.equals(items[2], "c", "items[2] == c");
      t.equals(items[3], "d", "items[3] == d");
      t.equals(items[4], "e", "items[4] == e");
    });

    t.test("SSC::join()", [](auto t) {
      const auto joined = join(split("a|b|c|d|e", '|'), '|');
      t.equals(joined, "a|b|c|d|e", "joins vector");
    });

    t.test("SSC::parseStringList()", [](auto t) {
      t.comment("TODO");
    });
  }
}
