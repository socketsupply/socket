#include "../json.hh"

namespace ssc::runtime::JSON {
  Type Raw::valueType = Type::Raw;

  Raw::Raw (const Raw& raw) {
    this->data = raw.data;
  }

  Raw::Raw (Raw&& raw) {
    this->data = std::move(raw.data);
    raw.data = "";
  }

  Raw::Raw (const Raw* raw) {
    if (raw != nullptr) {
      this->data = raw->data;
    }
  }

  Raw::Raw (const runtime::String& source) {
    this->data = source;
  }

  Raw::Raw (const Any& source) {
    if (source.isString()) {
      this->data = source.data;
    } else {
      this->data = source.str();
    }
  }

  Raw& Raw::operator = (const Raw& raw) {
    this->data = raw.data;
    return *this;
  }

  Raw& Raw::operator = (Raw&& raw) {
    this->data = std::move(raw.data);
    raw.data = "";
    return *this;
  }

  const runtime::String Raw::value () const {
    return this->data;
  }

  const runtime::String Raw::str () const {
    return this->data;
  }
}
