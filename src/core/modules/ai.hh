#ifndef SOCKET_RUNTIME_CORE_AI_H
#define SOCKET_RUNTIME_CORE_AI_H

#include <llama/common/common.h>
#include <llama/llama.h>

#include "../module.hh"

// #include <cassert>
// #include <cinttypes>
// #include <cmath>
// #include <ctime>

#if defined (_WIN32)
  #define WIN32_LEAN_AND_MEAN

  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
#endif

namespace SSC {
  class LLM;
  class Core;

  struct LLMOptions {
    int attentionCapacity;
    int seed;
    String path;
    String prompt;
  };

  class CoreAI : public CoreModule {
    public:
      using ID = uint64_t;
      using LLMs = std::map<ID, SharedPointer<LLM>>;

      Mutex mutex;
      LLMs llms;

      void chatLLM (
        const String& seq,
        ID id,
        String message,
        const CoreModule::Callback& callback
      );

      void createLLM (
        const String& seq,
        ID id,
        LLMOptions options,
        const CoreModule::Callback& callback
      );

      void destroyLLM (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      void stopLLM (
        const String& seq,
        ID id,
        const CoreModule::Callback& callback
      );

      bool hasLLM (ID id);
      SharedPointer<LLM> getLLM (ID id);

      CoreAI (Core* core)
        : CoreModule(core)
      {}
  };

  class LLM {
    using Cb = std::function<bool(LLM*, String, bool)>;
    using Logger = std::function<void(ggml_log_level, const char*, void*)>;

    gpt_params params;
    llama_model* model;
    llama_context* ctx;
    llama_context* guidance = nullptr;
    struct llama_sampling_context* sampling;

    std::vector<llama_token>* input_tokens;
    std::ostringstream* output_ss;
    std::vector<llama_token>* output_tokens;
    std::vector<llama_token> session_tokens;
    std::vector<llama_token> embd_inp;
    std::vector<llama_token> guidance_inp;
    std::vector<std::vector<llama_token>> antiprompt_ids;

    String path_session = "";
    int guidance_offset = 0;
    int original_prompt_len = 0;
    int n_ctx = 0;
    int n_past = 0;
    int n_consumed = 0;
    int n_session_consumed = 0;
    int n_past_guidance = 0;

    public:
      String err = "";
      bool stopped = false;
      bool interactive = false;

      void chat (String input, const Cb cb);
      void escape (String& input);

      LLM(LLMOptions options);
      ~LLM();

      static void tramp(ggml_log_level level, const char* message, void* user_data);
      static Logger log;
  };
}

#endif
