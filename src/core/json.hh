#ifndef SSC_CORE_JSON_H
#define SSC_CORE_JSON_H

#include "../common.hh"

namespace SSC::JSON {
  enum class Type {
    Any,
    Null,
    Object,
    Array,
    Boolean,
    Number,
    String
  };

  class Any;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<SSC::String, Any>;
  using ArrayEntries = SSC::Vector<Any>;

  template <typename D, Type t> class Value {
    public:
      Type type = t;
      virtual SSC::String str () const = 0;

    protected:
      D data;
  };

  class Any : public Value<void *, Type::Any> {
    public:
      std::shared_ptr<void> pointer;

      Any ();
      ~Any ();
      Any (const Any&);
      Any (std::nullptr_t);
      Any (const Null);
      Any (bool);
      Any (const Boolean);
      Any (int64_t);
      Any (uint64_t);
      Any (uint32_t);
      Any (int32_t);
      Any (float);
      Any (double);
#if defined(__APPLE__)
      Any (ssize_t);
#endif
      Any (const Number);
      Any (const char *);
      Any (const SSC::String);
      Any (const String);
      Any (const Object);
      Any (const ObjectEntries);
      Any (const Array);
      Any (const ArrayEntries);
      SSC::String str () const;
  };

  class Null : Value<std::nullptr_t, Type::Null> {
    public:
      Null () = default;
      std::nullptr_t value () const { return nullptr; }
      SSC::String str () const { return "null"; }
  };

  class Object : Value<ObjectEntries, Type::Object> {
    public:
      using Entries = ObjectEntries;
      Object () = default;
      Object (const Object::Entries entries);
      Object (const Object& object) { this->data = object.value(); }
      Object (const SSC::Map map);
      SSC::String str () const;
      const Object::Entries value () const { return this->data; }
  };

  class Array : Value<ArrayEntries, Type::Array> {
    public:
      using Entries = ArrayEntries;
      Array () = default;
      Array (const Array& array) { this->data = array.value(); }
      Array (const Array::Entries entries);
      SSC::String str () const;
      Array::Entries value () const { return this->data; }
  };

  class Boolean : Value<bool, Type::Boolean> {
    public:
      Boolean () = default;
      Boolean (const Boolean& boolean) { this->data = boolean.value(); }
      Boolean (bool boolean) { this->data = boolean; }
      Boolean (int64_t data) { this->data = data != 0; }
      Boolean (double data) { this->data = data != 0; }
      Boolean (void *data) { this->data = data != nullptr; }
      bool value () const { return this->data; }
      SSC::String str () const { return this->data ? "true" : "false"; }
  };

  class Number : Value<float, Type::Number> {
    public:
      Number () = default;
      Number (const Number& number) { this->data = number.value(); }
      Number (float number) { this->data = number; }
      Number (int64_t number) { this->data = (float) number; }
      float value () const { return this->data; }
      SSC::String str () const {
        auto remainer = this->data - (int32_t) this->data;
        if (remainer > 0) {
          return SSC::format("$S", std::to_string(this->data));
        } else {
          return SSC::format("$S", std::to_string((int32_t) this->data));
        }
      }
  };

  class String : Value<SSC::String, Type::Number> {
    public:
      String () = default;
      String (const String& data) { this->data = SSC::String(data.str()); }
      String (const SSC::String data) { this->data = data; }
      String (const char *data) { this->data = SSC::String(data); }
      SSC::String str ()  const { return SSC::format("\"$S\"", this->data); }
      SSC::String value () const { return this->data; }
  };
} // SSC::JSON
#endif
