#include "../llm.hh"

namespace ssc::runtime::ai::llm {
  Context::Context (
    SharedPointer<Model> model,
    const Options& options
  ) : options(options),
      sampler(llama_sampler_chain_init(llama_sampler_chain_default_params())),
      params(llama_context_default_params()),
      model(model)
  {
    this->params.n_ctx = this->options.size;
    this->params.n_batch = this->options.size;
    this->params.n_ubatch = 512;

    if (this->model != nullptr) {
      this->context = llama_init_from_model(
        this->model->model,
        this->params
      );
    }

    if (this->sampler != nullptr) {
      if (options.minP != 0) {
        llama_sampler_chain_add(this->sampler, llama_sampler_init_min_p(options.minP, 1));
      }

      if (options.temp != 0) {
        llama_sampler_chain_add(this->sampler, llama_sampler_init_temp(options.temp));
      }

      if (options.dist > 0) {
        llama_sampler_chain_add(this->sampler, llama_sampler_init_dist(options.dist));
      }

      if (options.topK != 0) {
        llama_sampler_chain_add(this->sampler, llama_sampler_init_top_k(options.topK));
      }

      if (options.topP != 0) {
        llama_sampler_chain_add(this->sampler, llama_sampler_init_top_k(options.topP));
      }
    }

    if (options.id > 0) {
      this->id = options.id;
    }
  }

  Context::~Context () {
    if (this->sampler) {
      llama_sampler_free(this->sampler);
      this->sampler = nullptr;
    }

    if (this->context) {
      llama_clear_adapter_lora(this->context);
      llama_free(this->context);
      this->context = nullptr;
    }
  }

  size_t Context::size () const {
    if (this->context != nullptr) {
      return llama_n_ctx(this->context);
    }

    return 0;
  }

  size_t Context::used () const {
    if (this->context != nullptr) {
      return llama_get_kv_cache_used_cells(this->context);
    }

    return 0;
  }

  JSON::Object Context::json () const {
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"size", this->size()},
      {"used", this->used()},
      {"model", this->model->json()},
      {"options", JSON::Object::Entries {
        {"size", this->options.size},
        {"minP", this->options.minP},
        {"temp", this->options.temp},
        {"topK", this->options.topK}
      }}
    };
  }

  bool Context::restore (const bytes::Buffer& buffer) {
    if (this->context == nullptr) {
      return false;
    }

    if (llama_state_set_data(this->context, buffer.data(), buffer.size()) > 0) {
      return true;
    }

    return false;
  }

  const bytes::Buffer Context::dump () const {
    if (this->context == nullptr) {
      return bytes::Buffer(0);
    }

    const auto size = llama_state_get_size(this->context);
    auto data = std::make_shared<uint8_t[]>(size);
    llama_state_get_data(this->context, data.get(), size);
    return bytes::Buffer::from(data.get(), size);
  }
}
