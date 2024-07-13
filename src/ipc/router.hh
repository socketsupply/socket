#ifndef SOCKET_RUNTIME_IPC_ROUTER_H
#define SOCKET_RUNTIME_IPC_ROUTER_H

#include "../core/core.hh"
#include "message.hh"
#include "result.hh"

namespace SSC::IPC {
  class Bridge;
  class Router {
    public:
      using ReplyCallback = Function<void(const Result&)>;
      using ResultCallback = Function<void(Result)>;
      using MessageCallback = Function<void(const Message&, Router*, ReplyCallback)>;

      struct MessageCallbackContext {
        bool async = true;
        MessageCallback callback;
      };

      struct MessageCallbackListenerContext {
        uint64_t token;
        MessageCallback callback;
      };

      using Table = std::map<String, MessageCallbackContext>;
      using Listeners = std::map<String, std::vector<MessageCallbackListenerContext>>;

    private:
      Table preserved;

    public:
      Listeners listeners;
      Mutex mutex;
      Table table;

      Bridge *bridge = nullptr;

      Router () = default;
      Router (Bridge* bridge);
      Router (const Router&) = delete;
      Router (const Router&&) = delete;
      Router (Router&&) = delete;

      void init ();
      void mapRoutes ();
      void preserveCurrentTable ();
      uint64_t listen (const String& name, const MessageCallback& callback);
      bool unlisten (const String& name, uint64_t token);
      void map (const String& name, const MessageCallback& callback);
      void map (const String& name, bool async, const MessageCallback& callback);
      void unmap (const String& name);
      bool invoke (const String& uri, const ResultCallback& callback);
      bool invoke (const String& uri, SharedPointer<char[]> bytes, size_t size);
      bool invoke (
        const String& uri,
        SharedPointer<char[]> bytes,
        size_t size,
        const ResultCallback& callback
      );

      bool invoke (
        const Message& message,
        SharedPointer<char[]> bytes,
        size_t size,
        const ResultCallback& callback
      );
  };
}
#endif
