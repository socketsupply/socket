#include "../url.hh"

namespace ssc::runtime::url {
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
    return this->setSearchParams(Map<String, String> {{ key, value }});
  }

  URL::Builder& URL::Builder::setSearchParam (const String& key, const JSON::Any& value) {
    if (JSON::typeof(value) == "string" || JSON::typeof(value) == "number" || JSON::typeof(value) == "boolean") {
      return this->setSearchParam(key, value.str());
    }

    return *this;
  }

  URL::Builder& URL::Builder::setSearchParams (const Map<String, String>& params) {
    if (params.size() > 0) {
      if (!this->search.starts_with("?")) {
        this->search = "?";
      } else if (this->search.size() > 0) {
        this->search += "&";
      }

      for (const auto& entry : params) {
        this->search += encodeURIComponent(entry.first) + "=" + encodeURIComponent(entry.second) + "&";
      }
    }

    if (this->search.ends_with("&")) {
      this->search = this->search.substr(0, this->search.size() - 1);
    }

    return *this;
  }

  URL::Builder& URL::Builder::setDecodeURIComponents (bool enabled) {
    this->decodeURIComponents = enabled;
    return *this;
  }

  URL URL::Builder::build () const {
    StringStream stream;

    if (this->protocol.size() == 0) {
      return String("");
    }

    stream << this->protocol << ":";

    if (this->hostname.size() > 0) {
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
      } else {
        stream << this->hostname;
      }
    }

    if (this->hostname.size() > 0 && this->pathname.size() > 0) {
      if (!this->pathname.starts_with("/")) {
        stream << "/";
      }
    } else {
      stream << "/";
    }

    stream << this->pathname << this->search << this->hash;

    return stream.str();
  }
}
