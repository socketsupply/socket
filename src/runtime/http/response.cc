#include "../http.hh"

namespace ssc::runtime::http {
  Response::Response (const String& input)
    : body(input)
  {}

  Response::Response (const Headers& headers)
    : headers(headers)
  {}

  Response::Response (const Status& status)
    : status(status)
  {}

  Response::Response (const Status& status, const Headers& headers)
    : headers(headers),
      status(status)
  {}

  Response& Response::setHeader (const Headers::Header& header) {
    this->headers.set(header);
    return *this;
  }

  Response& Response::setHeader (const String& key, const String& value) {
    this->headers.set(key, value);
    return *this;
  }

  const unsigned char* Response::data () const {
    return this->body.data();
  }

  String Response::str () const {
    return this->status.str() + "\r\n" + this->headers.str();
  }
}
