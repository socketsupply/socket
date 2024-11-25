#ifndef SOCKET_RUNTIME_CORE_JSON_H
#define SOCKET_RUNTIME_CORE_JSON_H

#include "../platform/platform.hh"

namespace SSC::JSON {
  // forward
  class Any;
  class Raw;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = std::map<SSC::String, Any>;
  using ArrayEntries = Vector<Any>;

  enum class Type {
    Empty = -1,
    Any = 0,
    Null = 1,
    Object = 2,
    Array = 3,
    Boolean = 4,
    Number = 5,
    String = 6,
    Raw = 7,
    Error = 8
  };

  template <typename D, Type t> struct Value {
    public:
      Type type = t;
      D data;

      const SSC::String typeof () const {
        switch (this->type) {
          case Type::Empty: return SSC::String("empty");
          case Type::Raw: return SSC::String("raw");
          case Type::Any: return SSC::String("any");
          case Type::Array: return SSC::String("array");
          case Type::Boolean: return SSC::String("boolean");
          case Type::Number: return SSC::String("number");
          case Type::Null: return SSC::String("null");
          case Type::Object: return SSC::String("object");
          case Type::String: return SSC::String("string");
          case Type::Error: return SSC::String("error");
        }
      }

      bool isError () const { return this->type == Type::Error; }
      bool isRaw () const { return this->type == Type::Raw; }
      bool isArray () const { return this->type == Type::Array; }
      bool isBoolean () const { return this->type == Type::Boolean; }
      bool isNumber () const { return this->type == Type::Number; }
      bool isNull () const { return this->type == Type::Null; }
      bool isObject () const { return this->type == Type::Object; }
      bool isString () const { return this->type == Type::String; }
      bool isEmpty () const { return this->type == Type::Empty; }

      const SSC::String str () const {
        return "";
      }
  };

  class Error : public std::invalid_argument, public Value<SSC::String, Type::Error> {
    public:
      int code = 0;
      SSC::String name;
      SSC::String message;
      SSC::String location;

      Error ();
      Error (const SSC::String& message);
      Error (const Error& error);
      Error (Error* error);
      Error (
        const SSC::String& name,
        const SSC::String& message,
        int code = 0
      );
      Error (
        const SSC::String& name,
        const SSC::String& message,
        const SSC::String& location
      );

      const SSC::String value () const;
      const char* what () const noexcept override;
      const SSC::String str () const;
  };

  class Null : public Value<std::nullptr_t, Type::Null> {
    public:
      Null () = default;
      Null (std::nullptr_t);
      std::nullptr_t value () const;
      const SSC::String str () const;
  };

  class Any : public Value<void *, Type::Any> {
    public:
      SharedPointer<void> pointer = nullptr;

      Any ();
      Any (const Any& any);
      Any (Type type, SharedPointer<void> pointer);
      Any (std::nullptr_t);
      Any (const Null);

      Any (bool);
      Any (int64_t);
      Any (uint64_t);
      Any (uint32_t);
      Any (int32_t);
      Any (double);
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      Any (size_t);
      Any (ssize_t);
    #elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
      Any (long long);
    #endif

      Any (Atomic<bool>&);
      Any (Atomic<int64_t>&);
      Any (Atomic<uint64_t>&);
      Any (Atomic<uint32_t>&);
      Any (Atomic<int32_t>&);
      Any (Atomic<double>&);
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      Any (Atomic<size_t>&);
      Any (Atomic<ssize_t>&);
    #elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
      Any (Atomic<long long>&);
    #endif

      Any (const char);
      Any (const char *);

      Any (const SSC::String&);
      Any (const SSC::Path&);

      Any (const Boolean&);
      Any (const Number&);
      Any (const String&);
      Any (const Object&);
      Any (const Array&);
      Any (const Raw&);
      Any (const Error&);
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      Any (const NSError*);
    #elif SOCKET_RUNTIME_PLATFORM_LINUX
      Any (const GError*);
    #endif

      Any (const ArrayEntries&);
      Any (const ObjectEntries&);

      ~Any ();

      Any operator[](const SSC::String& key) const;
      Any& operator[](const SSC::String& key);
      Any operator[](const unsigned int index) const;
      Any& operator[](const unsigned int index);

      bool operator == (const Any&) const;
      bool operator != (const Any&) const;

      template <typename T> T& as () const {
        auto ptr = this->pointer.get();

        if (ptr != nullptr && this->type != Type::Null) {
          return *reinterpret_cast<T *>(ptr);
        }

        throw Error("BadCastError", "cannot cast to null value", __PRETTY_FUNCTION__);
      }

      const SSC::String str () const;
  };

  class Raw : public Value<SSC::String, Type::Raw> {
    public:
      Raw (const Raw& raw);
      Raw (const Raw* raw);
      Raw (const SSC::String& source);
      const SSC::String str () const;
  };

  class Object : public Value<ObjectEntries, Type::Object> {
    public:
      using Entries = ObjectEntries;
      using const_iterator = Entries::const_iterator;
      Object () = default;
    #if SOCKET_RUNTIME_PLATFORM_LINUX
      Object (JSCValue* value);
    #endif
      Object (const std::map<SSC::String, int>& entries);
      Object (const std::map<SSC::String, bool>& entries);
      Object (const std::map<SSC::String, double>& entries);
      Object (const std::map<SSC::String, int64_t>& entries);
      Object (const Object::Entries& entries);
      Object (const Object& object);
      Object (const std::map<SSC::String, SSC::String>& map);
      Object (const Error& error);

      Any operator [] (const SSC::String& key) const;
      Any &operator [] (const SSC::String& key);

      const SSC::String str () const;
      const Object::Entries value () const;
      const Any& get (const SSC::String& key) const;
      Any& get (const SSC::String& key);
      void set (const SSC::String& key, const Any& value);
      bool has (const SSC::String& key) const;
      bool contains (const SSC::String& key) const;
      Entries::size_type size () const;
      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
  };

  class Array : public Value<ArrayEntries, Type::Array> {
    public:
      using Entries = ArrayEntries;
      using const_iterator = Entries::const_iterator;
      Array () = default;
      Array (const Array& array);
      Array (const Array::Entries& entries);
    #if SOCKET_RUNTIME_PLATFORM_LINUX
      Array (JSCValue* value);
    #endif

      Any operator [] (const unsigned int index) const;
      Any &operator [] (const unsigned int index);

      const SSC::String str () const;
      Array::Entries value () const;
      bool has (const unsigned int index) const;
      Entries::size_type size () const;
      Any get (const unsigned int index) const;
      void set (const unsigned int index, const Any& value);
      void push (Any value);
      Any& pop ();
      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
  };

  class Boolean : public Value<bool, Type::Boolean> {
    public:
      Boolean () = default;
      Boolean (const Boolean& boolean);
      Boolean (bool boolean);
      Boolean (int data);
      Boolean (int64_t data);
      Boolean (double data);
      Boolean (void *data);
      Boolean (const SSC::String& string);

      bool value () const;
      const SSC::String str () const;
  };

  class Number : public Value<double, Type::Number> {
    public:
      Number () = default;
      Number (const Number& number);
      Number (double number);
      Number (char number);
      Number (int number);
      Number (int64_t number);
      Number (bool number);
      Number (const String& string);

      float value () const;
      const SSC::String str () const;
  };

  class String : public Value<SSC::String, Type::String> {
    public:
      String () = default;
      String (const String& data);
      String (const SSC::String& data);
      String (const char data);
      String (const char *data);
      String (const Any& any);
      String (const Number& number);
      String (const Boolean& boolean);
      String (const Error& error);

      const SSC::String str () const;
      SSC::String value () const;
      SSC::String::size_type size () const;
  };

  extern Null null;
  extern Any anyNull;

  inline const auto typeof (const Any& any) {
    return any.typeof();
  }
}
#endif
