#include "../core.hh"
#include "../resource.hh"
#include "ai.hh"

namespace SSC {
  static JSON::Object::Entries ERR_AI_LLM_NOEXISTS (
    const String& source,
    CoreAI::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_AI_LLM_NOEXISTS"},
        {"message", "The requested LLM does not exist"}
      }}
    };
  }

  static JSON::Object::Entries ERR_AI_LLM_MESSAGE (
    const String& source,
    CoreAI::ID id,
    const String& message
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_AI_LLM_MESSAGE"},
        {"message", message}
      }}
    };
  }

  static JSON::Object::Entries ERR_AI_LLM_EXISTS (
    const String& source,
    CoreAI::ID id
  ) {
    return JSON::Object::Entries {
      {"source", source},
      {"err", JSON::Object::Entries {
        {"id", std::to_string(id)},
        {"type", "InternalError"},
        {"code", "ERR_AI_LLM_EXISTS"},
        {"message", "The requested LLM already exists"}
      }}
    };
  }

  SharedPointer<LLM> CoreAI::getLLM (ID id) {
    if (!this->hasLLM(id)) return nullptr;
    Lock lock(this->mutex);
    return this->llms.at(id);
  }

  bool CoreAI::hasLLM (ID id) {
    Lock lock(this->mutex);
    return this->llms.find(id) != this->llms.end();
  }

  void CoreAI::createLLM(
    const String& seq,
    ID id,
    LLMOptions options,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      if (this->hasLLM(id)) {
        auto json = ERR_AI_LLM_EXISTS("ai.llm.create", id);
        return callback(seq, json, Post{});
      }

      /* auto log = [&](String message) {
        const auto json = JSON::Object::Entries {
          {"source", "ai.llm.log"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"message", message}
          }}
        };

        callback("-1", json, Post{});
      }; */

      auto llm = std::make_shared<LLM>(options);
      if (llm->err.size()) {
        auto json = ERR_AI_LLM_MESSAGE("ai.llm.create", id, llm->err);
        return callback(seq, json, Post{});
        return;
      }

      const auto json = JSON::Object::Entries {
        {"source", "ai.llm.create"},
        {"data", JSON::Object::Entries {
          {"id", std::to_string(id)},
        }}
      };

      callback(seq, json, Post{});
      Lock lock(this->mutex);
      this->llms.emplace(id, llm);
    });
  };

  void CoreAI::chatLLM(
    const String& seq,
    ID id,
    String message,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->hasLLM(id)) {
        auto json = ERR_AI_LLM_NOEXISTS("ai.llm.chat", id);
        return callback(seq, json, Post{});
      }

      auto llm = this->getLLM(id);

      llm->chat(message, [&](auto self, auto token, auto isComplete) {
        const auto json = JSON::Object::Entries {
          {"source", "ai.llm.chat"},
          {"data", JSON::Object::Entries {
            {"id", std::to_string(id)},
            {"token", encodeURIComponent(token)},
            {"complete", isComplete}
          }}
        };

        callback("-1", json, Post{});

        return isComplete;
      });
    });
  };

  void CoreAI::destroyLLM(
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->hasLLM(id)) {
        auto json = ERR_AI_LLM_NOEXISTS("ai.llm.destroy", id);
        return callback(seq, json, Post{});
      }

      Lock lock(this->mutex);
      auto llm = this->getLLM(id);
      llm->stopped = true;

      while (llm->interactive) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

      this->llms.erase(id);
    });
  };

  void CoreAI::stopLLM(
    const String& seq,
    ID id,
    const CoreModule::Callback& callback
  ) {
    this->core->dispatchEventLoop([=, this] {
      if (!this->hasLLM(id)) {
        auto json = ERR_AI_LLM_NOEXISTS("ai.llm.stop", id);
        return callback(seq, json, Post{});
      }

      auto llm = this->getLLM(id);
      llm->stopped = true; // remains stopped until chat is called again.
    });
  };

  void LLM::escape(String& input) {
    std::size_t input_len = input.length();
    std::size_t output_idx = 0;

    for (std::size_t input_idx = 0; input_idx < input_len; ++input_idx) {
      if (input[input_idx] == '\\' && input_idx + 1 < input_len) {
        switch (input[++input_idx]) {
          case 'n':  input[output_idx++] = '\n'; break;
          case 'r':  input[output_idx++] = '\r'; break;
          case 't':  input[output_idx++] = '\t'; break;
          case '\'': input[output_idx++] = '\''; break;
          case '\"': input[output_idx++] = '\"'; break;
          case '\\': input[output_idx++] = '\\'; break;
          case 'x':
            // Handle \x12, etc
            if (input_idx + 2 < input_len) {
              const char x[3] = { input[input_idx + 1], input[input_idx + 2], 0 };
              char *err_p = nullptr;
              const long val = std::strtol(x, &err_p, 16);
              if (err_p == x + 2) {
                input_idx += 2;
                input[output_idx++] = char(val);
                break;
              }
            }
          // fall through
          default: {
            input[output_idx++] = '\\';
            input[output_idx++] = input[input_idx]; break;
          }
        }
      } else {
        input[output_idx++] = input[input_idx];
      }
    }

    input.resize(output_idx);
  }

  LLM::LLM (LLMOptions options) {
    llama_log_set([](ggml_log_level level, const char* message, void* llm) {
      debug("LLMSTATUS: %s", message);
      // llm.log(message);
    }, this);

    //
    // set params and init the model and context
    //
    llama_backend_init();
    llama_numa_init(this->params.numa);

    llama_sampling_params& sparams = this->params.sparams;
    this->sampling = llama_sampling_init(sparams);

    if (!this->sampling) this->err = "failed to initialize sampling subsystem";
    if (this->params.seed == LLAMA_DEFAULT_SEED) this->params.seed = time(nullptr);

    this->params.chatml = true;
    this->params.verbose_prompt = false;

    if (this->params.chatml) {
      this->params.prompt = "<|im_start|>system\n" + options.prompt + "<|im_end|>\n\n";
    }

    this->params.n_ctx = 2048;

    FileResource modelResource(options.path);

    if (!modelResource.exists()) {
      this->err = "Unable to access the model file due to permissions";
      return;
    }

    this->params.model = options.path;

    std::tie(this->model, this->ctx) = llama_init_from_gpt_params(this->params);

    if (this->ctx == nullptr) {
      this->err = "Unable to create the context";
      return;
    }

    if (this->model == nullptr) {
      this->err = "Unable to create the model";
      return;
    }

    this->embd_inp = ::llama_tokenize(this->ctx, this->params.prompt.c_str(), true, true);

    //
    // create a guidance context
    //
    if (sparams.cfg_scale > 1.f) {
      struct llama_context_params lparams = llama_context_params_from_gpt_params(this->params);
      this->guidance = llama_new_context_with_model(this->model, lparams);
    }

    if (this->model == nullptr) {
      this->err = "unable to load model";
      return;
    }

    //
    // determine the capacity of the model
    //
    const int n_ctx_train = llama_n_ctx_train(this->model);
    const int n_ctx = llama_n_ctx(this->ctx);

    if (n_ctx > n_ctx_train) {
      LOG("warning: model was trained on only %d context tokens (%d specified)\n", n_ctx_train, n_ctx);
    }

    this->n_ctx = n_ctx;

    if (this->guidance) {
      this->guidance_inp = ::llama_tokenize(this->guidance, sparams.cfg_negative_prompt, true, true);
      std::vector<llama_token> original_inp = ::llama_tokenize(ctx, params.prompt.c_str(), true, true);
      original_prompt_len = original_inp.size();
      guidance_offset = (int)this->guidance_inp.size() - original_prompt_len;
    }

    //
    // number of tokens to keep when resetting context
    //
    const bool add_bos = llama_should_add_bos_token(this->model);
    GGML_ASSERT(llama_add_eos_token(this->model) != 1);

    if (this->params.n_keep < 0 || this->params.n_keep > (int)this->embd_inp.size() || this->params.instruct || this->params.chatml) {
      this->params.n_keep = (int)this->embd_inp.size();
    } else if (add_bos) {
      this->params.n_keep += add_bos;
    }

    if (this->params.instruct) {
      this->params.interactive_first = true;
      this->params.antiprompt.emplace_back("### Instruction:\n\n");
    } else if (this->params.chatml) {
      this->params.interactive_first = true;
      this->params.antiprompt.emplace_back("<|im_start|>user\n");
    } else if (this->params.conversation) {
      this->params.interactive_first = true;
    }

    if (params.interactive_first) {
      params.interactive = true;
    }

    if (params.interactive) {
      if (!params.antiprompt.empty()) {
        for (const auto & antiprompt : params.antiprompt) {
          LOG("Reverse prompt: '%s'\n", antiprompt.c_str());

          if (params.verbose_prompt) {
            auto tmp = ::llama_tokenize(ctx, antiprompt.c_str(), false, true);

            for (int i = 0; i < (int) tmp.size(); i++) {
              LOG("%6d -> '%s'\n", tmp[i], llama_token_to_piece(ctx, tmp[i]).c_str());
            }
          }
        }
      }

      if (params.input_prefix_bos) {
        LOG("Input prefix with BOS\n");
      }

      if (!params.input_prefix.empty()) {
        LOG("Input prefix: '%s'\n", params.input_prefix.c_str());

        if (params.verbose_prompt) {
          auto tmp = ::llama_tokenize(ctx, params.input_prefix.c_str(), true, true);

          for (int i = 0; i < (int) tmp.size(); i++) {
            LOG("%6d -> '%s'\n", tmp[i], llama_token_to_piece(ctx, tmp[i]).c_str());
          }
        }
      }

      if (!params.input_suffix.empty()) {
        LOG("Input suffix: '%s'\n", params.input_suffix.c_str());

        if (params.verbose_prompt) {
          auto tmp = ::llama_tokenize(ctx, params.input_suffix.c_str(), false, true);

          for (int i = 0; i < (int) tmp.size(); i++) {
            LOG("%6d -> '%s'\n", tmp[i], llama_token_to_piece(ctx, tmp[i]).c_str());
          }
        }
      }
    }

    //
    // initialize any anti-prompts sent in as params
    //
    this->antiprompt_ids.reserve(this->params.antiprompt.size());

    for (const String& antiprompt : this->params.antiprompt) {
      this->antiprompt_ids.emplace_back(::llama_tokenize(this->ctx, antiprompt.c_str(), false, true));
    }

    this->path_session = params.path_prompt_cache;
  };

  LLM::~LLM () {
    llama_free(this->ctx);
    llama_free(this->guidance);
    llama_free_model(this->model);
    llama_sampling_free(this->sampling);
    llama_backend_free();
  };

  void LLM::chat (String buffer, const Cb cb) {
    this->stopped = false;
    int ga_i = 0;

    const int ga_n = this->params.grp_attn_n;
    const int ga_w = this->params.grp_attn_w;

    if (ga_n != 1) {
      GGML_ASSERT(ga_n > 0 && "grp_attn_n must be positive");
      GGML_ASSERT(ga_w % ga_n == 0 && "grp_attn_w must be a multiple of grp_attn_n");
    }

    this->interactive = this->params.interactive_first = true;

    bool display = true;
    bool is_antiprompt = false;
    bool input_echo = false;
    int n_remain = this->params.n_predict;

    std::vector<int> input_tokens;
    this->input_tokens = &input_tokens;

    std::vector<int> output_tokens;
    this->output_tokens = &output_tokens;

    std::ostringstream output_ss;
    this->output_ss = &output_ss;

    std::vector<llama_token> embd;
    std::vector<llama_token> embd_guidance;

    const int n_ctx = this->n_ctx;
    const auto inp_pfx = ::llama_tokenize(ctx, "\n\n### Instruction:\n\n", true,  true);
    const auto inp_sfx = ::llama_tokenize(ctx, "\n\n### Response:\n\n",    false, true);

    LOG("inp_pfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_pfx).c_str());
    LOG("inp_sfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_sfx).c_str());

    const auto cml_pfx = ::llama_tokenize(ctx, "\n<|im_start|>user\n", true, true);
    const auto cml_sfx = ::llama_tokenize(ctx, "<|im_end|>\n<|im_start|>assistant\n", false, true);

    while ((n_remain != 0 && !is_antiprompt) || this->params.interactive) {
      if (!embd.empty()) {
        int max_embd_size = n_ctx - 4;

        if ((int) embd.size() > max_embd_size) {
          const int skipped_tokens = (int)embd.size() - max_embd_size;
          embd.resize(max_embd_size);
          LOG("<<input too long: skipped %d token%s>>", skipped_tokens, skipped_tokens != 1 ? "s" : "");
        }

        if (ga_n == 1) {
          if (n_past + (int) embd.size() + std::max<int>(0, guidance_offset) >= n_ctx) {
            if (this->params.n_predict == -2) {
              LOG("\n\ncontext full and n_predict == -%d => stopping\n", this->params.n_predict);
              break;
            }

            const int n_left = n_past - this->params.n_keep;
            const int n_discard = n_left/2;

            LOG("context full, swapping: n_past = %d, n_left = %d, n_ctx = %d, n_keep = %d, n_discard = %d\n",
              n_past, n_left, n_ctx, this->params.n_keep, n_discard);

            llama_kv_cache_seq_rm (ctx, 0, this->params.n_keep, this->params.n_keep + n_discard);
            llama_kv_cache_seq_add(ctx, 0, this->params.n_keep + n_discard, n_past, -n_discard);

            n_past -= n_discard;

            if (this->guidance) {
              n_past_guidance -= n_discard;
            }

            LOG("after swap: n_past = %d, n_past_guidance = %d\n", n_past, n_past_guidance);
            LOG("embd: %s\n", LOG_TOKENS_TOSTR_PRETTY(this->ctx, embd).c_str());
            LOG("clear session path\n");
            this->path_session.clear();
          }
        } else {
          while (n_past >= ga_i + ga_w) {
            const int ib = (ga_n*ga_i)/ga_w;
            const int bd = (ga_w/ga_n)*(ga_n - 1);
            const int dd = (ga_w/ga_n) - ib*bd - ga_w;

            LOG("\n");
            LOG("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i, n_past, ib*bd, ga_i + ib*bd, n_past + ib*bd);
            LOG("div:   [%6d, %6d] / %6d -> [%6d, %6d]\n", ga_i + ib*bd, ga_i + ib*bd + ga_w, ga_n, (ga_i + ib*bd)/ga_n, (ga_i + ib*bd + ga_w)/ga_n);
            LOG("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i + ib*bd + ga_w, n_past + ib*bd, dd, ga_i + ib*bd + ga_w + dd, n_past + ib*bd + dd);

            llama_kv_cache_seq_add(ctx, 0, ga_i, n_past, ib*bd);
            llama_kv_cache_seq_div(ctx, 0, ga_i + ib*bd, ga_i + ib*bd + ga_w, ga_n);
            llama_kv_cache_seq_add(ctx, 0, ga_i + ib*bd + ga_w, n_past + ib*bd, dd);

            n_past -= bd;

            ga_i += ga_w/ga_n;

            LOG("\nn_past_old = %d, n_past = %d, ga_i = %d\n\n", n_past + bd, n_past, ga_i);
          }
        }

        if (n_session_consumed < (int) this->session_tokens.size()) {
          size_t i = 0;

          for ( ; i < embd.size(); i++) {
            if (embd[i] != this->session_tokens[n_session_consumed]) {
              this->session_tokens.resize(n_session_consumed);
              break;
            }

            n_past++;
            n_session_consumed++;

            if (n_session_consumed >= (int) this->session_tokens.size()) {
              ++i;
              break;
            }
          }

          if (i > 0) {
            embd.erase(embd.begin(), embd.begin() + i);
          }
        }

        if (this->guidance) {
          int input_size = 0;
          llama_token * input_buf = nullptr;

          if (n_past_guidance < (int)this->guidance_inp.size()) {
            embd_guidance = this->guidance_inp;

            if (embd.begin() + original_prompt_len < embd.end()) {
              embd_guidance.insert(embd_guidance.end(), embd.begin() + original_prompt_len, embd.end());
            }

            input_buf  = embd_guidance.data();
            input_size = embd_guidance.size();

            LOG("guidance context: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_guidance).c_str());
          } else {
            input_buf  = embd.data();
            input_size = embd.size();
          }

          for (int i = 0; i < input_size; i += this->params.n_batch) {
            int n_eval = std::min(input_size - i, this->params.n_batch);

            if (llama_decode(this->guidance, llama_batch_get_one(input_buf + i, n_eval, n_past_guidance, 0))) {
              LOG("failed to eval\n");
              return;
            }

            n_past_guidance += n_eval;
          }
        }

        for (int i = 0; i < (int) embd.size(); i += this->params.n_batch) {
          int n_eval = (int) embd.size() - i;

          if (n_eval > this->params.n_batch) {
            n_eval = this->params.n_batch;
          }

          LOG("eval: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd).c_str());

          if (llama_decode(ctx, llama_batch_get_one(&embd[i], n_eval, n_past, 0))) {
            LOG("%s : failed to eval\n", __func__);
            return;
          }

          n_past += n_eval;
        }

        if (!embd.empty() && !this->path_session.empty()) {
          this->session_tokens.insert(this->session_tokens.end(), embd.begin(), embd.end());
          n_session_consumed = this->session_tokens.size();
        }
      }

      embd.clear();
      embd_guidance.clear();

      if ((int)this->embd_inp.size() <= n_consumed && !interactive) {
        const llama_token id = llama_sampling_sample(this->sampling, this->ctx, this->guidance);
        llama_sampling_accept(this->sampling, this->ctx, id, true);

        LOG("last: %s\n", LOG_TOKENS_TOSTR_PRETTY(this->ctx, this->sampling->prev).c_str());
        embd.push_back(id);

        input_echo = true;
        --n_remain;

        LOG("n_remain: %d\n", n_remain);
      } else {
        LOG("embd_inp.size(): %d, n_consumed: %d\n", (int)this->embd_inp.size(), n_consumed);

        while ((int)this->embd_inp.size() > n_consumed) {
          embd.push_back(this->embd_inp[n_consumed]);
          llama_sampling_accept(this->sampling, this->ctx, this->embd_inp[n_consumed], false);

          ++n_consumed;
          if ((int) embd.size() >= this->params.n_batch) {
            break;
          }
        }
      }

      if (input_echo && display) {
        for (auto id : embd) {
          const String token_str = llama_token_to_piece(ctx, id, !this->params.conversation);
          if (this->stopped) {
            llama_sampling_reset(this->sampling);
            this->interactive = false;
            return;
          }

          cb(this, token_str, false);

          std::this_thread::sleep_for(std::chrono::milliseconds(1));

          if (embd.size() > 1) {
            input_tokens.push_back(id);
          } else {
            output_tokens.push_back(id);
            output_ss << token_str;
          }
        }
      }

      if (input_echo && (int)this->embd_inp.size() == n_consumed) {
        display = true;
      }

      if ((int)this->embd_inp.size() <= n_consumed) {
        if (!this->params.antiprompt.empty()) {
          const int n_prev = 32;
          const String last_output = llama_sampling_prev_str(this->sampling, this->ctx, n_prev);

          is_antiprompt = false;

          for (String & antiprompt : this->params.antiprompt) {
            size_t extra_padding = this->params.interactive ? 0 : 2;
            size_t search_start_pos = last_output.length() > static_cast<size_t>(antiprompt.length() + extra_padding)
              ? last_output.length() - static_cast<size_t>(antiprompt.length() + extra_padding)
              : 0;

            if (last_output.find(antiprompt, search_start_pos) != String::npos) {
              if (this->params.interactive) {
                this->interactive = true;
              }

              is_antiprompt = true;
              break;
            }
          }

          llama_token last_token = llama_sampling_last(this->sampling);
          for (std::vector<llama_token> ids : antiprompt_ids) {
            if (ids.size() == 1 && last_token == ids[0]) {
              if (this->params.interactive) {
                this->interactive = true;
              }

              is_antiprompt = true;
              break;
            }
          }

          if (is_antiprompt) {
            LOG("found antiprompt: %s\n", last_output.c_str());
          }
        }

        if (llama_token_is_eog(model, llama_sampling_last(this->sampling))) {
          LOG("found an EOG token\n");

          if (this->params.interactive) {
            if (!this->params.antiprompt.empty()) {
              const auto first_antiprompt = ::llama_tokenize(this->ctx, this->params.antiprompt.front().c_str(), false, true);
              this->embd_inp.insert(this->embd_inp.end(), first_antiprompt.begin(), first_antiprompt.end());
              is_antiprompt = true;
            }

            this->interactive = true;
          } else if (this->params.instruct || this->params.chatml) {
            this->interactive = true;
          }
        }

        if (n_past > 0 && this->interactive) {
          LOG("waiting for user input\n");

          if (this->params.input_prefix_bos) {
            LOG("adding input prefix BOS token\n");
            this->embd_inp.push_back(llama_token_bos(this->model));
          }

          if (!this->params.input_prefix.empty() && !this->params.conversation) {
            LOG("appending input prefix: '%s'\n", this->params.input_prefix.c_str());
          }

          if (buffer.length() > 1) {
            if (!this->params.input_suffix.empty() && !this->params.conversation) {
              LOG("appending input suffix: '%s'\n", this->params.input_suffix.c_str());
            }

            LOG("buffer: '%s'\n", buffer.c_str());

            const size_t original_size = this->embd_inp.size();

            if (this->params.instruct && !is_antiprompt) {
              LOG("inserting instruction prefix\n");
              n_consumed = this->embd_inp.size();
              embd_inp.insert(this->embd_inp.end(), inp_pfx.begin(), inp_pfx.end());
            }

            if (this->params.chatml && !is_antiprompt) {
              LOG("inserting chatml prefix\n");
              n_consumed = this->embd_inp.size();
              embd_inp.insert(this->embd_inp.end(), cml_pfx.begin(), cml_pfx.end());
            }

            if (this->params.escape) {
              this->escape(buffer);
            }

            const auto line_pfx = ::llama_tokenize(this->ctx, this->params.input_prefix.c_str(), false, true);
            const auto line_inp = ::llama_tokenize(this->ctx, buffer.c_str(), false, this->params.interactive_specials);
            const auto line_sfx = ::llama_tokenize(this->ctx, this->params.input_suffix.c_str(), false, true);

            LOG("input tokens: %s\n", LOG_TOKENS_TOSTR_PRETTY(this->ctx, line_inp).c_str());

            this->embd_inp.insert(this->embd_inp.end(), line_pfx.begin(), line_pfx.end());
            this->embd_inp.insert(this->embd_inp.end(), line_inp.begin(), line_inp.end());
            this->embd_inp.insert(this->embd_inp.end(), line_sfx.begin(), line_sfx.end());

            if (this->params.instruct) {
              LOG("inserting instruction suffix\n");
              this->embd_inp.insert(this->embd_inp.end(), inp_sfx.begin(), inp_sfx.end());
            }

            if (this->params.chatml) {
              LOG("inserting chatml suffix\n");
              this->embd_inp.insert(this->embd_inp.end(), cml_sfx.begin(), cml_sfx.end());
            }

            for (size_t i = original_size; i < this->embd_inp.size(); ++i) {
              const llama_token token = this->embd_inp[i];
              this->output_tokens->push_back(token);
              output_ss << llama_token_to_piece(this->ctx, token);
            }

            n_remain -= line_inp.size();
            LOG("n_remain: %d\n", n_remain);
          } else {
            LOG("empty line, passing control back\n");
          }

          input_echo = false;
        }

        if (n_past > 0) {
          if (this->interactive) {
            llama_sampling_reset(this->sampling);
          }
          this->interactive = false;
        }
      }

      if (llama_token_is_eog(this->model, embd.back())) {
        if (this->stopped) {
          llama_sampling_reset(this->sampling);
          this->interactive = false;
          return;
        }

        if (cb(this, "", true)) return;
      }

      if (this->params.interactive && n_remain <= 0 && this->params.n_predict >= 0) {
        n_remain = this->params.n_predict;
        this->interactive = true;
      }
    }
  }
}
