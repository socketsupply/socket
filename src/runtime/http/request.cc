#include "../string.hh"
#include "../http.hh"

using ssc::runtime::string::split;

namespace ssc::runtime::http {
  Request::Request (const String& input) {
    const auto crlf = input.find("\r\n");
    if (crlf != String::npos) {
      auto stream = std::istringstream(input.substr(0, crlf));
      String pathname;
      String version;
      stream
        >> this->method
        >> pathname
        >> version;

      const auto versionParts = split(version, '/');
      const auto pathParts = split(pathname, '?');

      if (versionParts.size() == 2) {
        this->version = versionParts[1];
      }

      if (pathParts.size() == 2) {
        this->url.pathname = pathParts[0];
        this->url.search = "?" + pathParts[1];
        this->url.searchParams.set(pathParts[1]);
      } else {
        this->url.pathname = pathname;
      }

      this->headers = input.substr(crlf, input.find("\r\n\r\n"));
      this->body = input.substr(input.find("\r\n\r\n"));

      const auto host = this->headers.get("host");
      if (!host.empty()) {
        const auto parts = split(host.value.str(), ':');
        if (parts.size() == 2) {
          this->url.hostname = parts[0];
          this->url.port = parts[0];
        } else {
          this->url.hostname = host.value.str();
        }
      }
    }
  }

  Request::Request (const unsigned char* input, size_t size) {
    const auto string = size >= 0
      ? String(reinterpret_cast<const char*>(input), size)
      : String(reinterpret_cast<const char*>(input));

    const auto crlf = string.find("\r\n");

    if (crlf != String::npos) {
      auto stream = std::istringstream(string.substr(0, crlf));
      String pathname;
      String version;
      stream
        >> this->method
        >> pathname
        >> version;

      const auto versionParts = split(version, '/');
      const auto pathParts = split(pathname, '?');

      if (versionParts.size() == 2) {
        this->version = versionParts[1];
      }

      if (pathParts.size() == 2) {
        this->url.pathname = pathParts[0];
        this->url.search = "?" + pathParts[1];
        this->url.searchParams.set(pathParts[1]);
      } else {
        this->url.pathname = pathname;
      }

      this->headers = string.substr(crlf, string.find("\r\n\r\n"));
      this->body.set(
        input + string.find("\r\n\r\n") + 4,
        0,
        size - string.find("\r\n\r\n") - 4
      );

      const auto host = this->headers.get("host");
      if (!host.empty()) {
        const auto parts = split(host.value.str(), ':');
        if (parts.size() == 2) {
          this->url.hostname = parts[0];
          this->url.port = parts[0];
        } else {
          this->url.hostname = host.value.str();
        }
      }
    }
  }

  bool Request::valid () const {
    return (
      this->method.size() > 0 &&
      this->version.size() > 0 &&
      this->url.size() > 0
    );
  }

  String Request::str () const {
    return this->method + " " + this->url.pathname + this->url.search + " HTTP/" + this->version + "\r\n" + this->headers.str();
  }
}
