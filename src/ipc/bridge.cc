#include "../core/core.hh"
#include "ipc.hh"

namespace SSC::IPC {
  Bridge::Bridge (Core *core) : router(core) {
    this->core = core;
  }

  Router::Router (Core *core) {
    this->core = core;
    this->bind("ping", [](auto message, auto router) {
      auto result = JSON::Object(JSON::Object::Entries {
        {"source", "ping"},
        {"data", JSON::Object::Entries {
          {"value", JSON::Array::Entries {"pong", "pong", "pong"}}
        }}
      });

      router->reply(message, result);
    });

    this->bind("event", [](auto message, auto router) {
      auto value = message.value;
      auto data = message.get("data");
      auto seq = message.seq;
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

  bool Router::invoke (
    const String& name,
    const Message& message
  ) {
    auto callback = table.at(name);

    if (callback != nullptr) {
      return this->dispatch([this, callback, message] {
        callback(message, this);
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
    for (auto const &tuple : table) {
      auto name = tuple.first;

      if (name == message.name && invoke(name, message)) {
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
