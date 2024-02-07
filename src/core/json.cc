#include <regex>

#include "json.hh"
#include "string.hh"

namespace SSC::JSON {
  Null null;
  Any anyNull = nullptr;

  Number::Number (const String& string) {
    this->data = std::stod(string.str());
  }

  std::string Number::str () const {
    if (this->data == 0) {
      return "0";
    }

    auto value = this->data;
    auto output = std::to_string(value);
    auto decimal = output.find(".");

    // trim trailing zeros
    if (decimal >= 0) {
      auto i = output.size() - 1;
      while (output[i] == '0' && i >= decimal) {
        i--;
      }

      return output.substr(0, i);
    }

    return output;
  }

  std::string Object::str () const {
    std::stringstream stream;
    auto count = this->data.size();
    stream << std::string("{");
    for (const auto& tuple : this->data) {
      auto key = replace(tuple.first, "\"","\\\"");
      auto value = tuple.second.str();

      stream << std::string("\"");
      stream << key;
      stream << std::string("\":");
      stream << value;

      if (--count > 0) {
        stream << std::string(",");
      }
    }

    stream << std::string("}");
    return stream.str();
  }

  std::string Array::str () const {
    std::stringstream stream;
    auto count = this->data.size();
    stream << std::string("[");

    for (const auto& value : this->data) {
      stream << value.str();

      if (--count > 0) {
        stream << std::string(",");
      }
    }

    stream << std::string("]");
    return stream.str();
  }

  String::String (const Number& number) {
    this->data = number.str();
  }

  SSC::String String::str () const {
    auto escaped = replace(this->data, "\"", "\\\"");
    return "\"" + replace(escaped, "\n", "\\n") + "\"";
  }

  Any::Any (const Null null) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (std::nullptr_t) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (const char *string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const char string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const std::string string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const String string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean boolean) {
    this->pointer = std::shared_ptr<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (int32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }

#if defined(__APPLE__) && !TARGET_OS_IPHONE && !TARGET_IPHONE_SIMULATOR
  Any::Any (size_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }
#endif

#if defined(__APPLE__)
  Any::Any (ssize_t number) {
    this->pointer = std::shared_ptr<void>(new Number((double) number));
    this->type = Type::Number;
  }
#endif

  Any::Any (const Number number) {
    this->pointer = std::shared_ptr<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Object object) {
    this->pointer = std::shared_ptr<void>(new Object(object));
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Object(entries));
    this->type = Type::Object;
  }

  Any::Any (const Array array) {
    this->pointer = std::shared_ptr<void>(new Array(array));
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries entries) {
    this->pointer = std::shared_ptr<void>(new Array(entries));
    this->type = Type::Array;
  }

  Any::Any (const Raw source) {
    this->pointer = std::shared_ptr<void>(new Raw(source));
    this->type = Type::Raw;
  }

  std::string Any::str () const {
    const auto ptr = this->pointer.get() == nullptr
      ? reinterpret_cast<const void*>(this)
      : this->pointer.get();

    switch (this->type) {
      case Type::Empty: return "";
      case Type::Any: return "";
      case Type::Raw: return reinterpret_cast<const Raw*>(ptr)->str();
      case Type::Null: return "null";
      case Type::Object: return reinterpret_cast<const Object*>(ptr)->str();
      case Type::Array: return reinterpret_cast<const Array*>(ptr)->str();
      case Type::Boolean: return reinterpret_cast<const Boolean*>(ptr)->str();
      case Type::Number: return reinterpret_cast<const Number*>(ptr)->str();
      case Type::String: return reinterpret_cast<const String*>(ptr)->str();
    }

    return "";
  }
}
