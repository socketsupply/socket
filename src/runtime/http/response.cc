#include "../http.hh"

namespace ssc::runtime::http {
  Response::Response (const String& input)
    : body(input)
  {
    this->setHeader("content-length", this->body.size());
  }

  Response::Response (const Status& status, const String& input)
    : status(status),
      body(input)
  {}

  Response::Response (const Status& status, const JSON::Any& json)
    : status(status),
      body(json.str())
  {
    this->setHeader("content-type", "application/json");
  }

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

  Response& Response::setHeader (const String& key, const Headers::Value& value) {
    this->headers.set(key, value);
    return *this;
  }

  const unsigned char* Response::data () const {
    return this->body.data();
  }

  size_t Response::size () const {
    return this->body.size();
  }

  String Response::str () const {
    return this->status.str() + "\r\n" + this->headers.str();
  }
}
