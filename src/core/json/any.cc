#include "../json.hh"

namespace SSC::JSON {
  Any anyNull = nullptr;

  Any::Any () {
    this->pointer = nullptr;
    this->type = Type::Null;
  }

  Any::Any (const Any& any) : pointer(any.pointer) {
    this->type = any.type;
  }

  Any::Any (Type type, SharedPointer<void> pointer) : pointer(pointer) {
    this->type = type;
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

  Any::Any (const SSC::Path& path)
    : Any(path.string())
  {}

  Any::Any (const SSC::String& string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (const String& string) {
    this->pointer = SharedPointer<void>(new String(string));
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->pointer = SharedPointer<void>(new Boolean(boolean));
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean& boolean) {
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
#elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
  Any::Any (long long number) {
    this->pointer = SharedPointer<void>(new Number((double) number));
    this->type = Type::Number;
  }
#endif

  Any::Any (Atomic<bool>& boolean) : Any(boolean.load()) {}
  Any::Any (Atomic<int64_t>& number) : Any(number.load()) {}
  Any::Any (Atomic<uint64_t>& number) : Any(number.load()) {}
  Any::Any (Atomic<uint32_t>& number) : Any(number.load()) {}
  Any::Any (Atomic<int32_t>& number) : Any(number.load()) {}
  Any::Any (Atomic<double>& number) : Any(number.load()) {}

  Any::Any (const Number& number) {
    this->pointer = SharedPointer<void>(new Number(number));
    this->type = Type::Number;
  }

  Any::Any (const Object& object) {
    this->pointer = SharedPointer<void>(new Object(object));
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries& entries) {
    this->pointer = SharedPointer<void>(new Object(entries));
    this->type = Type::Object;
  }

  Any::Any (const Array& array) {
    this->pointer = SharedPointer<void>(new Array(array));
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries& entries) {
    this->pointer = SharedPointer<void>(new Array(entries));
    this->type = Type::Array;
  }

  Any::Any (const Raw& source) {
    this->pointer = SharedPointer<void>(new Raw(source));
    this->type = Type::Raw;
  }

  Any::Any (const Error& error) {
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

  Any::~Any () {
    this->pointer = nullptr;
    this->type = Type::Any;
  }

  Any Any::operator[](const SSC::String& key) const {
    if (this->type == Type::Object) {
      return this->as<Object>()[key];
    }
    throw Error("TypeError", "cannot use operator[] on non-object type", __PRETTY_FUNCTION__);
  }

  Any& Any::operator[](const SSC::String& key) {
    if (this->type == Type::Object) {
      return this->as<Object>()[key];
    }
    throw Error("TypeError", "cannot use operator[] on non-object type", __PRETTY_FUNCTION__);
  }

  Any Any::operator[](const unsigned int index) const {
    if (this->type == Type::Array) {
      return this->as<Array>()[index];
    }
    throw Error("TypeError", "cannot use operator[] on non-array type", __PRETTY_FUNCTION__);
  }

  Any& Any::operator[](const unsigned int index) {
    if (this->type == Type::Array) {
      return this->as<Array>()[index];
    }
    throw Error("TypeError", "cannot use operator[] on non-array type", __PRETTY_FUNCTION__);
  }

  bool Any::operator == (const Any& input) const {
    if (this->isEmpty() && input.isEmpty()) {
      return true;
    }

    if (this->isNull() && input.isNull()) {
      return true;
    }

    if (this->isString() && input.isString()) {
      return this->str() == input.str();
    }

    if (this->isNumber() && input.isNumber()) {
      return this->as<Number>().value() == input.as<Number>().value();
    }

    if (this->isBoolean() && input.isBoolean()) {
      return this->as<Boolean>().value() == input.as<Boolean>().value();
    }

    return false;
  }

  bool Any::operator != (const Any& input) const {
    if (this->isEmpty() && !input.isEmpty()) {
      return true;
    }

    if (this->isNull() && !input.isNull()) {
      return true;
    }

    if (this->isString() && input.isString()) {
      return this->str() != input.str();
    }

    if (this->isNumber() && input.isNumber()) {
      return this->as<Number>().value() != input.as<Number>().value();
    }

    if (this->isBoolean() && input.isBoolean()) {
      return this->as<Boolean>().value() != input.as<Boolean>().value();
    }

    return true;
  }

  const SSC::String Any::str () const {
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
