#ifndef SOCKET_RUNTIME_RUNTIME_JSON_H
#define SOCKET_RUNTIME_RUNTIME_JSON_H

#include "platform.hh"
#include "crypto.hh"

namespace ssc::runtime::JSON {
  // forward
  class Any;
  class Raw;
  class Null;
  class Object;
  class Array;
  class Boolean;
  class Number;
  class String;

  using ObjectEntries = Map<runtime::String, Any>;
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

  class Entity {
    public:
      using ID = uint64_t;

      ID id = crypto::rand64();

      virtual ~Entity() = 0;

      operator bool () const;

      const runtime::String typeof () const;
      bool isError () const;
      bool isRaw () const;
      bool isArray () const;
      bool isBoolean () const;
      bool isNumber () const;
      bool isNull () const;
      bool isObject () const;
      bool isString () const;
      bool isEmpty () const;

      const ID getEntityID ();
      virtual Type getEntityType () const = 0;
      virtual bool getEntityBooleanValue () const = 0;
      virtual const runtime::String str () const = 0;
  };

  class SharedEntityPointer {
    public:
      struct ControlBlock {
        std::atomic_size_t size;
        Entity* entity;

        ControlBlock (Entity*);
      };

      SharedEntityPointer (Entity* = nullptr);
      SharedEntityPointer (const SharedEntityPointer&);
      SharedEntityPointer (SharedEntityPointer&&);
      ~SharedEntityPointer ();

      SharedEntityPointer& operator = (const SharedEntityPointer&);
      SharedEntityPointer& operator = (SharedEntityPointer&&);
      Entity* operator -> () const;
      operator bool () const;

      void reset (Entity* = nullptr);
      size_t use_count () const;
      void swap (SharedEntityPointer&);

      Entity* get () const;
      template <typename T> T* as () const;

    protected:
      SharedPointer<ControlBlock> control = nullptr;
  };

  template <typename T = Null, typename... Args>
  SharedEntityPointer make_shared (Args... args) {
    static_assert(std::is_base_of<Entity, T>::value, "T must derive from Entity");
    return new T(args...);
  }

  template <typename D, Type t> class Value : public Entity {
    public:
      using DataType = D;
      Type type = t;
      DataType data;

      virtual const DataType value () const = 0;

      Type getEntityType () const override {
        return this->type;
      }

      bool getEntityBooleanValue () const override {
        return Entity::getEntityBooleanValue();
      }
  };

  class Error : public std::invalid_argument, public Value<runtime::String, Type::Error> {
    public:
      static Type valueType;
      int code = 0;
      runtime::String name;
      runtime::String message;
      runtime::String location;

      Error ();
      Error (const runtime::String& message);
      Error (const Error&);
      Error (Error*);
      Error (
        const runtime::String& name,
        const runtime::String& message,
        int code = 0
      );
      Error (
        const runtime::String& name,
        const runtime::String& message,
        const runtime::String& location
      );

      const runtime::String value () const override;
      const char* what () const noexcept override;
      const runtime::String str () const override;
  };

  class Null : public Value<std::nullptr_t, Type::Null> {
    public:
      static Type valueType;
      Null () = default;
      Null (std::nullptr_t);
      const std::nullptr_t value () const override;
      const runtime::String str () const override;
  };

  class Any : public Value<SharedEntityPointer, Type::Any> {
    public:
      static Type valueType;

      Any (Type, const SharedEntityPointer&);
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

      Any (const runtime::String&);
      Any (const runtime::Path&);
      Any (const runtime::Map<runtime::String, runtime::String>&);
      Any (const runtime::Map<runtime::String, std::nullptr_t>&);
      Any (const runtime::Map<runtime::String, const Null>&);
      Any (const runtime::Map<runtime::String, bool>&);
      Any (const runtime::Map<runtime::String, int64_t>&);
      Any (const runtime::Map<runtime::String, uint64_t>&);
      Any (const runtime::Map<runtime::String, uint32_t>&);
      Any (const runtime::Map<runtime::String, int32_t>&);
      Any (const runtime::Map<runtime::String, double>&);
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      Any (const runtime::Map<runtime::String, size_t>&);
      Any (const runtime::Map<runtime::String, ssize_t>&);
    #elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
      Any (const runtime::Map<runtime::String, long long>&);
    #endif

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

      Any ();
      Any (Any&&);
      Any (const Any& any);
      ~Any ();

      Any& operator = (const Any&);
      Any& operator = (Any&&);

      Any& operator [] (const char*);
      Any& operator [] (const runtime::String&);
      Any& operator [] (const unsigned int);

      bool operator == (const Any&) const;
      bool operator != (const Any&) const;

      template <typename T> T& as () const;
      Any& at (const runtime::String&);
      Any& at (const unsigned int);
      const SharedEntityPointer value () const override;
      const runtime::String str () const override;
  };

  class Raw : public Value<runtime::String, Type::Raw> {
    public:
      static Type valueType;
      Raw () = default;
      Raw (const Raw&);
      Raw (Raw&&);
      Raw (const Raw*);
      Raw (const Any&);
      Raw (const runtime::String&);
      Raw& operator = (const Raw&);
      Raw& operator = (Raw&&);
      const runtime::String value () const override;
      const runtime::String str () const override;
  };

  class Object : public Value<ObjectEntries, Type::Object> {
    public:
      static Type valueType;
      using Entries = ObjectEntries;
      using const_iterator = Entries::const_iterator;
      using iterator = Entries::iterator;
      Object () = default;
    #if SOCKET_RUNTIME_PLATFORM_LINUX
      Object (JSCValue*);
    #endif
      Object (const Map<runtime::String, runtime::String>& entries);
      Object (const Object::Entries& entries);
      Object (const runtime::Map<runtime::String, std::nullptr_t>&);
      Object (const runtime::Map<runtime::String, const Null>&);
      Object (const runtime::Map<runtime::String, bool>&);
      Object (const runtime::Map<runtime::String, int64_t>&);
      Object (const runtime::Map<runtime::String, uint64_t>&);
      Object (const runtime::Map<runtime::String, uint32_t>&);
      Object (const runtime::Map<runtime::String, int32_t>&);
      Object (const runtime::Map<runtime::String, double>&);
    #if SOCKET_RUNTIME_PLATFORM_APPLE
      Object (const runtime::Map<runtime::String, size_t>&);
      Object (const runtime::Map<runtime::String, ssize_t>&);
    #elif !SOCKET_RUNTIME_PLATFORM_WINDOWS
      Object (const runtime::Map<runtime::String, long long>&);
    #endif
      Object (const Object&);
      Object (const Error&);

      Any& operator [] (const char*);
      Any& operator [] (const runtime::String&);

      const runtime::String str () const override;
      const Object::Entries value () const override;
      const Any& get (const runtime::String&) const;
      const Any& get (const runtime::String&);
      Any& at (const runtime::String&);
      void set (const runtime::String&, const Any&);
      bool has (const runtime::String&) const;
      bool contains (const runtime::String&) const;
      Entries::size_type size () const;
      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
      iterator begin () noexcept;
      iterator end () noexcept;
  };

  class Array : public Value<ArrayEntries, Type::Array> {
    public:
      static Type valueType;
      using Entries = ArrayEntries;
      using const_iterator = Entries::const_iterator;
      using iterator = Entries::iterator;
      Array () = default;
      Array (const Array&);
      Array (const Array::Entries&);
    #if SOCKET_RUNTIME_PLATFORM_LINUX
      Array (JSCValue*);
    #endif

      Any& operator [] (const unsigned int);

      const runtime::String str () const override;
      const Array::Entries value () const override;
      bool has (const unsigned int) const;
      Entries::size_type size () const;
      const Any& get (const unsigned int) const;
      Any& at (const unsigned int);
      void set (const unsigned int, const Any&);
      void push (Any);
      const Any pop ();
      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
      iterator begin () noexcept;
      iterator end () noexcept;
  };

  class Boolean : public Value<bool, Type::Boolean> {
    public:
      static Type valueType;
      Boolean () = default;
      Boolean (const Boolean&);
      Boolean (bool);
      Boolean (int);
      Boolean (int64_t);
      Boolean (double);
      Boolean (void*);
      Boolean (const runtime::String&);

      const bool value () const override;
      const runtime::String str () const override;
  };

  class Number : public Value<double, Type::Number> {
    public:
      static Type valueType;
      Number () = default;
      Number (const Number&);
      Number (double);
      Number (char);
      Number (int);
      Number (int64_t);
      Number (bool);
      Number (const String&);

      const double value () const override;
      const runtime::String str () const override;
  };

  class String : public Value<runtime::String, Type::String> {
    public:
      static Type valueType;
      String () = default;
      String (const String&);
      String (const runtime::String&);
      String (const char);
      String (const char*);
      String (const Any&);
      String (const Number&);
      String (const Boolean&);
      String (const Error&);

      const runtime::String str () const override;
      const runtime::String value () const override;
      runtime::String::size_type size () const;
  };

  extern const Null null;
  extern const Any nullAny;

  inline const auto typeof (const Any& any) {
    return any.typeof();
  }
}
#endif
