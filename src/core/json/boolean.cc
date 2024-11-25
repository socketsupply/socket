#include "../json.hh"

namespace SSC::JSON {
  Boolean::Boolean (const Boolean& boolean) {
    this->data = boolean.value();
  }

  Boolean::Boolean (bool boolean) {
    this->data = boolean;
  }

  Boolean::Boolean (int data) {
    this->data = data != 0;
  }

  Boolean::Boolean (int64_t data) {
    this->data = data != 0;
  }

  Boolean::Boolean (double data) {
    this->data = data != 0;
  }

  Boolean:: Boolean (void *data) {
    this->data = data != nullptr;
  }

  Boolean:: Boolean (const SSC::String& string) {
    this->data = string.size() > 0;
  }

  bool Boolean::value () const {
    return this->data;
  }

  const SSC::String Boolean::str () const {
    return this->data ? "true" : "false";
  }
}
