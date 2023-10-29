#include "./tests.hh"
#include "src/core/config.hh"

namespace SSC::Tests {
  void config (Harness& t) {
    t.test("SSC::Config::get()", [](auto t) {
      const auto config = Config(R"INI(
      [a]
      key = "value"

      [b]
      key = "value"
      )INI");

      const auto a = config.get("a.key");
      const auto b = config.get("b.key");

      t.equals(a, "value", "a.key == value");
      t.equals(b, "value", "b.key == value");
    });

    t.test("SSC::Config::set()", [](auto t) {
      Config config;
      config.set("a.key", "value");
      config.set("b.key", "value");
      t.equals(config.get("a.key"), "value", "a.key == value");
      t.equals(config.get("b.key"), "value", "b.key == value");
    });

    t.test("SSC::Config::contains()", [](auto t) {
      Config config;
      config.set("a.key", "value");
      config.set("b.key", "value");
      t.assert(config.contains("a.key"), "contains a.key");
      t.assert(config.contains("b.key"), "contains b.key");
    });

    t.test("SSC::Config::query()", [](auto t) {
      const auto config = Config(R"INI(
      [simple]
      key = "value"

      [first.class-a]
      key = "class a"

      [second.class-a]
      key = "class a"

      [third.class-a]
      key = "class a"
      )INI");

      auto simple = config.query("[simple]");
      auto first = config.query("[first]");
      auto second = config.query("[second]");
      auto third = config.query("[third]");

      auto classA = config.query("[.class-a]");

      t.equals(simple.get("simple.key"), "value", "simple.key == value");
      t.equals(first.get("first.class-a.key"), "class a", "first.class-a.key == class a");
      t.equals(second.get("second.class-a.key"), "class a", "second.class-a.key == class a");
      t.equals(third.get("third.class-a.key"), "class a", "third.class-a.key == class a");

      for (const auto& tuple : classA) {
        t.equals(tuple.second, "class a", tuple.first + "contains [.class-a]");
      }
    });

    t.test("SSC::Config::erase()", [](auto t) {
      Config config;
      config.set("a.key", "value");
      config.set("b.key", "value");

      config.erase("a.key");
      t.assert(!config.contains("a.key"), "does not contain a.key");

      config.erase("b");
      t.assert(!config.contains("b.key"), "does not contain b.key");
    });

    t.test("SSC::Config::clear()", [](auto t) {
      Config config;
      config.set("a.key", "value");
      config.set("b.key", "value");

      config.clear();
      t.assert(!config.contains("a.key"), "does not contain a.key");
      t.assert(!config.contains("b.key"), "does not contain b.key");
    });

    t.test("SSC::Config::size()", [](auto t) {
      Config config;
      config.set("a", "value");
      t.equals(config.size(), 1, "config.size() == 1");
      config.set("b", "value");
      t.equals(config.size(), 2, "config.size() == 2");
      config.set("c", "value");
      t.equals(config.size(), 3, "config.size() == 3");
      config.set("c", "value");
      t.equals(config.size(), 3, "config.size() == 3");
      config.erase("c");
      t.equals(config.size(), 2, "config.size() == 2");
    });

    t.test("SSC::Config::slice()", [](auto t) {
      const auto config = Config(R"INI(
      [meta]
      title = "my application"
      version = "1.2.3"

      [build]
      script = "build.sh"

      [build.extensions.my-extension]
      source = "extension/"

      [build.extensions.my-other-extension]
      source = "other-extension/"
      )INI");

      const auto meta = config.slice("meta");
      t.equals(meta.get("title"), "my application", "meta.title = 'my application'");
      t.equals(meta.get("version"), "1.2.3", "meta.version = '1.2.3'");

      const auto build = config.slice("build");
      t.equals(build.get("script"), "build.sh", "build.script == 'build.sh'");

      const auto extensions = build.slice("extensions");
      t.equals(extensions.get("my-extension.source"), "extension/", "build.extensions.my-extension.source = 'extension/'");
      t.equals(extensions.get("my-other-extension.source"), "other-extension/", "build.extensions.my-other-extension.source = 'other-extension/'");
    });

    t.test("SSC::Config::children()", [](auto t) {
      const auto config = Config(R"INI(
      [0]
      leaf = 0
        [0.1]
        leaf = 01
          [0.1.0]
          leaf = 010
          [0.1.1]
          leaf = 011
          [0.1.2]
          leaf = 012
        [0.2]
        leaf = 02
        [0.3]
        leaf = 03

      [1]
      leaf = 1

      [2]
      leaf = 2
      )INI");

      const auto children = config.children();
      t.equals(children[0].prefix, "0", "children[0].prefix == 0");
      t.equals(children[1].prefix, "1", "children[1].prefix == 1");
      t.equals(children[2].prefix, "2", "children[2].prefix == 2");

      t.equals(children[0].children()[0].prefix, "1", "children[0].children[0].prefix == 1");
      t.equals(children[0].children()[1].prefix, "2", "children[0].children[1].prefix == 2");
      t.equals(children[0].children()[2].prefix, "3", "children[0].children[2].prefix == 3");
    });
  }
}
