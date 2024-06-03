#include "bridge.hh"
#include "router.hh"
#include "../core/trace.hh"

namespace SSC::IPC {
  Router::Router (Bridge* bridge)
    : bridge(bridge)
  {}

  void Router::init () {
    this->mapRoutes();
    this->preserveCurrentTable();
  }

  void Router::preserveCurrentTable () {
    this->preserved = this->table;
  }

  uint64_t Router::listen (
    const String& name,
    const MessageCallback& callback
  ) {
    const auto key = toLowerCase(name);

    if (!this->listeners.contains(key)) {
      this->listeners[key] = Vector<MessageCallbackListenerContext>();
    }

    auto& listeners = this->listeners.at(key);
    const auto token = rand64();
    listeners.push_back(MessageCallbackListenerContext { token , callback });
    return token;
  }

  bool Router::unlisten (const String& name, uint64_t token) {
    const auto key = toLowerCase(name);
    if (!this->listeners.contains(key)) {
      return false;
    }

    auto& listeners = this->listeners.at(key);
    for (int i = 0; i < listeners.size(); ++i) {
      const auto& listener = listeners[i];
      if (listener.token == token) {
        listeners.erase(listeners.begin() + i);
        return true;
      }
    }

    return false;
  }

  void Router::map (const String& name, const MessageCallback& callback) {
    return this->map(name, true, callback);
  }

  void Router::map (
    const String& name,
    bool async,
    const MessageCallback& callback
  ) {
    if (callback != nullptr) {
      const auto key = toLowerCase(name);
      this->table.insert_or_assign(key, MessageCallbackContext {
        async,
        callback
      });
    }
  }

  void Router::unmap (const String& name) {
    this->table.erase(toLowerCase(name));
  }

  bool Router::invoke (
    const String& uri,
    SharedPointer<char[]> bytes,
    size_t size
  ) {
    return this->invoke(uri, bytes, size, [this](auto result) {
      this->bridge->dispatch([=, this] () {
        this->bridge->send(result.seq, result.str(), result.post);
      });
    });
  }

  bool Router::invoke (const String& uri, const ResultCallback& callback) {
    return this->invoke(uri, nullptr, 0, callback);
  }

  bool Router::invoke (
    const String& uri,
    SharedPointer<char[]> bytes,
    size_t size,
    const ResultCallback& callback
  ) {
    if (this->bridge->core->shuttingDown) {
      return false;
    }

    const auto message = Message(uri, true);
    return this->invoke(message, bytes, size, callback);
  }

  bool Router::invoke (
    const Message& message,
    SharedPointer<char[]> bytes,
    size_t size,
    const ResultCallback& callback
  ) {
    if (this->bridge->core->shuttingDown) {
      return false;
    }

    const auto name = toLowerCase(message.name);
    MessageCallbackContext context;

    // lookup router function in the preserved table,
    // then the public table, return if unable to determine a context
    if (this->preserved.contains(name)) {
      context = this->preserved.at(name);
    } else if (this->table.contains(name)) {
      context = this->table.at(name);
    } else {
      return false;
    }

    if (context.callback == nullptr) {
      return false;
    }

    Message msg(message);

    if (bytes != nullptr && size > 0) {
      msg.buffer.bytes = bytes;
      msg.buffer.size = size;
    }

    // named listeners
    if (this->listeners.contains(name)) {
      const auto listeners = this->listeners[name];
      for (const auto& listener : listeners) {
        listener.callback(msg, this, [](const auto& _) {});
      }
    }

    // wild card (*) listeners
    if (this->listeners.contains("*")) {
      const auto listeners = this->listeners["*"];
      for (const auto& listener : listeners) {
        listener.callback(msg, this, [](const auto& _) {});
      }
    }

    if (context.async) {
      return this->bridge->dispatch([=, this]() mutable {
        context.callback(msg, this, [=, this](const auto result) mutable {
          if (result.seq == "-1") {
            this->bridge->send(result.seq, result.str(), result.post);
          } else {
            callback(result);
          }
        });
      });
    }

    context.callback(msg, this, [=, this](const auto result) mutable {
      if (result.seq == "-1") {
        this->bridge->send(result.seq, result.str(), result.post);
      } else {
        callback(result);
      }
    });

    return true;
  }
}
