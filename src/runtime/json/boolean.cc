#include "../json.hh"

namespace ssc::runtime::JSON {
  Type Boolean::valueType = Type::Boolean;

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

  Boolean:: Boolean (const runtime::String& string) {
    this->data = string.size() > 0;
  }

  const bool Boolean::value () const {
    return this->data;
  }

  const runtime::String Boolean::str () const {
    return this->data ? "true" : "false";
  }
}
