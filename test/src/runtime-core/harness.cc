#include "tests.hh"
#include "src/core/types.hh"
#include "./ok.hh"

namespace SSC::Tests {
  Harness::Harness (const Options& options) : options(options) {
    this->pending = this->options.pending;
    this->isAsync = this->options.isAsync;
  }

  bool Harness::run (TestRunner runner) {
    return this->run(false, "", runner);
  }

  bool Harness::run (bool isAsync, TestRunner runner) {
    return this->run(isAsync, "", runner);
  }

  bool Harness::run (bool isAsync, const String& label, TestRunner runner) {
    if (label.size() > 0) {
      this->mutex.lock();
      this->label(label);
      if (this->options.resetContextAfterEachRun) {
        ok_reset();
      }
    }

    this->pending++;
    Harness harness(Options {
      .resetContextAfterEachRun = this->options.resetContextAfterEachRun,
      .isAsync = isAsync,
      .pending = this->pending.load()
    });

    runner(harness);
    this->pending--;

    if (label.size() > 0 ) {
      if (!this->isAsync) {
        this->mutex.unlock();
      }
    }

    if (harness.isAsync) {
      harness.wait();
    }

    if (this->pending == 0) {
      if (ok_count() > 0 || ok_failed() > 0) {
        auto success = ok_done();
        ok_reset();
        return success;
      }
    }

    return false;
  }

  void Harness::end () {
    this->mutex.unlock();
  }

  void Harness::wait () {
    this->mutex.lock();
    this->mutex.unlock();
  }

  void Harness::plan (unsigned int planned) {
    if (planned > 0) {
      this->isAsync = true;
      this->mutex.lock();
    } else if (planned == 0 && this->isAsync) {
      this->isAsync = false;
    }

    this->planned = planned;
  }

  bool Harness::test (const String& label, bool isAsync, TestRunner scope) {
    return this->run(isAsync, label, scope);
  }

  bool Harness::test (const String& label, TestRunner scope) {
    return this->run(false, label, scope);
  }

  void Harness::comment (const String& comment) const {
    ok_comment(comment.c_str());
  }

  void Harness::label (const String& label) const {
    ok_begin(nullptr);
    ok_comment(label.c_str());
  }

  void Harness::log (const String& message) const {
    sapi_log(0, message.c_str());
  }

  void Harness::log (const Map& message) const {
    if (message.size() == 0) {
      return this->log("Map {}");
    }

    auto size = message.size();
    int i = 0;
    this->log("Map {");
    for (const auto& tuple : message) {
      const auto postfix = ++i < size ? "," : "";
      this->log("  \"" + tuple.first + "\" = \"" + tuple.second + "\""+ postfix);
    }
    this->log("}");
  }

  void Harness::log (const Vector<String>& message) const {
    if (message.size() == 0) {
      return this->log("Vector<String> {}");
    }

    auto size = message.size();
    int i = 0;
    this->log("Vector<String> {");
    for (const auto& item: message) {
      const auto postfix = ++i < size ? "," : "";
      this->log("  " + item + postfix);
    }
    this->log("}");
  }

  bool Harness::assert (bool assertion, const String& message) const {
    if (assertion) {
      ok("%s",  message.c_str());
      return true;
    } else {
      notok("assertion failed: %s", message.c_str());
      return false;
    }
  }

  bool Harness::assert (int64_t value, const String& message) const {
    return this->assert(value != 0, message);
  }

  bool Harness::assert (double value, const String& message) const {
    return this->assert(value != 0.0, message);
  }

  bool Harness::assert (void* value, const String& message) const {
    return this->assert(value != 0, message);
  }

  bool Harness::assert (const String& value, const String& message) const {
    return this->assert(value.size() != 0, message);
  }

  bool Harness::equals (const char* left, const char* right, const String& message) const {
    return this->equals(String(left), String(right), message);
  }

  bool Harness::equals (const String& left, const String& right, const String& message) const {
    if (left == right) {
      ok("'%s' equals '%s': %s",  left.c_str(), right.c_str(), message.c_str());
      return true;
    } else {
      notok("'%s' does not equal '%s': %s", left.c_str(), right.c_str(), message.c_str());
      return false;
    }
  }

  bool Harness::equals (const bool left, const bool right, const String& message) const {
    if (left == right) {
      ok("%s equals %s: %s", left ? "true" : "false", right ? "true" : "false", message.c_str());
      return true;
    } else {
      notok("%s does not equal %s: %s", left ? "true" : "false", right ? "true" : "false", message.c_str());
      return false;
    }
  }

  bool Harness::equals (const int64_t left, const int64_t right, const String& message) const {
    if (left == right) {
      ok("%lld equals %lld: %s",  left, right, message.c_str());
      return true;
    } else {
      notok("%lld does not equal %lld: %s", left, right, message.c_str());
      return false;
    }
  }

  bool Harness::equals (const double left, const double right, const String& message) const {
    if (left == right) {
      ok("%f equals %f: %s",  left, right, message.c_str());
      return true;
    } else {
      notok("%f does not equal %f: %s", left, right, message.c_str());
      return false;
    }
  }

  bool Harness::equals (const size_t left, const size_t right, const String& message) const {
    if (left == right) {
      ok("%zu equals %zu: %s",  left, right, message.c_str());
      return true;
    } else {
      notok("%zu does not equal %zu: %s", left, right, message.c_str());
      return false;
    }
  }

  bool Harness::notEquals (const String& left, const String& right, const String& message) const {
    if (left == right) {
      notok("'%s' equals '%s': %s",  left.c_str(), right.c_str(), message.c_str());
      return false;
    } else {
      ok("'%s' does not equal '%s': %s", left.c_str(), right.c_str(), message.c_str());
      return true;
    }
  }

  bool Harness::notEquals (const char* left, const char* right, const String& message) const {
    return this->notEquals(String(left), String(right), message);
  }

  bool Harness::notEquals (const int64_t left, const int64_t right, const String& message) const {
    if (left == right) {
      notok("%lld equals %lld: %s",  left, right, message.c_str());
      return false;
    } else {
      ok("%lld does not equal %lld: %s", left, right, message.c_str());
      return true;
    }
  }

  bool Harness::notEquals (const double left, const double right, const String& message) const {
    if (left == right) {
      notok("%f equals %f: %s",  left, right, message.c_str());
      return false;
    } else {
      ok("%f does not equal %f: %s", left, right, message.c_str());
      return true;
    }
  }

  bool Harness::notEquals (const size_t left, const size_t right, const String& message) const {
    if (left == right) {
      notok("%zu equals %zu: %s",  left, right, message.c_str());
      return false;
    } else {
      ok("%zu does not equal %zu: %s", left, right, message.c_str());
      return true;
    }
  }

  bool Harness::throws (std::function<void()> fn, const String& message) const {
    try {
      fn();
      notok("does not throw exception: %s", message.c_str());
      return false;
    } catch (std::exception e) {
      ok("throws exception: %s", message.c_str());
      return true;
    }
  }
}
