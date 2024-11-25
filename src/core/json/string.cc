#include "../json.hh"

namespace SSC::JSON {
  String::String (const Number& number) {
    this->data = number.str();
  }

  String::String (const String& data) {
    this->data = SSC::String(data.value());
  }

  String::String (const SSC::String& data) {
    this->data = data;
  }

  String::String (const char data) {
    this->data = SSC::String(1, data);
  }

  String::String (const char *data) {
    this->data = SSC::String(data);
  }

  String::String (const Any& any) {
    this->data = any.str();
  }

  String::String (const Boolean& boolean) {
    this->data = boolean.str();
  }

  String::String (const Error& error) {
    this->data = error.str();
  }

  const SSC::String String::str () const {
    auto escaped = replace(this->data, "\"", "\\\"");
    escaped = replace(escaped, "\\n", "\\\\n");
    return "\"" + replace(escaped, "\n", "\\n") + "\"";
  }

  SSC::String String::value () const {
    return this->data;
  }

  SSC::String::size_type String::size () const {
    return this->data.size();
  }
}
