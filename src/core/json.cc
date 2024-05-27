#include "json.hh"

namespace SSC::JSON {
  Null null;
  Any anyNull = nullptr;

  Number::Number (const String& string) {
    this->data = std::stod(string.str());
  }

  SSC::String Number::str () const {
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

  SSC::String Object::str () const {
    SSC::StringStream stream;
    auto count = this->data.size();
    stream << SSC::String("{");
    for (const auto& tuple : this->data) {
      auto key = replace(tuple.first, "\"","\\\"");
      auto value = tuple.second.str();

      stream << SSC::String("\"");
      stream << key;
      stream << SSC::String("\":");
      stream << value;

      if (--count > 0) {
        stream << SSC::String(",");
      }
    }

    stream << SSC::String("}");
    return stream.str();
  }

  SSC::String Array::str () const {
    SSC::StringStream stream;
    auto count = this->data.size();
    stream << SSC::String("[");

    for (const auto& value : this->data) {
      stream << value.str();

      if (--count > 0) {
        stream << SSC::String(",");
      }
    }

    stream << SSC::String("]");
    return stream.str();
  }

  String::String (const Number& number) {
    this->data = number.str();
  }

  SSC::String String::str () const {
    auto escaped = replace(this->data, "\"", "\\\"");
    escaped = replace(escaped, "\\n", "\\\\n");
    return "\"" + replace(escaped, "\n", "\\n") + "\"";
  }

  Any::Any (const Null null) {
    this->pointer = SharedPointer<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (std::nullptr_t) {
    this->pointer = SharedPointer<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (const char *string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const char string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const SSC::String string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const String string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->pointer = SharedPointer<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean boolean) {
    this->pointer = SharedPointer<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (int32_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Any::Any (size_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }

  Any::Any (ssize_t number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }
#else
  Any::Any (long long number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }
#endif

  Any::Any (const Number number) {
    this->pointer = SharedPointer<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Object object) {
    this->pointer = SharedPointer<void>(new Object(object));
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries entries) {
    this->pointer = SharedPointer<void>(new Object(entries));
    this->type = Type::Object;
  }

  Any::Any (const Array array) {
    this->pointer = SharedPointer<void>(new Array(array));
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries entries) {
    this->pointer = SharedPointer<void>(new Array(entries));
    this->type = Type::Array;
  }

  Any::Any (const Raw source) {
    this->pointer = SharedPointer<void>(new Raw(source));
    this->type = Type::Raw;
  }

  Any::Any (const Error error) {
    this->pointer = SharedPointer<void>(new Error(error));
    this->type = Type::Error;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Any::Any (const NSError* error) {
    this->type = Type::Error;
    this->pointer = SharedPointer<void>(new Error(
      error.domain.UTF8String,
      error.localizedDescription.UTF8String,
      error.code
    ));
  }
#elif SOCKET_RUNTIME_PLATFORM_LINUX
  Any::Any (const GError* error) {
    this->type = Type::Error;
    this->pointer = SharedPointer<void>(new Error(
        g_quark_to_string(error->domain),
        error->message,
        error->code
    ));
  }
#endif

  SSC::String Any::str () const {
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
      case Type::Error: return reinterpret_cast<const Error*>(ptr)->str();
    }

    return "";
  }
}
