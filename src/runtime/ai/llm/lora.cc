#include "../../config.hh"
#include "../../env.hh"
#include "../../cwd.hh"
#include "../../filesystem.hh"
#include "../llm.hh"

using ssc::runtime::config::getUserConfig;

namespace ssc::runtime::ai::llm {
  LoRA::LoRA (
    SharedPointer<Model> model,
    const Options& options
  ) : options(options),
      name(options.name)
  {}

  LoRA::~LoRA () {
    if (this->lora != nullptr) {
      llama_adapter_lora_free(this->lora);
      this->lora = nullptr;
    }
  }

  bool LoRA::load () {
    Lock lock(this->mutex);

    if (this->isLoaded.load(std::memory_order_acquire)) {
      return true;
    }

    if (this->name.size() == 0) {
      return false;
    }

    if (this->lora == nullptr) {
      if (filesystem::Resource::isFile(this->name)) {
        this->lora = llama_adapter_lora_init(this->model->model, this->name.c_str());
        this->filename = this->name;
      }
    }

    if (this->lora == nullptr) {
      auto rootLoRADirectory = env::get("SOCKET_RUNTIME_AI_LLM_lora_PATH");
      if (rootLoRADirectory.size() > 0) {
        if (filesystem::Resource::isDirectory(rootLoRADirectory)) {
          const auto filename = Path(rootLoRADirectory) / this->name;
          if (filesystem::Resource::isFile(filename)) {
            this->lora = llama_adapter_lora_init(this->model->model, filename.c_str());
            this->filename = filename;
          }
        }
      }
    }

    if (this->lora == nullptr && this->options.directory.size() > 0) {
      if (filesystem::Resource::isDirectory(this->options.directory)) {
        const auto filename = Path(this->options.directory) / this->name;
        if (filesystem::Resource::isFile(filename)) {
          this->lora = llama_adapter_lora_init(this->model->model, filename.c_str());
          this->filename = filename;
        }
      }
    }

    if (this->lora == nullptr) {
      static auto userConfig = getUserConfig();
      if (!userConfig["ai_llm_lora_path"].empty()) {
        const auto directory = userConfig["ai_llm_lora_path"];
        const auto filename = Path(directory) / this->name;
        if (filesystem::Resource::isFile(filename)) {
          this->lora = llama_adapter_lora_init(this->model->model, filename.c_str());
          this->filename = filename;
        }
      }
    }

    if (this->lora == nullptr) {
      const auto filename = Path(getcwd()) / this->name;
      if (filesystem::Resource::isFile(filename)) {
        this->lora = llama_adapter_lora_init(this->model->model, filename.c_str());
        this->filename = filename;
      }
    }

    this->filename.clear();
    return false;
  }

  bool LoRA::attach (
    SharedPointer<Context> context,
    const AttachOptions& options
  ) {
    if (context == nullptr || this->lora == nullptr) {
      return false;
    }

    ScopedLock lock(this->mutex, context->mutex);
    for (const auto& entry : context->loras) {
      if (entry.get() == this) {
        return false;
      }
    }

    return 0 == llama_set_adapter_lora(
      context->context,
      this->lora,
      options.scale
    );
  }

  bool LoRA::detach (SharedPointer<Context> context) {
    if (context == nullptr || this->lora == nullptr) {
      return false;
    }

    ScopedLock lock(this->mutex, context->mutex);
    for (const auto& entry : context->loras) {
      if (entry.get() == this) {
        return true;
        return 0 == llama_rm_adapter_lora(
          context->context,
          this->lora
        );
      }
    }

    return false;
  }

  bool LoRA::loaded () const {
    return this->isLoaded.load(std::memory_order_acquire);
  }

  JSON::Object LoRA::json () const {
    return JSON::Object::Entries {
      {"id", std::to_string(this->id)},
      {"name", this->name},
      {"loaded", this->loaded()},
      {"filename", this->filename},
      {"options", JSON::Object::Entries {
        {"directory", this->options.directory},
      }}
    };
  }
}
