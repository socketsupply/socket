#include "../core/core.hh"
#include "ipc.hh"
#include "json.hh"

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router(core) {
    this->core = core;
  }

  Router::Router (Core *core) {
    this->core = core;
    this->bind("ping", [](auto message, auto router, auto reply) {
      auto result = Result { message, "ping" };
      result.data = JSON::Object::Entries {
        {"value", JSON::Array::Entries {"pong", true, 123.456}}
      };

      reply(result);
    });

    this->bind("event", [](auto message, auto router, auto reply) {
      auto value = message.value;
      auto data = message.get("data");
      auto seq = message.seq;
      fprintf(stderr, "IN EVENT\n");
      router->core->handleEvent(seq, value, data, [=](auto seq, auto result, auto post) {
        router->reply(message, result, post);
      });
    });
  }

  void Router::bind (
    const String& name,
    MessageCallback callback
  ) {
    table.insert_or_assign(name, callback);
  }

  void Router::unbind (const String& name) {
    if (table.find(name) != table.end()) {
      table.erase(name);
    }
  }

  bool Router::invoke (const Message& message) {
    return this->invoke(message, [this](auto result) {
      this->reply(result);
    });
  }

  bool Router::invoke (const Message& message, ResultCallback callback) {
    if (this->table.find(message.name) == this->table.end()) {
      return false;
    }

    auto fn = this->table.at(message.name);

    if (fn!= nullptr) {
      return this->dispatch([this, fn, message, callback] {
        fn(message, this, callback);
      });
    }

    return false;
  }

  bool Router::route (const String source) {
    const Message message(source);
    return this->route(message);
  }

  bool Router::route (const String source, char *bytes, size_t size) {
    const Message message(source, bytes, size);
    return this->route(message);
  }

  bool Router::route (const Message& message) {
    return this->route(message, [this](auto result) {
      this->reply(result);
    });
  }

  bool Router::route (const Message& message, ResultCallback callback) {
    fprintf(stderr, "route=%s", message.uri.c_str());
    for (auto const &tuple : table) {
      auto name = tuple.first;

      if (name == message.name && this->invoke(message, callback)) {
        return true;
      }
    }

    return false;
  }

  bool Router::push (const JSON::Any data) {
    return this->push(data.str());
  }

  bool Router::push (const String& data) {
    return this->push(data, Post{});
  }

  bool Router::push (const Post &post) {
    return this->push(String(""), post);
  }

  bool Router::push (const JSON::Any data, const Post &post) {
    return this->push(data.str(), post);
  }

  bool Router::push (const String& data, const Post &post) {
    return this->send("-1", data, post);
  }

  bool Router::reply (const Message& message, const JSON::Any data) {
    return this->reply(message, data.str());
  }

  bool Router::reply (const Message& message, const JSON::Any data, const Post& post) {
    return this->reply(message, data.str(), post);
  }

  bool Router::reply (const Message& message, const String& data) {
    return this->reply(message, data, Post{});
  }

  bool Router::reply (const Result& result) {
    return this->reply(result.message, result.str(), result.post);
  }

  bool Router::reply (
    const Message& message,
    const String& data,
    const Post& post
  ) {
    return this->send(message.seq, data, post);
  }

  bool Router::send (const Message::Seq& seq, const JSON::Any data) {
    return this->send(seq, data.str(), Post{});
  }

  bool Router::send (const Message::Seq& seq, const JSON::Any data, const Post& post) {
    return this->send(seq, data.str(), post);
  }

  bool Router::send (
    const Message::Seq& seq,
    const String& data
  ) {
    return this->send(seq, data, Post{});
  }

  bool Router::send (
    const Message::Seq& seq,
    const String& data,
    const Post& post
  ) {
    if (post.body || seq == "-1") {
      auto script = this->core->createPost(seq, data, post);
      this->evaluateJavaScript(script);
      return true;
    }

    // this had a sequence, we need to try to resolve it.
    if (seq != "-1" && seq.size() > 0) {
      auto value = SSC::encodeURIComponent(data);
      auto script = SSC::getResolveToRenderProcessJavaScript(seq, "0", value);
      this->evaluateJavaScript(script);
      return true;
    }

    if (data.size() > 0) {
      this->evaluateJavaScript(data);
      return true;
    }

    return false;
  }

  bool Router::emit (
    const String& name,
    const String& data
  ) {
    auto value = SSC::encodeURIComponent(data);
    auto script = SSC::getEmitToRenderProcessJavaScript(name, value);
    return this->evaluateJavaScript(script);
  }

  bool Router::evaluateJavaScript (const String js) {
    if (this->implementation.evaluateJavaScript != nullptr) {
      this->implementation.evaluateJavaScript(js);
      return true;
    }

    return false;
  }

  bool Router::dispatch (DispatchCallback callback) {
    if (this->implementation.dispatch != nullptr) {
      this->implementation.dispatch(callback);
      return true;
    }

    return false;
  }

  void Router::setImplementation (Implementation implementation) {
    this->implementation = implementation;
  }
}
