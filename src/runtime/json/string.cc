#include "../string.hh"

#include "../json.hh"

using ssc::runtime::string::replace;

namespace ssc::runtime::JSON {
  Type String::valueType = Type::String;

  String::String (const Number& number) {
    this->data = number.str();
  }

  String::String (const String& data) {
    this->data = runtime::String(data.value());
  }

  String::String (const runtime::String& data) {
    this->data = data;
  }

  String::String (const char data) {
    this->data = runtime::String(1, data);
  }

  String::String (const char *data) {
    this->data = runtime::String(data);
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

  const runtime::String String::str () const {
    auto escaped = replace(this->data, "\"", "\\\"");
    escaped = replace(escaped, "\\n", "\\\\n");
    return "\"" + replace(escaped, "\n", "\\n") + "\"";
  }

  const runtime::String String::value () const {
    return this->data;
  }

  runtime::String::size_type String::size () const {
    return this->data.size();
  }
}
