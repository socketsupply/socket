#include <llama.h>

#include "../../debug.hh"
#include "../llm.hh"

namespace ssc::runtime::ai::llm {
  bool Manager::init () {
    if (!this->isInitialized) {
      Lock lock(this->mutex);
      ggml_backend_load_all();
      this->isInitialized = true;
      llama_log_set([](enum ggml_log_level level, const char* message, void*) {
        if (level >= GGML_LOG_LEVEL_ERROR) {
          debug("%s", message);
        }
      }, nullptr);
      return true;
    }

    return false;
  }

  SharedPointer<Model> Manager::loadModel (const Model::Options& options) {
    Lock lock(this->mutex);

    if (options.id > 0) {
      for (const auto& entry : this->models) {
        if (entry.second->id == options.id) {
          return entry.second;
        }
      }
    }

    if (options.name.empty()) {
      return nullptr;
    }

    if (this->models.contains(options.name)) {
      return this->models.at(options.name);
    }

    auto model = std::make_shared<Model>(options);

    if (model->load()) {
      this->models.insert_or_assign(model->name, model);
      return model;
    }

    return nullptr;
  }

  bool Manager::unloadModel (ID id) {
    Lock lock(this->mutex);
    for (const auto& entry : this->models) {
      if (entry.second->id == id) {
        this->models.erase(entry.first);
        return true;
      }
    }

    return false;
  }

  bool Manager::unloadModel (const String& name) {
    Lock lock(this->mutex);
    if (this->models.contains(name)) {
      this->models.erase(name);
      return true;
    }

    return false;
  }

  SharedPointer<LoRA> Manager::loadLoRA (ID id) {
    Lock lock(this->mutex);

    if (id > 0 && this->loras.contains(id)) {
      return this->loras.at(id);
    }

    return nullptr;
  }

  SharedPointer<LoRA> Manager::loadLoRA (
    SharedPointer<Model> model,
    const LoRA::Options& options
  ) {
    Lock lock(this->mutex);
    if (options.id > 0 && this->loras.contains(options.id)) {
      return this->loras.at(options.id);
    }

    auto lora = std::make_shared<LoRA>(model, options);
    if (lora->load()) {
      this->loras.insert_or_assign(lora->id, lora);
      return lora;
    }
    return lora;
  }

  bool Manager::unloadLora (ID id) {
    Lock lock(this->mutex);
    if (this->loras.contains(id)) {
      auto lora = this->loras.at(id);
      for (const auto& entry : this->contexts) {
        for (size_t i = 0; i < entry.second->loras.size(); ++i) {
          const auto item = entry.second->loras[i];
          if (item == lora) {
            entry.second->loras.erase(entry.second->loras.begin() + i);
            break;
          }
        }
      }

      this->loras.erase(id);
      return true;
    }

    return false;
  }

  SharedPointer<Context> Manager::createContext (
    SharedPointer<Model> model,
    const Context::Options& options
  ) {
    Lock lock(this->mutex);
    auto context = std::make_shared<Context>(model, options);
    this->contexts.insert_or_assign(context->id, context);
    return context;
  }

  SharedPointer<Context> Manager::getContext (ID id) {
    Lock lock(this->mutex);
    if (this->contexts.contains(id)) {
      return this->contexts.at(id);
    }

    return nullptr;
  }

  bool Manager::destroyContext (ID id) {
    Lock lock(this->mutex);
    if (this->contexts.contains(id)) {
      this->contexts.erase(id);
      return true;
    }

    return false;
  }
}
