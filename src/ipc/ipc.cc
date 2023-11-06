#include "../core/core.hh"
#include "ipc.hh"
namespace SSC {
  #if defined(_WIN32)
  SSC::String FormatError(DWORD error, SSC::String source) {
    SSC::StringStream message;
    LPVOID lpMsgBuf;
    FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER |
    FORMAT_MESSAGE_FROM_SYSTEM |
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    error,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    (LPTSTR) &lpMsgBuf,
    0, NULL );

    message << "Error " << error << " in " << source << ": " <<  (LPTSTR)lpMsgBuf;
    LocalFree(lpMsgBuf);

    return message.str();
  }
  #endif
}
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

  Message::Message (const String& source, char *bytes, size_t size)
    : Message(source, false, bytes, size)
  {}

  Message::Message (
    const String& source,
    bool decodeValues,
    char *bytes,
    size_t size
  ) : Message(source, decodeValues)
  {
    this->buffer.bytes = bytes;
    this->buffer.size = size;
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
          index = std::stoi(pair[1].size() > 0 ? pair[1] : "0");
        } catch (...) {
          debug("Warning: received non-integer index");
        }
      }

      if (pair[0].compare("value") == 0) {
        value = decodeURIComponent(pair[1]);
      }

      if (pair[0].compare("seq") == 0) {
        seq = decodeURIComponent(pair[1]);
      }

      if (decodeValues) {
        args[pair[0]] = decodeURIComponent(pair[1]);
      } else {
        args[pair[0]] = pair[1];
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
    return args.count(key) ? decodeURIComponent(args.at(key)) : fallback;
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message
  ) {
    this->id = rand64();
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
    this->headers = Headers(post.headers);

    if (value.type != JSON::Type::Any) {
      this->value = value;
    }
  }

  Result::Result (const JSON::Any value) {
    this->id = rand64();
    this->value = value;
  }

  Result::Result (const Err error): Result(error.message.seq, error.message) {
    this->err = error.value;
    this->source = error.message.name;
  }

  Result::Result (const Data data): Result(data.message.seq, data.message) {
    this->data = data.value;
    this->post = data.post;
    this->source = data.message.name;
    this->headers = Headers(data.post.headers);
  }

  JSON::Any Result::json () const {
    if (!this->value.isNull()) {
      if (this->value.isObject()) {
        auto object = this->value.as<JSON::Object>();

        if (object.has("data") || object.has("err")) {
          object["source"] = this->source;
          object["id"] = std::to_string(this->id);
        }

        return object;
      }

      return this->value;
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"id", std::to_string(this->id)}
    };

    if (!this->err.isNull()) {
      entries["err"] = this->err;
      if (this->err.isObject()) {
        if (this->err.as<JSON::Object>().has("id")) {
          entries["id"] = this->err.as<JSON::Object>().get("id");
        }
      }
    } else if (!this->data.isNull()) {
      entries["data"] = this->data;
      if (this->data.isObject()) {
        if (this->data.as<JSON::Object>().has("id")) {
          entries["id"] = this->data.as<JSON::Object>().get("id");
        }
      }
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
