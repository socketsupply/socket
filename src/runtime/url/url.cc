#include "../url.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::url {
  const URL::Components URL::Components::parse (const String& url) {
    URL::Components components;
    components.originalURL = url;
    auto input = url;

    if (input.starts_with("./")) {
      input = input.substr(1);
    }

    if (!input.starts_with("/")) {
      const auto colon = input.find(':');

      if (colon != String::npos) {
        components.scheme = input.substr(0, colon);
        input = input.substr(colon + 1, input.size());

        if (input.starts_with("//")) {
          input = input.substr(2, input.size());

          const auto slash = input.find("/");
          if (slash != String::npos) {
            components.authority = input.substr(0, slash);
            input = input.substr(slash, input.size());
          } else {
            const auto questionMark = input.find("?");
            const auto fragment = input.find("#");
            if (questionMark != String::npos & fragment != String::npos) {
              if (questionMark < fragment) {
                components.authority = input.substr(0, questionMark);
                input = input.substr(questionMark, input.size());
              } else {
                components.authority = input.substr(0, fragment);
                input = input.substr(fragment, input.size());
              }
            } else if (questionMark != String::npos) {
              components.authority = input.substr(0, questionMark);
              input = input.substr(questionMark, input.size());
            } else if (fragment != String::npos) {
              components.authority = input.substr(0, fragment);
              input = input.substr(fragment, input.size());
            } else {
              components.authority = input;
              components.pathname = "/";
            }
          }
        }
      }
    }

    if (components.pathname.size() == 0) {
      const auto questionMark = input.find("?");
      const auto fragment = input.find("#");

      if (questionMark != String::npos && fragment != String::npos) {
        if (questionMark < fragment) {
          components.pathname = input.substr(0, questionMark);
          components.query = input.substr(questionMark + 1, fragment - questionMark - 1);
          components.fragment = input.substr(fragment + 1, input.size());
        } else {
          components.pathname = input.substr(0, fragment);
          components.fragment = input.substr(fragment + 1, input.size());
        }
      } else if (questionMark != String::npos) {
        components.pathname = input.substr(0, questionMark);
        components.query = input.substr(questionMark + 1, input.size());
      } else if (fragment != String::npos) {
        components.pathname = input.substr(0, fragment);
        components.fragment = input.substr(fragment + 1, input.size());
      } else {
        components.pathname = input;
      }

      if (!components.pathname.starts_with("/")) {
        components.pathname = "/" + components.pathname;
      }
    }

    return components;
  }

  URL::URL (const URL& url) {
    this->href = url.href;
    this->origin = url.origin;
    this->protocol = url.protocol;
    this->username = url.username;
    this->password = url.password;
    this->hostname = url.hostname;
    this->port = url.port;
    this->pathname = url.pathname;
    this->search = url.search;
    this->hash = url.hash;

    this->scheme = url.scheme;
    this->fragment = url.fragment;
    this->query = url.query;

    this->searchParams = url.searchParams;
    this->pathComponents = url.pathComponents;
  }

  URL::URL (URL&& url) {
    this->href = url.href;
    this->origin = url.origin;
    this->protocol = url.protocol;
    this->username = url.username;
    this->password = url.password;
    this->hostname = url.hostname;
    this->port = url.port;
    this->pathname = url.pathname;
    this->search = url.search;
    this->hash = url.hash;

    this->scheme = url.scheme;
    this->fragment = url.fragment;
    this->query = url.query;

    this->searchParams = std::move(url.searchParams);
    this->pathComponents = std::move(url.pathComponents);

    url.href = "";
    url.origin = "";
    url.protocol = "";
    url.username = "";
    url.password = "";
    url.hostname = "";
    url.port = "";
    url.pathname = "";
    url.search = "";
    url.hash = "";
    url.scheme = "";
    url.fragment = "";
    url.query = "";

    url.searchParams = SearchParams {};
    url.pathComponents = PathComponents {};
  }

  URL::URL (const JSON::Object& json)
    : URL(
        json.has("href")
          ? json.get("href").str()
          : ""
      )
  {}

  URL::URL (const String& href, bool decodeURIComponents)
    : searchParams(SearchParams::Options { decodeURIComponents })
  {
    if (href.size() > 0) {
      this->set(href, decodeURIComponents);
    }
  }

  URL& URL::operator = (const URL& url) {
    this->href = url.href;
    this->origin = url.origin;
    this->protocol = url.protocol;
    this->username = url.username;
    this->password = url.password;
    this->hostname = url.hostname;
    this->port = url.port;
    this->pathname = url.pathname;
    this->search = url.search;
    this->hash = url.hash;

    this->scheme = url.scheme;
    this->fragment = url.fragment;
    this->query = url.query;

    this->searchParams = url.searchParams;
    this->pathComponents = url.pathComponents;
    return *this;
  }

  URL& URL::operator = (URL&& url) {
    this->href = url.href;
    this->origin = url.origin;
    this->protocol = url.protocol;
    this->username = url.username;
    this->password = url.password;
    this->hostname = url.hostname;
    this->port = url.port;
    this->pathname = url.pathname;
    this->search = url.search;
    this->hash = url.hash;

    this->scheme = url.scheme;
    this->fragment = url.fragment;
    this->query = url.query;

    this->searchParams = std::move(url.searchParams);
    this->pathComponents = std::move(url.pathComponents);

    url.href = "";
    url.origin = "";
    url.protocol = "";
    url.username = "";
    url.password = "";
    url.hostname = "";
    url.port = "";
    url.pathname = "";
    url.search = "";
    url.hash = "";
    url.scheme = "";
    url.fragment = "";
    url.query = "";

    url.searchParams = SearchParams {};
    url.pathComponents = PathComponents {};
    return *this;
  }

  void URL::set (const String& href, bool decodeURIComponents) {
    const auto components = URL::Components::parse(href);

    this->scheme = components.scheme;
    this->pathname = components.pathname;
    this->query = components.query;
    this->fragment = components.fragment;
    this->search = this->query.size() > 0 ? "?" + this->query : "";
    this->hash = this->fragment.size() > 0 ? "#" + this->fragment : "";

    if (components.scheme.size() > 0) {
      this->protocol = components.scheme + ":";
    }

    const auto authorityParts = components.authority.size() > 0
      ? split(components.authority, '@')
      : Vector<String> {};

    if (authorityParts.size() == 2) {
      const auto userParts = split(authorityParts[0], ':');

      if (userParts.size() == 2) {
        this->username = userParts[0];
        this->password = userParts[1];
      } else if (userParts.size() == 1) {
        this->username = userParts[0];
      }

      const auto hostParts = split(authorityParts[1], ':');
      if (hostParts.size() > 1) {
        this->port = hostParts[1];
      }

      if (hostParts.size() > 0) {
        this->hostname = hostParts[0];
      }
    } else if (authorityParts.size() == 1) {
      const auto hostParts = split(authorityParts[0], ':');
      if (hostParts.size() > 1) {
        this->port = hostParts[1];
      }

      if (hostParts.size() > 0) {
        this->hostname = hostParts[0];
      }
    }

    if (this->protocol.size() > 0) {
      if (this->hostname.size() > 0) {
        this->origin = this->protocol + "//" + this->hostname;
      } else {
        this->origin = this->protocol + this->pathname;
      }

      this->href = this->origin + this->pathname + this->search + this->hash;
    }

    if (this->query.size() > 0) {
      for (const auto& entry : split(this->query, '&')) {
        const auto parts = split(entry, '=');
        if (parts.size() == 2) {
          const auto key = trim(parts[0]);
          const auto value = trim(parts[1]);
          this->searchParams.set(key, value);
        }
      }
    }

    if (this->pathname.size() > 0) {
      this->pathComponents.set(this->pathname);
    }
  }

  const String URL::str () const {
    return this->href;
  }

  const char* URL::c_str () const {
    return this->href.c_str();
  }

  const JSON::Object URL::json () const {
    return JSON::Object::Entries {
      {"href", this->href},
      {"origin", this->origin},
      {"protocol", this->protocol},
      {"username", this->username},
      {"password", this->password},
      {"hostname", this->hostname},
      {"pathname", this->pathname},
      {"search", this->search},
      {"hash", this->hash}
    };
  }

  const size_t URL::size () const {
    return this->href.size();
  }
}