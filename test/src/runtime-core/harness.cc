#include "tests.hh"
#include "src/core/types.hh"

namespace SSC::Tests {
  static std::mutex mutex;

  Harness::Harness () {
    mutex.unlock();
  }

  bool Harness::run (TestRunner runner) const {
    return run(false, "", runner);
  }

  bool Harness::run (bool isAsync, TestRunner runner) const {
    return run(isAsync, "", runner);
  }

  bool Harness::run (bool isAsync, const String& label, TestRunner runner) const {
    if (label.size() > 0) {
      mutex.lock();
      this->label(label);
      ok_reset();
    }

    runner(*this);

    if (label.size() > 0 ) {
      if (!isAsync) {
        mutex.unlock();
      }

      if (ok_count() > 0 || ok_failed() > 0 || ok_expected() > 0) {
        auto success = ok_done();
        ok_reset();
        return success;
      }
    }

    return true;
  }

  void Harness::end () const {
    mutex.unlock();
  }

  bool Harness::test (const String& label, bool isAsync, TestRunner scope) const {
    return this->run(isAsync, label, scope);
  }

  bool Harness::test (const String& label, TestRunner scope) const {
    return this->run(false, label, scope);
  }

  void Harness::comment (const String& comment) const {
    ok_comment(comment.c_str());
  }

  void Harness::label (const String& label) const {
    ok_begin(label.c_str());
  }

  void Harness::log (const String& message) const {
    sapi_log(0, message.c_str());
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
    return assert(value != 0, message);
  }

  bool Harness::assert (double value, const String& message) const {
    return assert(value != 0.0, message);
  }

  bool Harness::assert (void* value, const String& message) const {
    return assert(value != 0, message);
  }

  bool Harness::assert (const String& value, const String& message) const {
    return assert(value.size() != 0, message);
  }

  bool Harness::equals (const char* left, const char* right, const String& message) const {
    return equals(String(left), String(right), message);
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
