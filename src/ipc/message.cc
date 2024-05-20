#include "message.hh"

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
    this->isHTTP = message.isHTTP;
    this->cancel = message.cancel;
  }

  Message::Message (const String& source)
    : Message(source, false)
  {}

  Message::Message (const String& source, bool decodeValues) {
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
          this->index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          debug("SSC:IPC::Message: Warning: received non-integer index");
        }
      }

      if (pair[0].compare("value") == 0) {
        this->value = decodeURIComponent(pair[1]);
      }

      if (pair[0].compare("seq") == 0) {
        this->seq = decodeURIComponent(pair[1]);
      }

      if (decodeValues) {
        this->args[pair[0]] = decodeURIComponent(pair[1]);
      } else {
        this->args[pair[0]] = pair[1];
      }
    }
  }

  bool Message::has (const String& key) const {
    return (
      this->args.find(key) != this->args.end() &&
      this->args.at(key).size() > 0
    );
  }

  String Message::get (const String& key) const {
    return this->get(key, "");
  }

  String Message::get (const String& key, const String &fallback) const {
    if (key == "value") return this->value;
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }
}
