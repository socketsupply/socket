#ifndef SOCKET_RUNTIME_RUNTIME_URL_H
#define SOCKET_RUNTIME_RUNTIME_URL_H

#include "json.hh"
#include "debug.hh"

namespace ssc::runtime::url {
  struct PathComponents {
    using Iterator = Vector<String>::const_iterator;
    using const_iterator = Vector<String>::const_iterator;

    Vector<String> parts;

    PathComponents () = default;
    PathComponents (const String&);

    const String operator [] (const size_t) const;
    const String& operator [] (const size_t);

    void set (const String&);
    const String& at (const size_t) const;
    const String str () const noexcept;
    const Iterator begin () const noexcept;
    const size_t size () const noexcept;
    const Iterator end () const noexcept;
    const bool empty () const noexcept;

    template <typename T>
      const T get (const size_t) const;
  };

  class SearchParams {
    public:
      class Value : public JSON::Raw {
        public:
          bool decodeURIComponents = true;

          Value () = default;
          Value (const Value&);
          Value (Value&&);
          Value (const String&);
          Value (const JSON::Any&);
          Value (const std::nullptr_t );

          Value& operator = (const Value&);
          Value& operator = (Value&&);
          Value& operator = (const String&);
          Value& operator = (const JSON::Any&);
          Value& operator = (const std::nullptr_t&);

          const String str () const;

          template <typename T>
            const T as () const {
              return T(this->value());
            }
      };

      using Entries = Map<String, Value>;
      using const_iterator = Entries::const_iterator;
      using iterator = Entries::iterator;

      struct Options {
        bool decodeURIComponents = true;
      };

      Options options;
      Map<String, Value> data;

      SearchParams () = default;
      SearchParams (const Options&);
      SearchParams (const SearchParams&);
      SearchParams (SearchParams&&);

      SearchParams (const Map<String, String>&, const Options& = {
        .decodeURIComponents = true
      });

      SearchParams (const JSON::Object&, const Options& = {
        .decodeURIComponents = true
      });

      SearchParams (const String&, const Options& = {
        .decodeURIComponents = true
      });

      SearchParams& operator = (const SearchParams&);
      SearchParams& operator = (SearchParams&&);

      Value operator [] (const String&) const;
      Value& operator [] (const String&);

      SearchParams& set (const String&, const Value&);
      SearchParams& set (const String&);
      const Value& at (const String&) const;
      const Value get (const String&) const;
      const Value get (const String&);
      bool contains (const String&) const;
      Entries::size_type size () const;
      const const_iterator begin () const noexcept;
      const const_iterator end () const noexcept;
      iterator begin () noexcept;
      iterator end () noexcept;

      const String str () const;
      const Map<String, String> map () const;
      const JSON::Object json () const;
  };

  struct URL {
    struct Components {
      String originalURL = "";
      String scheme = "";
      String authority = "";
      String pathname = "";
      String query = "";
      String fragment = "";

      static const Components parse (const String&);
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

      bool decodeURIComponents = false;

      Builder& setProtocol (const String&);
      Builder& setUsername (const String&);
      Builder& setPassword (const String&);
      Builder& setHostname (const String&);
      Builder& setPort (const String&);
      Builder& setPort (const int);
      Builder& setPathname (const String&);
      Builder& setQuery (const String&);
      Builder& setSearch (const String&);
      Builder& setHash (const String&);
      Builder& setFragment (const String&);
      Builder& setSearchParam (const String&, const String&);
      Builder& setSearchParam (const String&, const JSON::Any&);
      Builder& setSearchParams (const Map<String, String>&);
      Builder& setDecodeURIComponents (bool);

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
    String scheme = "";
    String fragment = "";
    String query = "";

    SearchParams searchParams;
    PathComponents pathComponents;

    URL () = default;
    URL (const URL&);
    URL (URL&&);
    URL (const String& href, bool decodeURIComponents = false);
    URL (const JSON::Object& json);

    URL& operator = (const URL&);
    URL& operator = (URL&&);

    void set (const String& href, bool decodeURIComponents = false);
    void set (const JSON::Object& json);
    const String str () const;
    const char* c_str () const;
    const JSON::Object json () const;
    const size_t size () const;
  };

  size_t url_encode_uri_component (
    const char *input,
    size_t inputSize,
    char *output,
    size_t outputSize
  );

  size_t url_decode_uri_component (
    const char *input,
    size_t inputSize,
    char *output,
    size_t outputSize
  );

  /**
   * Encodes input by replacing certain characters by
   * one, two, three, or four escape sequences representing the UTF-8
   * encoding of the character.
   * @param input The input string to encode
   * @return An encoded string value
   */
  String encodeURIComponent (const String& input);

  /**
   * Decodes a value encoded with `encodeURIComponent`
   * @param input The input string to decode
   * @return A decoded string
   */
  String decodeURIComponent (const String& input);
}

namespace ssc::runtime {
  using URL = url::URL;
}
#endif
