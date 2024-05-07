#include "codec.hh"
#include "string.hh"
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
            }
          }
        }
      }
    }

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

    return components;
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
      const auto hostParts = split(authorityParts[1], ':');
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
          this->searchParams.insert_or_assign(key, value);
        }
      }
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
