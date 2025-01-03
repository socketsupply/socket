#include <type_traits>

#include "codec.hh"
#include "debug.hh"
#include "url.hh"

namespace SSC {
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

  URL::PathComponents::PathComponents (const String& pathname) {
    this->set(pathname);
  }

  void URL::PathComponents::set (const String& pathname) {
    const auto parts = split(pathname, "/");
    for (const auto& part : parts) {
      const auto value = trim(part);
      if (value.size() > 0) {
        this->parts.push_back(value);
      }
    }
  }

  const String URL::PathComponents::operator[] (const size_t index) const {
    return this->parts[index];
  }

  const String& URL::PathComponents::operator[] (const size_t index) {
    return this->parts[index];
  }

  const String& URL::PathComponents:: at (const size_t index) const {
    return this->parts.at(index);
  }

  const String URL::PathComponents::str () const noexcept {
    return "/" + join(this->parts, "/");
  }

  const URL::PathComponents::Iterator URL::PathComponents::begin () const noexcept {
    return this->parts.begin();
  }

  const URL::PathComponents::Iterator URL::PathComponents::end () const noexcept {
    return this->parts.end();
  }

  const size_t URL::PathComponents::size () const noexcept {
    return this->parts.size();
  }

  const bool URL::PathComponents::empty () const noexcept {
    return this->parts.empty();
  }

  template <typename T>
  const T URL::PathComponents::get (const size_t index) const {
    if (std::is_same<T, uint64_t>::value) {
      return std::stoull(this->at(index));
    }

    if (std::is_same<T, int64_t>::value) {
      return std::stoll(this->at(index));
    }

    if (std::is_same<T, uint32_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int32_t>::value) {
      return std::stol(this->at(index));
    }

    if (std::is_same<T, uint16_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int16_t>::value) {
      return std::stol(this->at(index));
    }

    if (std::is_same<T, uint8_t>::value) {
      return std::stoul(this->at(index));
    }

    if (std::is_same<T, int8_t>::value) {
      return std::stod(this->at(index));
    }

    if (std::is_same<T, bool>::value) {
      if (std::stod(this->at(index)) == 0) {
        return false;
      }

      return true;
    }

    throw new Error("unhandled type in URL::PathComponents::get");
  }

  template const uint64_t URL::PathComponents::get (const size_t index) const;
  template const int64_t URL::PathComponents::get (const size_t index) const;
  template const uint32_t URL::PathComponents::get (const size_t index) const;
  template const int32_t URL::PathComponents::get (const size_t index) const;
  template const uint16_t URL::PathComponents::get (const size_t index) const;
  template const int16_t URL::PathComponents::get (const size_t index) const;
  template const uint8_t URL::PathComponents::get (const size_t index) const;
  template const int8_t URL::PathComponents::get (const size_t index) const;
  template const bool URL::PathComponents::get (const size_t index) const;

  URL::SearchParams::SearchParams (const String& input) {
    const auto query = input.starts_with("?")
      ? input.substr(1)
      : input;

    for (const auto& entry : split(query, '&')) {
      const auto parts = split(entry, '=');
      if (parts.size() == 2) {
        const auto key = decodeURIComponent(trim(parts[0]));
        const auto value = decodeURIComponent(trim(parts[1]));
        this->set(key, value);
      }
    }
  }

  URL::SearchParams::SearchParams (const SearchParams& input) {
    for (const auto& entry : input) {
      this->set(entry.first, entry.second);
    }
  }

  URL::SearchParams::SearchParams (const Map& input) {
    for (const auto& entry : input) {
      this->set(entry.first, entry.second);
    }
  }

  URL::SearchParams::SearchParams (const JSON::Object& input) {
    for (const auto& entry : input) {
      this->set(entry.first, entry.second);
    }
  }

  const String URL::SearchParams::str () const {
    Vector<String> components;
    for (const auto& entry : *this) {
      const auto parts = Vector<String> {
        entry.first,
        entry.second.str()
      };

      components.push_back(join(parts, "="));
    }

    return join(components, "&");
  }

  URL::Builder& URL::Builder::setProtocol (const String& protocol) {
    this->protocol = protocol;
    return *this;
  }

  URL::Builder& URL::Builder::setUsername (const String& username) {
    this->username = username;
    return *this;
  }

  URL::Builder& URL::Builder::setPassword (const String& password) {
    this->password = password;
    return *this;
  }

  URL::Builder& URL::Builder::setHostname (const String& hostname) {
    this->hostname = hostname;
    return *this;
  }

  URL::Builder& URL::Builder::setPort (const String& port) {
    this->port = port;
    return *this;
  }

  URL::Builder& URL::Builder::setPort (const int port) {
    this->port = std::to_string(port);
    return *this;
  }

  URL::Builder& URL::Builder::setPathname (const String& pathname) {
    this->pathname = pathname;
    return *this;
  }

  URL::Builder& URL::Builder::setQuery (const String& query) {
    this->search = "?" + query;
    return *this;
  }

  URL::Builder& URL::Builder::setSearch (const String& search) {
    this->search = search;
    return *this;
  }

  URL::Builder& URL::Builder::setHash (const String& hash) {
    this->hash = hash;
    return *this;
  }

  URL::Builder& URL::Builder::setFragment (const String& fragment) {
    this->hash = "#" + fragment;
    return *this;
  }

  URL::Builder& URL::Builder::setSearchParam (const String& key, const String& value) {
    return this->setSearchParams(Map {{ key, value }});
  }

  URL::Builder& URL::Builder::setSearchParam (const String& key, const JSON::Any& value) {
    if (JSON::typeof(value) == "string" || JSON::typeof(value) == "number" || JSON::typeof(value) == "boolean") {
      return this->setSearchParam(key, value.str());
    }

    return *this;
  }

  URL::Builder& URL::Builder::setSearchParams (const Map& params) {
    if (params.size() > 0) {
      if (!this->search.starts_with("?")) {
        this->search = "?";
      } else if (this->search.size() > 0) {
        this->search += "&";
      }

      for (const auto& entry : params) {
        this->search = entry.first + "=" + entry.second + "&";
      }
    }

    if (this->search.ends_with("&")) {
      this->search = this->search.substr(0, this->search.size() - 1);
    }

    return *this;
  }

  URL URL::Builder::build () const {
    StringStream stream;

    if (this->protocol.size() == 0) {
      return String("");
    }

    stream << this->protocol << ":";

    if (
      (this->username.size() > 0 || this->password.size() > 0) &&
      this->hostname.size() > 0
    ) {
      stream << "//";
      if (this->username.size() > 0) {
        stream << this->username;
        if (this->password.size() > 0) {
          stream << ":" << this->password;
        }

        stream << "@" << this->hostname;
        if (this->port.size() > 0) {
          stream << ":" << this->port;
        }
      }
    }

    if (this->hostname.size() > 0 && this->pathname.size() > 0) {
      if (!this->pathname.starts_with("/")) {
        stream << "/";
      }
    }

    stream << this->pathname << this->search << this->hash;

    return stream.str();
  }

  URL::URL (const JSON::Object& json)
    : URL(json["href"].str())
  {}

  URL::URL (const String& href) {
    if (href.size() > 0) {
      this->set(href);
    }
  }

  void URL::set (const String& href) {
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
          const auto key = decodeURIComponent(trim(parts[0]));
          const auto value = decodeURIComponent(trim(parts[1]));
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
}
