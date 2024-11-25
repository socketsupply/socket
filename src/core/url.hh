#ifndef SOCKET_RUNTIME_CORE_URL_H
#define SOCKET_RUNTIME_CORE_URL_H

#include "json.hh"

namespace SSC {
  struct URL {
    struct Components {
      String originalURL = "";
      String scheme = "";
      String authority = "";
      String pathname = "";
      String query = "";
      String fragment = "";

      static const Components parse (const String& url);
    };

    struct PathComponents {
      using Iterator = Vector<String>::const_iterator;
      using const_iterator = Vector<String>::const_iterator;

      Vector<String> parts;

      PathComponents () = default;
      PathComponents (const String& pathname);
      const String operator[] (const size_t index) const;
      const String& operator[] (const size_t index);

      void set (const String& pathname);
      const String& at (const size_t index) const;
      const String str () const noexcept;
      const Iterator begin () const noexcept;
      const size_t size () const noexcept;
      const Iterator end () const noexcept;
      const bool empty () const noexcept;

      template <typename T>
      const T get (const size_t index) const;
    };

    class SearchParams : public JSON::Object {
      public:
        class Value : public JSON::Any {
          public:
            Value (const Any& value)
              : JSON::Any(value.str())
            {}
        };

        using Entries = JSON::Object::Entries;
        SearchParams () = default;
        SearchParams (const SearchParams&);
        SearchParams (const String&);
        SearchParams (const Map&);
        SearchParams (const JSON::Object&);
        const String str () const;
    };

    struct Builder {
      String protocol = "";
      String username = "";
      String password = "";
      String hostname = "";
      String port = "";
      String pathname = "";
      String search = ""; // includes '?' and 'query' if 'query' is not empty
      String hash = ""; // include '#' and 'fragment' if 'fragment' is not empty

      Builder& setProtocol (const String& protocol);
      Builder& setUsername (const String& username);
      Builder& setPassword (const String& password);
      Builder& setHostname (const String& hostname);
      Builder& setPort (const String& port);
      Builder& setPort (const int port);
      Builder& setPathname (const String& pathname);
      Builder& setQuery (const String& query);
      Builder& setSearch (const String& search);
      Builder& setHash (const String& hash);
      Builder& setFragment (const String& fragment);
      Builder& setSearchParam (const String& key, const String& value);
      Builder& setSearchParam (const String& key, const JSON::Any& value);
      Builder& setSearchParams (const Map& params);

      URL build () const;
    };

    // core properties
    String href = "";
    String origin = "";
    String protocol = "";
    String username = "";
    String password = "";
    String hostname = "";
    String port = "";
    String pathname = "";
    String search = ""; // includes '?' and 'query' if 'query' is not empty
    String hash = ""; // include '#' and 'fragment' if 'fragment' is not empty

    // extra properties
    String scheme;
    String fragment;
    String query;

    SearchParams searchParams;

    PathComponents pathComponents;

    URL () = default;
    URL (const String& href);
    URL (const JSON::Object& json);

    void set (const String& href);
    void set (const JSON::Object& json);
    const String str () const;
    const JSON::Object json () const;
  };
}
#endif
