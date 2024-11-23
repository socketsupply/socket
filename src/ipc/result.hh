#ifndef SOCKET_RUNTIME_IPC_RESULT_H
#define SOCKET_RUNTIME_IPC_RESULT_H

#include "../core/core.hh"
#include "message.hh"

namespace SSC::IPC {
  class Result {
    public:
      class Err {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Err () = default;
          Err (const Message&, const char*);
          Err (const Message&, const String&);
          Err (const Message&, JSON::Any);
      };

      class Data {
        public:
          Message message;
          Message::Seq seq;
          JSON::Any value;
          Post post;

          Data () = default;
          Data (const Message&, JSON::Any);
          Data (const Message&, JSON::Any, Post);
      };

      Message message;
      Message::Seq seq = "-1";
      uint64_t id = 0;
      String source = "";
      String token = "";
      JSON::Any value = nullptr;
      JSON::Any data = nullptr;
      JSON::Any err = nullptr;
      Headers headers;
      Post post;

      Result () = default;
      Result (const Result&) = default;
      Result (const JSON::Any, const String& token = "");
      Result (const Err error);
      Result (const Data data);
      Result (const Message::Seq&, const Message&);
      Result (const Message::Seq&, const Message&, JSON::Any);
      Result (const Message::Seq&, const Message&, JSON::Any, Post);
      String str () const;
      JSON::Any json () const;
  };
}
#endif
