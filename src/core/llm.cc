#include "core.hh"
#include <llama/llama.h>
#include <llama/grammar-parser.h>
#include <llama/common.h>
#include "uv.h"

namespace SSC {
  class LLM {
    public:
      class Model;
      class Grammar;
      class Evaluator;
      class Context;
      class Worker;

      static std::map<uint64_t, Model*> models;
      static std::map<uint64_t, Grammar*> grammars;
      static std::map<uint64_t, Evaluator*> evaluators;
      static std::map<uint64_t, Context*> contexts;
      static std::map<uint64_t, Worker*> workers;

      struct ModelOptions {
        unsigned int gpuLayers;
        bool vocabOnly;
        bool useMmap;
        bool useMlock;
      };

      struct GrammarOptions {
        std::string grammarCode;
        bool printGrammar;
      };

      struct ContextOptions {
        int32_t seed = -1;
        int32_t batchSize = -1;
        uint32_t contextSize = 4096;
        int32_t threads = -1;
        bool embedding;
      };

      struct WorkerOptions {
        double temperature = 0.0f;
        uint32_t topK = 40;
        float topP = 0.85f;
        float repeatPenalty = 1.5f;
        std::vector<llama_token> repeatPenaltyTokens;
        float repeatPenaltyPresencePenalty = 0;
        float repeatPenaltyFrequencyPenalty = 0;
        uint64_t grammarEvaluationState = 0;
      };

      class Model {
        public:
          llama_model_params model_params;
          llama_model *model;

          Model (const std::filesystem::path path, ModelOptions options) {
            model_params = llama_model_default_params();

            if (options.gpuLayers) model_params.n_gpu_layers = options.gpuLayers;
            if (options.vocabOnly) model_params.vocab_only = options.vocabOnly;
            if (options.useMmap) model_params.use_mmap = options.useMmap;
            if (options.useMlock) model_params.use_mlock = options.useMlock;

            llama_backend_init(false);
            model = llama_load_model_from_file(path.c_str(), model_params);
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

      class Grammar {
        public:
          grammar_parser::parse_state parsed_grammar;
          bool hasParsedGrammar = false;

          Grammar (SSC::String text) {
            parsed_grammar = grammar_parser::parse(text.c_str());
            if (parsed_grammar.rules.empty()) return;
            hasParsedGrammar = true;
          }
      };

      class Evaluator {
        public:
          Grammar* grammarDef;
          llama_grammar *grammar = nullptr;

          Evaluator (Grammar* grammarDef) : grammarDef(grammarDef) {
            std::vector<const llama_grammar_element *> grammar_rules(grammarDef->parsed_grammar.c_rules());
            grammar = llama_grammar_init(
              grammar_rules.data(), grammar_rules.size(), grammarDef->parsed_grammar.symbol_ids.at("root")
            );
          }

          ~Evaluator () {
            if (grammar != nullptr) {
              llama_grammar_free(grammar);
              grammar = nullptr;
            }
          }
      };

      class Context {
        public:
          Model* model;
          llama_context_params context_params;
          llama_context* ctx;
          int n_cur = 0;

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
          
          void eval () {

          }
      };

      class Worker {
        Context* ctx;
        Evaluator* grammar_evaluation_state;
        bool use_grammar = false;
        std::vector<llama_token> tokens;
        llama_token result;
        float temperature = 0.0f;
        uint32_t top_k = 40;
        float top_p = 0.85f;
        float repeat_penalty = 1.10f; // 1.0 = disabled
        float repeat_penalty_presence_penalty = 0.00f; // 0.0 = disabled
        float repeat_penalty_frequency_penalty = 0.00f; // 0.0 = disabled
        std::vector<llama_token> repeat_penalty_tokens;
        bool use_repeat_penalty = false;

        public:
          Worker (WorkerOptions options) {
            bool hasEvaluator = evaluators.count(options.grammarEvaluationState);

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
              repeat_penalty_tokens.insert(
                  options.repeatPenaltyTokens.end(),
                  options.repeatPenaltyTokens.begin(),
                  options.repeatPenaltyTokens.end()
                );
              use_repeat_penalty = true;
            }

            if (options.repeatPenaltyPresencePenalty > 0.0f) {
              repeat_penalty_presence_penalty = options.repeatPenaltyPresencePenalty;
            }

            if (options.repeatPenaltyFrequencyPenalty > 0.0f) {
              repeat_penalty_frequency_penalty = options.repeatPenaltyFrequencyPenalty;
            }

            if (options.grammarEvaluationState != 0 && hasEvaluator) {
              grammar_evaluation_state = evaluators.at(options.grammarEvaluationState);
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
              if (r == 1) {
                // "could not find a KV slot for the batch (try reducing the size of the batch or increase the context)"
              } else {
                // "Eval has failed"
              }

              return;
            }

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
    };

  // void Core::LLM::create() {}
}
