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
    Map searchParams;

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
