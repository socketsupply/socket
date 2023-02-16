#include "../core/core.hh"
#include "ipc.hh"

namespace SSC::IPC {
  Message::Message (const Message& message) {
    this->buffer.bytes = message.buffer.bytes;
    this->buffer.size = message.buffer.size;
    this->value = message.value;
    this->index = message.index;
    this->name = message.name;
    this->seq = message.seq;
    this->uri = message.uri;
    this->args = message.args;
  }

  Message::Message (const String& source, char *bytes, size_t size)
    : Message(source)
  {
    this->buffer.bytes = bytes;
    this->buffer.size = size;
  }

  Message::Message (const String& source) {
    String str = source;
    uri = str;

    // bail if missing protocol prefix
    if (str.find("ipc://") == -1) return;

    // bail if malformed
    if (str.compare("ipc://") == 0) return;
    if (str.compare("ipc://?") == 0) return;

    String query;
    String path;

    auto raw = split(str, '?');
    path = raw[0];
    if (raw.size() > 1) query = raw[1];

    auto parts = split(path, '/');
    if (parts.size() >= 1) name = parts[1];

    if (raw.size() != 2) return;
    auto pairs = split(raw[1], '&');

    for (auto& rawPair : pairs) {
      auto pair = split(rawPair, '=');
      if (pair.size() <= 1) continue;

      if (pair[0].compare("index") == 0) {
        try {
          index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          std::cout << "Warning: received non-integer index" << std::endl;
        }
      }

      if (pair[0].compare("value") == 0) {
        value = decodeURIComponent(pair[1]);
      }

      if (pair[0].compare("seq") == 0) {
        seq = decodeURIComponent(pair[1]);
      }

      args[pair[0]] = pair[1];
    }
  }

  bool Message::has (const String& key) const {
    return this->args.find(key) != this->args.end();
  }

  String Message::get (const String& key) const {
    return this->get(key, "");
  }

  String Message::get (const String& key, const String &fallback) const {
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message
  ) {
    this->message = message;
    this->source = message.name;
    this->seq = seq;
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value
  ) : Result(seq, message, value, Post{}) {
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value,
    Post post
  ) : Result(seq, message) {
    this->post = post;

    if (value.type != JSON::Type::Any) {
      this->value = value;
    }
  }

  Result::Result (const Err error) {
    this->err = error.value;
    this->source = error.message.name;
  }

  Result::Result (const Data data) {
    this->data = data.value;
    this->source = data.message.name;
  }

  JSON::Any Result::json () const {
    if (!this->value.isNull()) {
      if (this->value.isObject()) {
        auto object = this->value.as<JSON::Object>();
        object["source"] = this->source;
        return object;
      }

      return this->value;
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source}
    };

    if (!this->err.isNull()) {
      entries["err"] = this->err;
    } else {
      entries["data"] = this->data;
    }

    return JSON::Object(entries);
  }

  String Result::str () const {
    auto json = this->json();
    return json.str();
  }

  Result::Err::Err (
    const Message& message,
    JSON::Any value
  ) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
  }

  Result::Data::Data (
    const Message& message,
    JSON::Any value
  ) : Data(message, value, Post{}) {
  }

  Result::Data::Data (
    const Message& message,
    JSON::Any value,
    Post post
  ) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
    this->post = post;
  }
}
