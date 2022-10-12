#include "../core/core.hh"
#include "ipc.hh"

namespace SSC::IPC {
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
    this->data = JSON::Any(nullptr);
    this->err = JSON::Any(nullptr);
  }

  Result::Result (const Message& message, const String& source)
    : Result() {
    this->message = message;
    this->source = source;
  }

  String Result::str () const {
    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"data", this->data},
      {"err", this->err}
    };

    return JSON::Object(entries).str();
  }
}
