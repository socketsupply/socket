#include "../core/core.hh"
#include "ipc.hh"

namespace SSC::IPC {
  Message::Message (const Message& message) {
    this->body.bytes = message.body.bytes;
    this->body.size = message.body.size;
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
    this->body.bytes = bytes;
    this->body.size = size;
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

  String Message::get (const String& key) const {
    return this->get(key, "");
  }

  String Message::get (const String& key, const String &fallback) const {
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }

  Result::Result () {
  }

  Result::Result (Message::Seq seq, const Message& message) {
    this->message = message;
    this->source = message.name;
    this->seq = seq;
  }

  Result::Result (Message::Seq seq, const Message& message, JSON::Any json)
    : Result(seq, message, json, Post{}) { }

  Result::Result (Message::Seq seq, const Message& message, JSON::Any json, Post post)
    : Result(seq, message)
  {
    this->post = post;

    if (json.type != JSON::Type::Any) {
      this->json = json;
    }
  }

  String Result::str () const {
    if (this->json.type != JSON::Type::Null) {
      return this->json.str();
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"data", this->data},
      {"err", this->err}
    };

    return JSON::Object(entries).str();
  }
}
