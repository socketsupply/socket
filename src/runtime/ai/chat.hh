#ifndef SOCKET_RUNTIME_AI_CHAT_H
#define SOCKET_RUNTIME_AI_CHAT_H

#include "../json.hh"
#include "../bytes.hh"
#include "../crypto.hh"
#include "../concurrent.hh"
#include "llm.hh"

namespace ssc::runtime::ai::chat {
  using ID = uint64_t;

  struct Message {
    String role = "";
    bytes::Buffer content;
    ID id = crypto::rand64();

    inline const llama_chat_message data () const {
      return {
        this->role.c_str(),
        reinterpret_cast<const char*>(this->content.data())
      };
    }

    inline JSON::Object json () const {
      return JSON::Object::Entries {
        {"id", this->id},
        {"role", this->role},
        {"content", this->content.str()},
      };
    }
  };

  class Session {
    public:
      struct Options {
        ID id = crypto::rand64();
        Vector<String> antiprompts = {
          "<|im_end|>",
          "<|eot_id|>",
          "<step>",
          "EOT",
          "</s>"
        };
      };

      struct GenerateOptions {
        concurrent::AbortSignal signal;
        Vector<String> antiprompts;
      };

      struct ChatOptions : public GenerateOptions {
        Vector<String> antiprompts;
      };

      using GenerateStreamCallback = Function<void(const bytes::Buffer, bool)>;
      using ChatStreamCallback = Function<void(ID, const bytes::Buffer, bool)>;

      ID id;
      Vector<Message> messages;
      Vector<String> antiprompts;
      Vector<llama_chat_message> history;
      SharedPointer<llm::Context> context = nullptr;
      Mutex mutex;

      Vector<char> tokens;
      Atomic<size_t> tokenCount = 0;

      Session (SharedPointer<llm::Context>, const Options&);
      ~Session();
      Session (const Session&) = delete;
      Session (Session&&) = delete;
      Session& operator = (const Session&) = delete;
      Session& operator = (Session&&) = delete;

      bool generate (
        const String&,
        const GenerateOptions&,
        const GenerateStreamCallback& = nullptr
      );

      bool chat (
        const String&,
        const ChatOptions&,
        const ChatStreamCallback& = nullptr
      );

      size_t size () const;
      JSON::Object json () const;
  };
}
#endif
