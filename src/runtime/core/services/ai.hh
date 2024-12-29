#ifndef SOCKET_RUNTIME_CORE_SERVICES_AI_H
#define SOCKET_RUNTIME_CORE_SERVICES_AI_H

#include <llama/common/common.h>
#include <llama/llama.h>

#include "../../core.hh"

namespace ssc::runtime::core::services {
  class LLM;

  struct LLMOptions {
    bool conversation = false;
    bool chatml = false;
    int n_ctx = 0;
    bool instruct = false;
    int n_keep = 0;
    int n_batch = 0;
    int n_threads = 0;
    int n_gpu_layers = 0;
    int n_predict = 0;
    int grp_attn_n = 0;
    int grp_attn_w = 0;
    int seed = 0;
    int max_tokens = 0;
    int top_k = 0;
    float top_p = 0.0;
    float min_p = 0.0;
    float tfs_z = 0.0;
    float typical_p = 0.0;
    float temp;

    String path;
    String prompt;
    String antiprompt;
  };

  class AI : public core::Service {
    public:
      using ID = uint64_t;
      using LLMs = Map<ID, SharedPointer<LLM>>;

      Mutex mutex;
      LLMs llms;

      AI (const Options& options)
        : core::Service(options)
      {}

      void chatLLM (const String&, ID, String, const Callback);
      void createLLM (const String&, ID, LLMOptions, const Callback);
      void destroyLLM (const String&, ID, const Callback);
      void stopLLM (const String&, ID , const Callback);
      bool hasLLM (ID id);
      SharedPointer<LLM> getLLM (ID id);
  };

  class LLM {
    using ChatCallback = Function<bool(LLM*, String, bool)>;
    using Logger = Function<void(ggml_log_level, const char*, void*)>;

    gpt_params params;
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    llama_context* guidance = nullptr;
    struct llama_sampling_context* sampling;

    Vector<llama_token>* input_tokens = nullptr;
    std::ostringstream* output_ss = nullptr;
    Vector<llama_token>* output_tokens = nullptr;
    Vector<llama_token> session_tokens;
    Vector<llama_token> embd_inp;
    Vector<llama_token> guidance_inp;
    Vector<Vector<llama_token>> antiprompt_ids;

    String path_session = "";
    int guidance_offset = 0;
    int original_prompt_len = 0;
    int n_ctx = 0;
    int n_past = 0;
    int n_consumed = 0;
    int n_session_consumed = 0;
    int n_past_guidance = 0;

    public:
      static Logger log;
      static void tramp (
        ggml_log_level level,
        const char* message,
        void* user_data
      );

      String err = "";
      bool stopped = false;
      bool interactive = false;

      LLM (LLMOptions options);
      ~LLM();

      void chat (String input, const ChatCallback cb);
      void escape (String& input);
  };
}
#endif
