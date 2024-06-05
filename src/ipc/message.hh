#ifndef SOCKET_RUNTIME_IPC_MESSAGE_H
#define SOCKET_RUNTIME_IPC_MESSAGE_H

#include "../core/core.hh"
#include "client.hh"

namespace SSC::IPC {
  struct MessageBuffer {
    size_t size = 0;
    SharedPointer<char[]> bytes = nullptr;

    MessageBuffer () = default;
    MessageBuffer (SharedPointer<char[]> bytes, size_t size)
        : size(size),
          bytes(bytes)
    {}

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    ICoreWebView2SharedBuffer* shared_buf = nullptr;
    MessageBuffer (ICoreWebView2SharedBuffer* buf, size_t size)
        : size(size),
          shared_buf(buf)
    {
      BYTE* b = reinterpret_cast<BYTE*>(bytes.get());
      HRESULT r = buf->get_Buffer(&b);
      if (r != S_OK) {
        // TODO(trevnorris): Handle this
      }
    }
  #endif
  };

  struct MessageCancellation {
    void (*handler)(void*) = nullptr;
    void *data = nullptr;
  };

  class Message {
    public:
      using Seq = String;
      MessageBuffer buffer;
      Client client;

      String value = "";
      String name = "";
      String uri = "";
      int index = -1;
      Seq seq = "";
      Map args = {};
      bool isHTTP = false;
      SharedPointer<MessageCancellation> cancel = nullptr;

      Message () = default;
      Message (const Message& message);
      Message (const String& source, bool decodeValues);
      Message (const String& source);
      bool has (const String& key) const;
      String get (const String& key) const;
      String get (const String& key, const String& fallback) const;
      String str () const { return this->uri; }
      const char* c_str () const { return this->uri.c_str(); }
  };
}
#endif
