#include "json.hh"

namespace SSC::JSON {
  Null null;

  Null::Null (std::nullptr_t)
    : Null()
  {}

  std::nullptr_t Null::value () const {
    return nullptr;
  }

  const SSC::String Null::str () const {
    return "null";
  }

  Raw::Raw (const Raw& raw) {
    this->data = raw.data;
  }

  Raw::Raw (const Raw* raw) {
    this->data = raw->data;
  }

  Raw::Raw (const SSC::String& source) {
    this->data = source;
  }

  const SSC::String Raw::str () const {
    return this->data;
  }
}
