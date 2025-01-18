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
    this->origin = url.origin;
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
    this->origin = url.origin;
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

    url.origin = "";
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

  URL::URL (
    const String& pathname,
    const String& base,
    bool decodeURIComponents
  ) : searchParams(SearchParams::Options { decodeURIComponents })
  {
    const auto parsed = Components::parse(pathname);
    if (base.size() > 0 && parsed.scheme.empty()) {
      auto components = URL::Components::parse(base);
      auto paths = split(components.pathname, '/');

      StringStream stream;

      if (pathname.starts_with("/")) {
        components.pathname = pathname;
      } else if (pathname.starts_with("./")) {
        if (components.pathname.ends_with("/")) {
          components.pathname = pathname.substr(2, pathname.size() - 1);
        } else {
          components.pathname = pathname.substr(1, pathname.size() - 1);
        }
      } else {
        Vector<String> parts;
        bool inBase = true;
        int offset = components.pathname.ends_with("/")
          ? paths.size()
          : paths.size() - 1;

        for (const auto& part : split(pathname, '/')) {
          if (part == "..") {
            if (inBase) {
              offset--;
            } else {
              parts.pop_back();
              if (parts.size() == 0) {
                inBase = true;
              }
            }
            continue;
          } else if (part == "." || part == "") {
            continue;
          } else {
            inBase = false;
            parts.push_back(part);
          }
        }

        if (parts.size() > 0) {
          components.pathname = components.pathname.substr(0, offset) + "/" + join(parts, '/');
        } else {
          components.pathname = components.pathname.substr(0, offset);
        }
      }

      if (components.scheme.size() > 0) {
        stream << components.scheme << ":";
        if (components.authority.size() > 0) {
          stream << "//" << components.authority;
        }

        stream << components.pathname;

        if (components.query.size() > 0) {
          stream << "?" << components.query;
        }

        if (components.fragment.size() > 0) {
          stream << "#" << components.fragment;
        }

        this->set(stream.str(), decodeURIComponents);
      }
    } else if (pathname.size() > 0) {
      this->set(pathname, decodeURIComponents);
    }
  }

  URL::URL (
    const String& pathname,
    const URL& base,
    bool decodeURIComponents
  ) : URL(pathname, base.href(), decodeURIComponents)
  {}

  URL& URL::operator = (const URL& url) {
    this->origin = url.origin;
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
    this->origin = url.origin;
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

    url.origin = "";
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

    if (this->scheme.size() > 0) {
      if (this->hostname.size() > 0) {
        this->origin = this->scheme + "://" + this->hostname;
      } else {
        this->origin = this->scheme + ":" + this->pathname;
      }
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

  const String URL::href () const {
    return this->str();
  }

  const String URL::str () const {
    StringStream stream;

    if (!this->scheme.empty() || this->scheme == "file") {
      stream << this->scheme << ":";

      if (!this->hostname.empty()) {
        stream << "//";
        if (!this->username.empty()) {
          stream << this->username;
          if (!this->password.empty()) {
            stream << ":" << this->password;
          }
          stream << "@";
        }
        stream << this->hostname;

        if (!this->port.empty()) {
          stream << ":" << this->port;
        }

        if (!this->pathname.starts_with("/")) {
           stream << "/";
        }
        stream << this->pathname;
      } else if (!this->pathname.empty()) {
        if (this->pathname.starts_with("/")) {
          stream << this->pathname.substr(1);
        } else {
          stream << this->pathname;
        }
      }

      auto params = SearchParams(this->searchParams);
      params.set(this->search);
      if (params.size() > 0) {
        stream << "?" << params.str();
      }
      stream << this->fragment;
    }

    return stream.str();
  }

  const JSON::Object URL::json () const {
    const auto search = this->searchParams.str();
    return JSON::Object::Entries {
      {"href", this->href()},
      {"origin", this->origin},
      {"username", this->username},
      {"password", this->password},
      {"hostname", this->hostname},
      {"pathname", this->pathname},
      {"search", "?" + search},
      {"hash", this->hash}
    };
  }

  const size_t URL::size () const {
    return this->str().size();
  }

  bool URL::empty () const {
    return this->str().empty();
  }
}
