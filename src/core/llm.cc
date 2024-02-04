#include "core.hh"
#include <llama/llama.h>
#include <llama/grammar-parser.h>
#include <llama/common.h>
#include "uv.h"

namespace SSC {
  class Core::LLM::Model {
    public:
      llama_model_params model_params;
      llama_model *model;

      Model (ModelOptions options) {
        model_params = llama_model_default_params();

        if (options.gpuLayers) model_params.n_gpu_layers = options.gpuLayers;
        if (options.vocabOnly) model_params.vocab_only = options.vocabOnly;
        if (options.useMmap) model_params.use_mmap = options.useMmap;
        if (options.useMlock) model_params.use_mlock = options.useMlock;

        llama_backend_init(false);
        model = llama_load_model_from_file(options.path.c_str(), model_params);
      }

      llama_token tokenBos () {
        return llama_token_bos(model);
      }

      llama_token tokenEos () {
        return llama_token_eos(model);
      }

      llama_token tokenNl () {
        return llama_token_nl(model);
      }

      SSC::String getTokenString (uint32_t token) {
        std::stringstream ss;
        const char* str = llama_token_get_text(model, token);
        if (str == nullptr) {
          ss.clear();
          return ss.str();
        }

        ss << str;
        return ss.str();
      }

      ~Model () {
        llama_free_model(model);
      }
  };

  class Core::LLM::Grammar {
    public:
      grammar_parser::parse_state parsed_grammar;
      bool state = false;

      Grammar (SSC::String text) {
        parsed_grammar = grammar_parser::parse(text.c_str());
        if (parsed_grammar.rules.empty()) return;
        state = true;
      }
  };

  class Core::LLM::Evaluator {
    public:
      Grammar* g;
      llama_grammar *grammar = nullptr;

      Evaluator (Grammar* g) : g(g) {
        std::vector<const llama_grammar_element *> grammar_rules(g->parsed_grammar.c_rules());

        grammar = llama_grammar_init(
          grammar_rules.data(), grammar_rules.size(), g->parsed_grammar.symbol_ids.at("root")
        );
      }

      ~Evaluator () {
        if (grammar != nullptr) {
          llama_grammar_free(grammar);
          grammar = nullptr;
        }
      }
  };

  class Core::LLM::Context {
    public:
      Model* model;
      llama_context_params context_params;
      llama_context* ctx;
      int n_cur = 0;

      Context (ContextOptions options) {
        model = LLM::models[options.modelId];
      }

      std::vector<uint32_t> encode (SSC::String& text) {
        std::vector<llama_token> tokens = llama_tokenize(ctx, text, false);
        std::vector<uint32_t> result;

        for (size_t i = 0; i < tokens.size(); ++i) {
          result[i] = static_cast<uint32_t>(tokens[i]);
        }

        return result;
      };

      SSC::String decode (std::vector<llama_token> tokens) {
        std::stringstream ss;
        for (size_t i = 0; i < tokens.size(); i++) {
          const std::string piece = llama_token_to_piece(ctx, (llama_token)tokens[i]);
          if (piece.empty()) continue;
          ss << piece;
        }
        return ss.str();
      };

      uint32_t size () {
        return llama_n_ctx(ctx);
      }
  };

  class Core::LLM::Worker {
    Evaluator* grammar_evaluation_state;
    bool use_grammar = false;
    std::vector<llama_token> tokens;
    float temperature = 0.0f;
    uint32_t top_k = 40;
    float top_p = 0.85f;
    float repeat_penalty = 1.10f; // 1.0 = disabled
    float repeat_penalty_presence_penalty = 0.00f; // 0.0 = disabled
    float repeat_penalty_frequency_penalty = 0.00f; // 0.0 = disabled
    std::vector<llama_token> repeat_penalty_tokens;
    bool use_repeat_penalty = false;
    bool failed = false;
    std::string status = "";

    public:
      Context* ctx;
      String seq;
      Core::Module::Callback cb;
      llama_token result;

      Worker (Core::LLM::WorkerOptions options) {
        bool hasEvaluator = LLM::evaluators.count(options.evaluatorId);

        if (options.temperature != 0.0f) {
          temperature = options.temperature;
        }

        if (options.topK != 40) {
          top_k = options.topK;
        }

        if (options.topP != 0.85f) {
          top_p = options.topP;
        }

        if (options.repeatPenalty != 1.10f) {
          repeat_penalty = options.repeatPenalty;
        }

        if (options.repeatPenaltyTokens.size() > 0) {
          for (const auto& element : options.repeatPenaltyTokens) {
              repeat_penalty_tokens.push_back(static_cast<llama_token>(element));
          }
          use_repeat_penalty = true;
        }

        if (options.repeatPenaltyPresencePenalty > 0.0f) {
          repeat_penalty_presence_penalty = options.repeatPenaltyPresencePenalty;
        }

        if (options.repeatPenaltyFrequencyPenalty > 0.0f) {
          repeat_penalty_frequency_penalty = options.repeatPenaltyFrequencyPenalty;
        }

        if (hasEvaluator) {
          grammar_evaluation_state = LLM::evaluators.at(options.evaluatorId);
          use_grammar = true;
        }
      }

      ~Worker () {
        if (use_grammar) {
          // grammar_evaluation_state
          use_grammar = false;
        }
      }

      void execute () {
        llama_batch batch = llama_batch_init(tokens.size(), 0, 1);

        for (size_t i = 0; i < tokens.size(); i++) {
          llama_batch_add(batch, tokens[i], ctx->n_cur, { 0 }, false);

          ctx->n_cur++;
        }

        GGML_ASSERT(batch.n_tokens == (int) tokens.size());

        batch.logits[batch.n_tokens - 1] = true;

        // Perform the evaluation using llama_decode.
        int r = llama_decode(ctx->ctx, batch);

        llama_batch_free(batch);

        if (r != 0) {
          this->failed = true;

          if (r == 1) {
            this->status = "Could not find a KV slot for the batch (try reducing the size of the batch or increase the context)";
          } else {
            this->status = "Decoding failed";
          }

          return;
        }

        this->status = "Decoding successful";

        llama_token new_token_id = 0;

        // Select the best prediction.
        auto logits = llama_get_logits_ith(ctx->ctx, batch.n_tokens - 1);
        auto n_vocab = llama_n_vocab(ctx->model->model);

        std::vector<llama_token_data> candidates;
        candidates.reserve(n_vocab);

        for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
          candidates.emplace_back(llama_token_data{ token_id, logits[token_id], 0.0f });
        }

        llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

        auto eos_token = llama_token_eos(ctx->model->model);

        if (use_repeat_penalty && !repeat_penalty_tokens.empty()) {
          llama_sample_repetition_penalties(
            ctx->ctx, &candidates_p, repeat_penalty_tokens.data(), repeat_penalty_tokens.size(), repeat_penalty,
            repeat_penalty_frequency_penalty, repeat_penalty_presence_penalty
          );
        }

        if (use_grammar && (grammar_evaluation_state)->grammar != nullptr) {
          llama_sample_grammar(ctx->ctx, &candidates_p, (grammar_evaluation_state)->grammar);
        }

        if (temperature <= 0) {
          new_token_id = llama_sample_token_greedy(ctx->ctx , &candidates_p);
        } else {
          const int32_t resolved_top_k = top_k <= 0 ? llama_n_vocab(ctx->model->model) : std::min(top_k, (uint32_t) llama_n_vocab(ctx->model->model));
          const int32_t n_probs = 0; // Number of probabilities to keep - 0 = disabled
          const float tfs_z = 1.00f; // Tail free sampling - 1.0 = disabled
          const float typical_p = 1.00f; // Typical probability - 1.0 = disabled
          const float resolved_top_p = top_p; // Top p sampling - 1.0 = disabled

          // Temperature sampling
          size_t min_keep = std::max(1, n_probs);
          llama_sample_top_k(ctx->ctx, &candidates_p, resolved_top_k, min_keep);
          llama_sample_tail_free(ctx->ctx, &candidates_p, tfs_z, min_keep);
          llama_sample_typical(ctx->ctx, &candidates_p, typical_p, min_keep);
          llama_sample_top_p(ctx->ctx, &candidates_p, resolved_top_p, min_keep);
          llama_sample_temp(ctx->ctx, &candidates_p, temperature);
          new_token_id = llama_sample_token(ctx->ctx, &candidates_p);
        }

        if (new_token_id != eos_token && use_grammar && (grammar_evaluation_state)->grammar != nullptr) {
          llama_grammar_accept_token(ctx->ctx, (grammar_evaluation_state)->grammar, new_token_id);
        }

        result = new_token_id;
      }
  };

  void Core::LLM::eval (const String seq, const WorkerOptions options, Core::Module::Callback cb) {
    this->core->dispatchEventLoop([=, this]() {

      if (!LLM::models.count(options.modelId)) {
        auto json = JSON::Object::Entries {
          {"source", "llm.eval"},
          {"err", JSON::Object::Entries {
            {"modelId", std::to_string(options.modelId)}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      if (!LLM::contexts.count(options.contextId)) {
        auto json = JSON::Object::Entries {
          {"source", "llm.eval"},
          {"err", JSON::Object::Entries {
            {"contextId", std::to_string(options.modelId)}
          }}
        };

        cb(seq, json, Post{});
        return;
      }

      auto worker = new Worker(options);
      worker->ctx = LLM::contexts.at(options.contextId);
      worker->seq = seq;
      worker->cb = cb;

      auto work = new uv_work_t;
      work->data = worker;

      int r = uv_queue_work(
        this->core->getEventLoop(),
        work,
        [](uv_work_t *req) {
          Worker* worker = static_cast<Worker*>(req->data);
          worker->execute();
        },
        [](uv_work_t *req, int status) {
          Worker* worker = static_cast<Worker*>(req->data);

          auto json = JSON::Object::Entries {
            {"source", "llm.eval"},
            {"data", JSON::Object::Entries {
              {"token", std::to_string(worker->result)}
            }}
          };

          worker->cb(String(worker->seq), json, Post{});
          delete worker;
        }
      );
    });
  }

  void Core::LLM::createModel (const String seq, const ModelOptions options, Core::Module::Callback cb) {
    auto model = new LLM::Model(options);
    uint64_t modelId = rand64();

    LLM::models.insert_or_assign(modelId, model);

    auto json = JSON::Object::Entries {
      {"source", "llm.createModel"},
      {"data", JSON::Object::Entries {
        {"workerId", std::to_string(modelId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::LLM::createEvaluator (const String seq, const uint64_t grammarId, Core::Module::Callback cb) {
    auto hasGrammar = LLM::models.count(grammarId) == 1;

    if (!hasGrammar) {
      auto json = JSON::Object::Entries {
        {"source", "llm.createEvaluator"},
        {"err", JSON::Object::Entries {
          {"grammarId", std::to_string(grammarId)}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto grammar = LLM::grammars.at(grammarId);
    auto evaluator = new LLM::Evaluator(grammar);
    uint64_t evaluatorId = rand64();

    auto json = JSON::Object::Entries {
      {"source", "llm.createEvaluator"},
      {"data", JSON::Object::Entries {
        {"evaluatorId", std::to_string(evaluatorId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::LLM::createContext (const String seq, ContextOptions options, Core::Module::Callback cb) {
    auto hasModel = LLM::models.count(options.modelId) == 1;

    if (!hasModel) {
      auto json = JSON::Object::Entries {
        {"source", "llm.createContext"},
        {"err", JSON::Object::Entries {
          {"modelId", std::to_string(options.modelId)}
        }}
      };

      cb(seq, json, Post{});
      return;
    }

    auto context = new Context(options);
    uint64_t contextId = rand64();
    LLM::contexts.insert_or_assign(contextId, context);

    auto json = JSON::Object::Entries {
      {"source", "llm.createContext"},
      {"data", JSON::Object::Entries {
        {"contextId", std::to_string(contextId)}
      }}
    };

    cb(seq, json, Post{});
  }

  void Core::LLM::parseGrammar (const String seq, const GrammarOptions options, Core::Module::Callback cb) {
    uint64_t grammarId = rand64();
    auto grammar = new Grammar(options.text);
    grammars.insert_or_assign(grammarId, grammar);

    if (!grammar->state) {
      auto json = JSON::Object::Entries {
        {"source", "llm.createContext"},
        {"err", "failed to parse"}
      };

      cb(seq, json, Post{});
      return;
    }

    auto json = JSON::Object::Entries {
      {"source", "llm.createContext"},
      {"data", JSON::Object::Entries {
        {"grammarId", std::to_string(grammarId)}
      }}
    };

    cb(seq, json, Post{});
  }
}
