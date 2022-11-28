#include "json.hh"

namespace SSC::JSON {
  Number::Number (const String& string) {
    this->data = std::stod(string.str());
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

  #if defined(__APPLE__)
  Any::Any (ssize_t  number) {
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

  std::string Any::str () const {
    auto ptr = this->pointer.get();

    switch (this->type) {
      case Type::Any: return "";
      case Type::Null: return Null().str();
      case Type::Object: return reinterpret_cast<Object *>(ptr)->str();
      case Type::Array: return reinterpret_cast<Array *>(ptr)->str();
      case Type::Boolean: return reinterpret_cast<Boolean *>(ptr)->str();
      case Type::Number: return reinterpret_cast<Number *>(ptr)->str();
      case Type::String: return reinterpret_cast<String *>(ptr)->str();
    }

    return "";
  }
}
