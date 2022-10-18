#include "json.hh"

namespace SSC::JSON {
  Any::Any () {
    this->pointer = nullptr;
    this->type = Type::Null;
  }

  Any::~Any () {
    this->pointer = nullptr;
    this->type = Type::Any;
  }

  Any::Any (const Any &any) {
    this->pointer = any.pointer;
    this->type = any.type;
  }

  Any::Any (const Null null) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any(std::nullptr_t null) {
    this->pointer = std::shared_ptr<void>(new Null());
    this->type = Type::Null;
  }

  Any::Any (const char *string) {
    this->pointer = std::shared_ptr<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const SSC::String string) {
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
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }

#if defined(__APPLE__)
  Any::Any (ssize_t  number) {
    this->pointer = std::shared_ptr<void>(new Number((float) number));
    this->type = Type::Number;
  }
#endif

  Any::Any (float number) {
    this->pointer = std::shared_ptr<void>(new Number(number));
    this->type = Type::Number;
  }

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

  SSC::String Any::str () const {
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

  Object::Object (const Object::Entries entries) {
    for (const auto& tuple : entries) {
      auto key = tuple.first;
      auto value = tuple.second;
      this->data.insert_or_assign(key, value);
    }
  }

  Object::Object (const SSC::Map map) {
    for (const auto& tuple : map) {
      auto key = tuple.first;
      auto value = Any(tuple.second);
      this->data.insert_or_assign(key, value);
    }
  }

  SSC::String Object::str () const {
    StringStream stream;
    auto count = this->data.size();
    stream << "{";

    for (const auto& tuple : this->data) {
      auto key = tuple.first;
      auto value = tuple.second.str();

      stream << "\"" << key << "\":";
      stream << value;

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "}";
    return stream.str();
  }

  Array::Array (const Array::Entries entries) {
    for (const auto& value : entries) {
      this->data.push_back(value);
    }
  }

  SSC::String Array::str () const {
    SSC::StringStream stream;
    auto count = this->data.size();
    stream << "[";

    for (const auto& value : this->data) {
      stream << value.str();

      if (--count > 0) {
        stream << ",";
      }
    }

    stream << "]";
    return stream.str();
  }
}
