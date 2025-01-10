#include "../bridge.hh"
#include "../crypto.hh"
#include "../string.hh"
#include "../ipc.hh"

using ssc::runtime::string::toLowerCase;
using ssc::runtime::crypto::rand64;

namespace ssc::runtime::ipc {
  Router::Router (bridge::Bridge& bridge)
    : dispatcher(bridge.dispatcher),
      bridge(bridge)
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
    const MessageCallback callback
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

  void Router::map (const String& name, const MessageCallback callback) {
    return this->map(name, true, std::move(callback));
  }

  void Router::map (
    const String& name,
    bool async,
    const MessageCallback callback
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
    SharedPointer<unsigned char[]> bytes,
    size_t size
  ) {
    return this->invoke(uri, bytes, size, [this](auto result) {
      this->dispatcher.dispatch([this, result] () {
        this->bridge.send(result.seq, result.str(), result.queuedResponse);
      });
    });
  }

  bool Router::invoke (const String& uri, const ResultCallback callback) {
    return this->invoke(uri, nullptr, 0, callback);
  }

  bool Router::invoke (
    const String& uri,
    SharedPointer<unsigned char[]> bytes,
    size_t size,
    const ResultCallback callback
  ) {
    if (!this->bridge.active()) {
      return false;
    }

    const auto message = Message(uri, true);
    return this->invoke(std::move(message), bytes, size, std::move(callback));
  }

  bool Router::invoke (
    const Message& message,
    SharedPointer<unsigned char[]> bytes,
    size_t size,
    const ResultCallback callback
  ) {
    if (!this->bridge.active()) {
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

    auto incomingMessage = Message(message);

    if (bytes != nullptr && size > 0) {
      incomingMessage.buffer = bytes::ArrayBuffer(size, bytes);
    }

    // named listeners
    if (this->listeners.contains(name)) {
      const auto& listeners = this->listeners[name];
      for (const auto& listener : listeners) {
        listener.callback(incomingMessage, this, [](const auto& _) {});
      }
    }

    // wild card (*) listeners
    if (this->listeners.contains("*")) {
      const auto& listeners = this->listeners["*"];
      for (const auto& listener : listeners) {
        listener.callback(incomingMessage, this, [](const auto& _) {});
      }
    }

    if (context.async) {
      return this->dispatcher.dispatch([
        this,
        context = std::move(context),
        callback = std::move(callback),
        incomingMessage = std::move(incomingMessage)
      ]() mutable {
        context.callback(incomingMessage, this, [this, incomingMessage, callback](const auto result) mutable {
          if (result.seq == "-1") {
            this->bridge.send(result.seq, result.str(), result.queuedResponse);
          } else {
            callback(result);
          }
        });
      });
    }

    context.callback(incomingMessage, this, [
      this,
      callback = std::move(callback)
    ](const auto result) mutable {
      if (result.seq == "-1") {
        this->bridge.send(result.seq, result.str(), result.queuedResponse);
      } else {
        callback(result);
      }
    });

    return true;
  }
}
