#include <llama.h>

#include "../../debug.hh"
#include "../../string.hh"

#include "../llm.hh"
#include "../chat.hh"

using ssc::runtime::string::trim;

namespace ssc::runtime::ai::chat {
  Session::Session (
    SharedPointer<llm::Context> context,
    const Options& options
  ) : context(context),
      antiprompts(options.antiprompts),
      id(options.id)
  {}

  Session::~Session () {}

  bool Session::generate (
    const String& prompt,
    const GenerateOptions& options,
    const GenerateStreamCallback& callback
  ) {
    Lock lock(this->mutex);

    if (this->context == nullptr || this->context->model == nullptr) {
      return false;
    }

    const auto tokenCount = -llama_tokenize(
      this->context->model->vocab,
      prompt.c_str(),
      prompt.size(),
      nullptr,
      0,
      true,
      true
    );

    Vector<llama_token> tokens(tokenCount);
    const auto status = llama_tokenize(
      this->context->model->vocab,
      prompt.c_str(),
      prompt.size(),
      tokens.data(),
      tokens.size(),
      this->context->used() == 0,
      true
    );

    if (status < 0) {
      return false;
    }

    // stop strings or reverse prompts (stop and wait for user input)
    Vector<String> antiprompts = this->antiprompts;
    bool isDetectingAntiprompt = false;
    size_t antipromptMaxSize = 0;
    bytes::BufferQueue generation;
    llama_batch batch;
    llama_token token;

    for (const auto& antiprompt :  options.antiprompts) {
      antiprompts.push_back(antiprompt);
    }

    for (const auto& antiprompt :  antiprompts) {
      if (antiprompt.size() > antipromptMaxSize) {
        antipromptMaxSize = antiprompt.size();
      }
    }

    batch = llama_batch_get_one(tokens.data(), tokens.size());
    llama_sampler_init_penalties(64, 1.00f, 0, 0);

    while (!options.signal.aborted()) {
      if (this->context->used() + batch.n_tokens > this->context->size()) {
        return false;
      }

      if (llama_decode(this->context->context, batch)) {
        debug("llama_decode failed");
        return false;
      }

      token = llama_sampler_sample(
        this->context->sampler,
        this->context->context,
        -1
      );

      if (llama_vocab_is_eog(this->context->model->vocab, token)) {
        callback(bytes::Buffer(0), true);
        return true;
      }

      char piece[256] = {0};
      const auto size = llama_token_to_piece(
        this->context->model->vocab,
        token,
        piece,
        sizeof(piece),
        0,
        true
      );

      if (size == 0) {
        callback(bytes::Buffer(0), true);
        return true;
      }

      if (size < 0) {
        debug("llama_token_to_piece failed");
        return false;
      }

      for (int i = 0; i < size; ++i) {
        generation.push(piece[i]);
      }

      if (generation.size() > 0) {
        // - enumerate each antiprompt
        // - look at tail generation buffer
        // - if generation tail expands to the prefix
        //   of an antiprompt, signal `isDetectingAntiprompt = true`
        for (size_t i = generation.size() - 1; i >= 0; --i) {
          if (generation.size() - i > antipromptMaxSize) {
            break;
          }

          const auto tail = generation.slice(i, generation.size());
          if (tail.size() == 0) {
            continue;
          }

          isDetectingAntiprompt = false;
          for (const auto& antiprompt : antiprompts) {
            if (bytes::Buffer::compare(tail, antiprompt) == 0) {
              callback(bytes::Buffer(0), true);
              return true;
            }

            bool startsWith = false;
            for (size_t j = 0; j < tail.size(); ++j) {
              if (antiprompt[j] == tail[j]) {
                startsWith = true;
              } else {
                startsWith = false;
                break;
              }
            }

            if (startsWith) {
              isDetectingAntiprompt = true;
              break;
            } else {
              isDetectingAntiprompt = false;
            }
          }

          if (isDetectingAntiprompt) {
            break;
          }
        }
      }

      batch = llama_batch_get_one(&token, 1);

      if (!isDetectingAntiprompt && callback != nullptr) {
        const auto buffer = bytes::Buffer::from(piece, size);
        const auto eog = buffer.size() == 0 || (buffer.size() == 1 && buffer[0] == 0);
        callback(std::move(buffer), false);
      }
    }

    return true;
  }

  bool Session::chat (
    const String& prompt,
    const ChatOptions& options,
    const ChatStreamCallback& callback
  ) {
    Vector<bytes::Buffer> generations;
    const auto messageId = crypto::rand64();
    auto chatTemplate = llama_model_chat_template(this->context->model->model);

    this->messages.push_back({
      "user",
      bytes::Buffer::from(prompt),
      messageId
    });

    this->history.push_back(this->messages.back().data());

    debug("template: %s", chatTemplate);

    for (const auto& message : this->messages) {
      debug("%s", message.json().str().c_str());
    }

    auto size = llama_chat_apply_template(
      chatTemplate,
      this->history.data(),
      this->history.size(),
      true,
      this->tokens.data(),
      this->tokens.size()
    );

    if (size > static_cast<int>(this->tokens.size())) {
      this->tokens.resize(size);
      size = llama_chat_apply_template(
        chatTemplate,
        this->history.data(),
        this->history.size(),
        true,
        this->tokens.data(),
        this->tokens.size()
      );
    }

    if (size < 0) {
      debug("llama_chat_apply_template failed");
      return false;
    }

    String input(
      this->tokens.begin() + this->tokenCount,
      this->tokens.begin() + size
    );

    debug("input: %s", input.c_str());
    const auto generated = this->generate(input, options, [
      messageId,
      &callback,
      &generations
    ] (auto result, auto eog) mutable {
        generations.push_back(result);
        if (callback != nullptr) {
          callback(messageId, result, eog);
        }
      }
    );

    if (!generated) {
      return false;
    }

    this->messages.push_back({
      "assistant",
      bytes::Buffer::concat(generations)
    });

    this->history.push_back(this->messages.back().data());

    this->tokenCount = llama_chat_apply_template(
      chatTemplate,
      this->history.data(),
      this->history.size(),
      false,
      nullptr,
      0
    );

    if (this->tokenCount < 0) {
      return false;
    }

    return true;
  }

  size_t Session::size () const {
    return this->tokenCount.load(std::memory_order_acquire);
  }

  JSON::Object Session::json () const {
    JSON::Array messages;
    for (const auto& message : this->messages) {
      messages.push(message.json());
    }
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"model", this->context->model->name},
      {"messages", messages}
    };
  }
}
