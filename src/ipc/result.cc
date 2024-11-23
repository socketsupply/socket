#include "result.hh"

namespace SSC::IPC {
  Result::Result (const Message::Seq& seq, const Message& message) {
    this->id = rand64();
    this->seq = seq;
    this->message = message;
    this->source = message.name;
    this->token = message.get("ipc-token", "");
    this->post.workerId = this->message.get("runtime-worker-id");
  }

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value
  ) : Result(seq, message, value, Post{})
  {}

  Result::Result (
    const Message::Seq& seq,
    const Message& message,
    JSON::Any value,
    Post post
  ) : Result(seq, message) {
    this->post = post;
    this->headers = Headers(post.headers);

    if (this->post.workerId.size() == 0) {
      this->post.workerId = this->message.get("runtime-worker-id");
    }

    if (value.type != JSON::Type::Any) {
      this->value = value;
    }
  }

  Result::Result (const JSON::Any value, const String& token) {
    this->id = rand64();
    this->value = value;
    this->token = token;
  }

  Result::Result (const Err error): Result(error.message.seq, error.message) {
    this->err = error.value;
    this->token = error.message.get("ipc-token", "");
    this->source = error.message.name;
  }

  Result::Result (const Data data): Result(data.message.seq, data.message) {
    this->data = data.value;
    this->post = data.post;
    this->token = data.message.get("ipc-token", "");
    this->source = data.message.name;
    this->headers = Headers(data.post.headers);
  }

  JSON::Any Result::json () const {
    if (!this->value.isNull()) {
      if (this->value.isObject()) {
        auto object = this->value.as<JSON::Object>();

        if (object.has("data") || object.has("err")) {
          object["source"] = this->source;
          object["token"] = this->token.size() > 0
            ? JSON::Any(JSON::String(this->token))
            : JSON::Any(JSON::Null());
          object["id"] = std::to_string(this->id);
        }

        return object;
      }

      return this->value;
    }

    auto entries = JSON::Object::Entries {
      {"source", this->source},
      {"token", this->token.size() > 0
        ? JSON::Any(this->token)
        : JSON::Any(JSON::Null())
      },
      {"id", std::to_string(this->id)}
    };

    if (!this->err.isNull()) {
      entries["err"] = this->err;
      if (this->err.isObject()) {
        if (this->err.as<JSON::Object>().has("id")) {
          entries["id"] = this->err.as<JSON::Object>().get("id");
        }

        if (this->err.as<JSON::Object>().has("token")) {
          entries["token"] = this->err.as<JSON::Object>().get("token");
        }

        if (this->err.as<JSON::Object>().has("source")) {
          entries["source"] = this->err.as<JSON::Object>().get("source");
        }
      }
    } else if (!this->data.isNull()) {
      entries["data"] = this->data;
      if (this->data.isObject()) {
        if (this->data.as<JSON::Object>().has("id")) {
          entries["id"] = this->data.as<JSON::Object>().get("id");
        }

        if (this->data.as<JSON::Object>().has("token")) {
          entries["token"] = this->data.as<JSON::Object>().get("token");
        }

        if (this->data.as<JSON::Object>().has("source")) {
          entries["source"] = this->data.as<JSON::Object>().get("source");
        }
      }
    }

    return JSON::Object(entries);
  }

  String Result::str () const {
    auto json = this->json();
    return json.str();
  }

  Result::Err::Err (const Message& message, JSON::Any value) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
  }

  Result::Err::Err (const Message& message, const char* error)
    : Err(message, String(error))
  {}

  Result::Err::Err (const Message& message, const String& error) {
    this->seq = message.seq;
    this->message = message;
    this->value = JSON::Object::Entries {{"message", error}};
  }

  Result::Data::Data (const Message& message, JSON::Any value)
    : Data(message, value, Post{})
  {}

  Result::Data::Data (const Message& message, JSON::Any value, Post post) {
    this->seq = message.seq;
    this->message = message;
    this->value = value;
    this->post = post;
  }
}
