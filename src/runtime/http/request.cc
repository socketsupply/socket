#include "../http.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::http {
  Request::Request (const String& input) {
    const auto crlf = input.find("\r\n");
    if (crlf != String::npos) {
      auto stream = std::istringstream(input.substr(0, crlf));
      String uri;
      stream
        >> this->method
        >> uri
        >> this->version;

      this->url = uri;
      this->headers = input.substr(crlf, input.find("\r\n\r\n"));
      this->body = input.substr(input.find("\r\n\r\n"));
    }
  }

  Request::Request (const unsigned char* input, size_t size) {
    const auto string = size >= 0
      ? String(reinterpret_cast<const char*>(input), size)
      : String(reinterpret_cast<const char*>(input));

    const auto crlf = string.find("\r\n");

    if (crlf != String::npos) {
      auto stream = std::istringstream(string.substr(0, crlf));
      String uri;
      stream
        >> this->method
        >> uri
        >> this->version;

      this->url = uri;
      this->headers = string.substr(crlf, string.find("\r\n\r\n"));
      this->body.set(
        input + string.find("\r\n\r\n") + 4,
        0,
        size - string.find("\r\n\r\n") - 4
      );
    }
  }

  bool Request::valid () const {
    return (
      this->method.size() > 0 &&
      this->version.size() > 0 &&
      this->url.href.size() > 0
    );
  }

  String Request::str () const {
    return this->method + " " + this->url.pathname + this->url.search + " HTTP/" + this->version + "\r\n" + this->headers.str();
  }
}
