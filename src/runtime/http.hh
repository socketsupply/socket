#ifndef SOCKET_RUNTIME_RUNTIME_HTTP_H
#define SOCKET_RUNTIME_RUNTIME_HTTP_H

#include "bytes.hh"
#include "json.hh"
#include "url.hh"

namespace ssc::runtime::http {
  struct Status {
    int code = 200;
    String text = "OK";
    Status () = default;
    Status (int, const String&);
    Status (int);
    Status (const String&);
    String str () const;
    JSON::Object json () const;
  };

  String getStatusText (int);
  int getStatusCode (const String&);
  const Map<int, Status>& getStatusMap ();

  const String toHeaderCase (const String&);

  class Headers {
    public:
      class Value {
        public:
          String string;
          Value () = default;
          Value (const String&);
          Value (const char*);
          Value (const Value&);
          Value (bool);
          Value (int);
          Value (float);
          Value (int64_t);
          Value (uint64_t);
          Value (double_t);
          Value (size_t value);
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
          Header (const Header&);
          Header (const String&, const Value&);
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
      Headers (const Headers&);
      Headers (const String&);
      Headers (const Vector<Map<String, Value>>&);
      Headers (const Entries&);
      size_t size () const;
      String str () const;
      bool empty () const;

      Headers& set (const String&, const Value&) noexcept;
      Headers& set (const Header&) noexcept;
      bool has (const String&) const noexcept;
      const Header get (const String&) const noexcept;
      Header& at (const String&);
      const Iterator begin () const noexcept;
      const Iterator end () const noexcept;
      bool erase (const String&) noexcept;
      const bool clear () noexcept;
      String& operator [] (const String&);
      const String operator [] (const String&) const noexcept;
      JSON::Object json () const noexcept;
  };

  class Request {
    public:
      String version = "1.1";
      String method = "";
      String scheme = "http";
      URL url;
      Headers headers;
      bytes::Buffer body;

      Request () = default;
      Request (const String&);
      Request (const unsigned char*, size_t = -1);
      String str () const;
      bool valid () const;
  };

  class Response {
    public:
      String version = "1.1";
      Status status;
      Headers headers;
      bytes::Buffer body;

      Response (const String&);
      Response (const Status&, const String&);
      Response (const Status&, const JSON::Any&);
      Response (const Headers&);
      Response (const Status&);
      Response (const Status&, const Headers&);

      Response& setHeader (const Headers::Header&);
      Response& setHeader (const String&, const Headers::Value&);

      const unsigned char* data () const;
      size_t size () const;
      String str () const;
  };
}
#endif
