#include "../json.hh"

namespace ssc::runtime::JSON {
  const Any nullAny = nullptr;

  Type Any::valueType = Type::Any;

  Any::Any () {
    this->data = nullptr;
    this->type = Type::Empty;
  }

  Any::Any (const Any& any) {
    this->type = any.type;
    this->data = any.data;
  }

  Any::Any (Any&& any) {
    this->data = std::move(any.data);
    this->type = any.type;
    any.type = Type::Empty;
    any.data = nullptr;
  }

  Any::Any (Type type, const SharedEntityPointer& data) {
    this->data = data;
    this->type = type;
  }

  Any::Any (const Null null) {
    this->data = JSON::make_shared<Null>();
    this->type = Type::Null;
  }

  Any::Any (std::nullptr_t) {
    this->data = JSON::make_shared<Null>();
    this->type = Type::Null;
  }

  Any::Any (const char* string) {
    this->data = JSON::make_shared<String>(string);
    this->type = Type::String;
  }

  Any::Any (const char string) {
    this->data = JSON::make_shared<String>(string);
    this->type = Type::String;
  }

  Any::Any (const runtime::Path& path)
    : Any(path.string())
  {}

  Any::Any (const runtime::Map<runtime::String, runtime::String>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, std::nullptr_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, const Null>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, bool>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, int64_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, uint64_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, uint32_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, int32_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, double>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Any::Any (const runtime::Map<runtime::String, size_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }

  Any::Any (const runtime::Map<runtime::String, ssize_t>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }
#elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
  Any::Any (const runtime::Map<runtime::String, long long>& map) {
    this->data = JSON::make_shared<Object>(map);
    this->type = Type::Object;
  }
#endif

  Any::Any (const runtime::String& string) {
    this->data = JSON::make_shared<String>(string);
    this->type = Type::String;
  }

  Any::Any (const String& string) {
    this->data = JSON::make_shared<String>(string);
    this->type = Type::String;
  }

  Any::Any (bool boolean) {
    this->data = JSON::make_shared<Boolean>(boolean);
    this->type = Type::Boolean;
  }

  Any::Any (const Boolean& boolean) {
    this->data = JSON::make_shared<Boolean>(boolean);
    this->type = Type::Boolean;
  }

  Any::Any (int32_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

  Any::Any (uint32_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

  Any::Any (int64_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

  Any::Any (uint64_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

  Any::Any (double number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Any::Any (size_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }

  Any::Any (ssize_t number) {
    this->data = JSON::make_shared<Number>((double) number);
    this->type = Type::Number;
  }
#elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
  Any::Any (long long number) {
    this->data = JSON::make_shared<Number>((double) number);
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
    this->data = JSON::make_shared<Number>(number);
    this->type = Type::Number;
  }

  Any::Any (const Object& object) {
    this->data = JSON::make_shared<Object>(object);
    this->type = Type::Object;
  }

  Any::Any (const Object::Entries& entries) {
    this->data = JSON::make_shared<Object>(entries);
    this->type = Type::Object;
  }

  Any::Any (const Array& array) {
    this->data = JSON::make_shared<Array>(array);
    this->type = Type::Array;
  }

  Any::Any (const Array::Entries& entries) {
    this->data = JSON::make_shared<Array>(entries);
    this->type = Type::Array;
  }

  Any::Any (const Raw& source) {
    this->data = JSON::make_shared<Raw>(source);
    this->type = Type::Raw;
  }

  Any::Any (const Error& error) {
    this->data = JSON::make_shared<Error>(error);
    this->type = Type::Error;
  }

#if SOCKET_RUNTIME_PLATFORM_APPLE
  Any::Any (const NSError* error) {
    this->type = Type::Error;
    this->data = JSON::make_shared<Error>(
      error.domain.UTF8String,
      error.localizedDescription.UTF8String,
      error.code
    );
  }
#elif SOCKET_RUNTIME_PLATFORM_LINUX
  Any::Any (const GError* error) {
    this->type = Type::Error;
    this->data = JSON::make_shared<Error>(
      g_quark_to_string(error->domain),
      error->message,
      error->code
    );
  }
#endif

  Any::~Any () {}

  Any& Any::operator = (const Any& any) {
    if (this != &any) {
      this->type = any.type;
      this->data = any.data;
    }
    return *this;
  }

  Any& Any::operator = (Any&& any) {
    if (this != &any) {
      this->type = any.type;
      this->data = std::move(any.data);
      any.type = Type::Empty;
      any.data = nullptr;
    }
    return *this;
  }

  Any& Any::operator [](const char* key) {
    return this->operator[](runtime::String(key));
  }

  Any& Any::operator [](const runtime::String& key) {
    if (this->type == Type::Object) {
      return this->as<Object>()[key];
    }
    throw Error("TypeError", "cannot use operator[] on non-object type", __PRETTY_FUNCTION__);
  }

  Any& Any::operator [](const unsigned int index) {
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

    return this->data.get() == input.data.get();
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

    return this->data.get() != input.data.get();
  }

  template Any& Any:: as() const;
  template Raw& Any::as () const;
  template Null& Any::as () const;
  template Object& Any::as () const;
  template Array& Any::as () const;
  template Boolean& Any::as () const;
  template Number& Any::as () const;
  template String& Any::as () const;
  template Error& Any::as () const;

  template <typename T> T& JSON::Any::as () const {
    if (this->type != T::valueType) {
      throw Error("TypeError", "Invalid cast in Any::as<T>()");
    }

    const auto ptr = this->data.get();

    if (ptr != nullptr && this->type != Type::Null) {
      return *static_cast<T *>(ptr);
    }

    throw Error("BadCastError", "cannot cast to null value", __PRETTY_FUNCTION__);
  }

  Any& Any::at (const runtime::String& key) {
    return this->as<Object>().at(key);
  }

  Any& Any::at (const unsigned int index) {
    return this->as<Array>().at(index);
  }

  const SharedEntityPointer Any::value () const {
    return this->data;
  }

  const runtime::String Any::str () const {
    if (this->data) {
      return this->data->str();
    }

    return "";
  }
}
