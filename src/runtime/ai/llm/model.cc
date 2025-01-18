#include "../../filesystem.hh"
#include "../../config.hh"
#include "../../env.hh"
#include "../../cwd.hh"
#include "../llm.hh"

using ssc::runtime::config::getUserConfig;

namespace ssc::runtime::ai::llm {
  Model::Model (const Options& options)
    : name(options.name),
      params(llama_model_default_params()),
      options(options)
  {
    this->params.n_gpu_layers = this->options.gpuLayerCount;
  }

  Model::~Model() {
    if (this->model) {
      llama_model_free(this->model);
      this->model = nullptr;
    }
  }

  bool Model::load () {
    Lock lock(this->mutex);

    if (this->isLoaded.load(std::memory_order_acquire)) {
      return true;
    }

    if (this->name.size() == 0) {
      return false;
    }

    if (this->model == nullptr) {
      if (filesystem::Resource::isFile(this->name)) {
        this->model = llama_model_load_from_file(this->name.c_str(), this->params);
        this->filename = this->name;
      }
    }

    if (this->model == nullptr) {
      auto rootModelDirectory = env::get("SOCKET_RUNTIME_AI_LLM_MODEL_PATH");
      if (rootModelDirectory.size() > 0) {
        if (filesystem::Resource::isDirectory(rootModelDirectory)) {
          const auto filename = Path(rootModelDirectory) / this->name;
          if (filesystem::Resource::isFile(filename)) {
            this->model = llama_model_load_from_file(filename.c_str(), this->params);
            this->filename = filename;
          }
        }
      }
    }

    if (this->model == nullptr && this->options.directory.size() > 0) {
      if (filesystem::Resource::isDirectory(this->options.directory)) {
        const auto filename = Path(this->options.directory) / this->name;
        if (filesystem::Resource::isFile(filename)) {
          this->model = llama_model_load_from_file(filename.c_str(), this->params);
          this->filename = filename;
        }
      }
    }

    if (this->model == nullptr) {
      static auto userConfig = getUserConfig();
      if (!userConfig["ai_llm_model_path"].empty()) {
        const auto directory = userConfig["ai_llm_model_path"];
        const auto filename = Path(directory) / this->name;
        if (filesystem::Resource::isFile(filename)) {
          this->model = llama_model_load_from_file(filename.c_str(), this->params);
          this->filename = filename;
        }
      }
    }

    if (this->model == nullptr) {
      const auto filename = Path(getcwd()) / this->name;
      if (filesystem::Resource::isFile(filename)) {
        this->model = llama_model_load_from_file(filename.c_str(), this->params);
        this->filename = filename;
      }
    }

    if (this->model != nullptr) {
      this->vocab = llama_model_get_vocab(this->model);
      if (this->vocab != nullptr) {
        this->isLoaded = true;
        return true;
      }
    }

    this->filename.clear();
    return false;
  }

  bool Model::loaded () const {
    return this->isLoaded.load(std::memory_order_acquire);
  }

  JSON::Object Model::json () const {
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"name", this->name},
      {"loaded", this->loaded()},
      {"filename", this->filename},
      {"options", JSON::Object::Entries {
        {"directory", this->options.directory},
        {"gpuLayerCount", this->options.gpuLayerCount}
      }}
    };
  }
}
