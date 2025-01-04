#include "../bytes.hh"
#include "../url.hh"

#include "../ipc.hh"

using ssc::runtime::url::decodeURIComponent;

namespace ssc::runtime::ipc {
  Message::Message (const Message& message)
    : value(message.value),
      index(message.index),
      name(message.name),
      seq(message.seq),
      uri(message.uri),
      isHTTP(message.isHTTP),
      cancel(message.cancel),
      buffer(message.buffer),
      client(message.client)
  {}

  Message::Message (const String& source)
    : Message(source, false)
  {}

  Message::Message (const String& source, bool decodeValues)
    : uri(source, decodeValues)
  {
    this->seq = this->get("seq");
    this->name = this->uri.hostname;
    this->value = this->get("value");

    if (this->uri.searchParams.contains("index")) {
      try {
        this->index = this->uri.searchParams
          .get("index")
          .as<JSON::Number>()
          .value();
      } catch (const Exception& e) {
        debug(
          "ssc::runtime::ipc::Message: Warning: received non-integer index: %s: %s",
          this->uri.str().c_str(),
          e.what()
        );
      }
    }
  }

  Message::Message (Message&& msg) {
    this->buffer = std::move(msg.buffer);
    this->client = std::move(msg.client);
    this->index = msg.index;
    this->value = std::move(msg.value);
    this->name = std::move(msg.name);
    this->uri = std::move(msg.uri);
    this->seq = std::move(msg.seq);
    this->isHTTP = msg.isHTTP;
    this->cancel = std::move(msg.cancel);

    msg.name = "";
    msg.index = -1;
    msg.value = "";
    msg.uri = URL();
    msg.seq = "";
    msg.isHTTP = false;
    msg.buffer.reset();
    msg.cancel = nullptr;
  }

  Message& Message::operator = (const Message& msg) {
    this->buffer = msg.buffer;
    this->client = msg.client;
    this->index = msg.index;
    this->value = msg.value;
    this->name = msg.name;
    this->uri = msg.uri;
    this->seq = msg.seq;
    this->isHTTP = msg.isHTTP;
    this->cancel = std::move(msg.cancel);
    return *this;
  }

  Message& Message::operator = (Message&& msg) {
    this->buffer = std::move(msg.buffer);
    this->client = std::move(msg.client);
    this->index = msg.index;
    this->value = std::move(msg.value);
    this->name = std::move(msg.name);
    this->uri = std::move(msg.uri);
    this->seq = std::move(msg.seq);
    this->isHTTP = msg.isHTTP;
    this->cancel = std::move(msg.cancel);

    msg.name = "";
    msg.index = -1;
    msg.value = "";
    msg.uri = URL();
    msg.seq = "";
    msg.isHTTP = false;
    msg.buffer.reset();
    msg.cancel = nullptr;
    return *this;
  }

  bool Message::has (const String& key) const {
    return this->uri.searchParams.contains(key);
  }

  bool Message::contains (const String& key) const {
    return this->uri.searchParams.contains(key);
  }

  const String& Message::at (const String& key) const {
    return this->uri.searchParams.at(key).data;
  }

  const String Message::get (const String& key) const {
    return this->get(key, "");
  }

  const String Message::get (const String& key, const String &fallback) const {
    if (key == "value" && this->value.size() > 0) {
      return this->value;
    }

    return this->contains(key)
      ? decodeURIComponent(this->uri.searchParams.get(key).str())
      : fallback;
  }

  const Map<String, String> Message::dump () const {
    return this->map();
  }

  const String Message::str () const {
    return this->uri.str();
  }

  const Map<String, String> Message::map () const {
    return this->uri.searchParams.map();
  }

  const JSON::Object Message::json () const {
    return JSON::Object::Entries {
      {"name", this->name},
      {"value", this->value},
      {"index", this->index},
      {"seq", this->seq},
      {"data", this->map()}
    };
  }
}
