#ifndef SOCKET_RUNTIME_CORE_HEADERS_H
#define SOCKET_RUNTIME_CORE_HEADERS_H

#include "json.hh"

namespace SSC {
  class Headers {
    public:
      class Value {
        public:
          String string;
          Value () = default;
          Value (const String& value);
          Value (const char* value);
          Value (const Value& value);
          Value (bool value);
          Value (int value);
          Value (float value);
          Value (int64_t value);
          Value (uint64_t value);
          Value (double_t value);
        #if SOCKET_RUNTIME_PLATFORM_APPLE
          Value (ssize_t value);
        #endif
          bool operator == (const Value&) const;
          bool operator != (const Value&) const;
          bool operator == (const String&) const;
          bool operator != (const String&) const;
          const String operator + (const String&) const;

          const String& str () const;
          const char * c_str() const;
          bool empty () const;
          size_t size () const;

          template <typename T> void set (T value) {
            auto v = Value(value);
            this->string = v.string;
          }
      };

      class Header {
        public:
          String name;
          Value value;
          Header () = default;
          Header (const Header& header);
          Header (const String& name, const Value& value);
          bool operator == (const Header&) const;
          bool operator != (const Header&) const;
          bool operator == (const String&) const;
          bool operator != (const String&) const;
          const String operator + (const String&) const;
          size_t size () const;
          bool empty () const;
      };

      using Entries = Vector<Header>;
      using Iterator = Entries::const_iterator;

      Entries entries;
      Headers () = default;
      Headers (const Headers& headers);
      Headers (const String& source);
      Headers (const Vector<std::map<String, Value>>& entries);
      Headers (const Entries& entries);
      size_t size () const;
      String str () const;
      bool empty () const;

      void set (const String& name, const String& value) noexcept;
      void set (const Header& header) noexcept;
      bool has (const String& name) const noexcept;
      const Header get (const String& name) const noexcept;
      Header& at (const String& name);
      const Iterator begin () const noexcept;
      const Iterator end () const noexcept;
      bool erase (const String& name) noexcept;
      const bool clear () noexcept;
      String& operator [] (const String& name);
      const String operator [] (const String& name) const noexcept;
      JSON::Object json () const noexcept;
  };

  const String toHeaderCase (const String& source);
}

#endif
