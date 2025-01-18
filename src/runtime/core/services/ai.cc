#include "../services.hh"
#include "ai.hh"

namespace ssc::runtime::core::services {
  void AI::LLM::loadModel (
    const ipc::Message::Seq& seq,
    const LoadModelOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto model = this->manager.loadModel(options);
      if (model != nullptr) {
        const auto json = JSON::Object::Entries {
          {"data", model->json()}
        };
        callback(seq, json, QueuedResponse{});
      } else {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Failed to load model"}
          }}
        };

        callback(seq, json, QueuedResponse{});
      }
    });
  }

  void AI::LLM::listModels (
    const ipc::Message::Seq& seq,
    const Callback& callback
  ) {
    JSON::Array models;
    Lock lock(this->manager.mutex);
    for (const auto& entry : this->manager.models) {
      models.push(entry.second->json());
    }
    const auto json = JSON::Object::Entries {
      {"data", models}
    };
    callback(seq, models, QueuedResponse{});
  }

  void AI::LLM::loadLoRA (
    const ipc::Message::Seq& seq,
    const LoadLoRAOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto model = this->manager.loadModel(options.model);
      if (model == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Failed to load model"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto lora = this->manager.loadLoRA(model, options);
      const auto json = JSON::Object::Entries {
        {"data", lora->json()}
      };
      return callback(seq, json, QueuedResponse{});
    });
  }

  void AI::LLM::attachLoRa (
    const ipc::Message::Seq& seq,
    const ai::llm::ID loraId,
    const ai::llm::ID contextId,
    const ai::llm::LoRA::AttachOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto lora = this->manager.loadLoRA(loraId);
      const auto context = this->manager.getContext(contextId);

      if (lora == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Failed to load lora"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      if (!lora->attach(context, options)) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to attach loRA to loaded context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto json = JSON::Object::Entries {
        {"data", lora->json()}
      };

      callback(seq, json, QueuedResponse{});
    });
  }

  void AI::LLM::detachLoRa (
    const ipc::Message::Seq& seq,
    const ai::llm::ID loraId,
    const ai::llm::ID contextId,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto lora = this->manager.loadLoRA(loraId);
      const auto context = this->manager.getContext(contextId);

      if (lora == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Failed to load lora"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      if (!lora->detach(context)) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to detach loRA to loaded context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto json = JSON::Object::Entries {
        {"data", lora->json()}
      };

      callback(seq, json, QueuedResponse{});
    });
  }

  void AI::LLM::createContext (
    const ipc::Message::Seq& seq,
    const CreateContextOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto model = this->manager.loadModel(options.model);
      if (model == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Failed to load model"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto context = this->manager.createContext(model, options);
      const auto json = JSON::Object::Entries {
        {"data", context->json()}
      };
      callback(seq, json, QueuedResponse{});
    });
  }

  void AI::LLM::destroyContext (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto context = this->manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      this->manager.destroyContext(id);
      return callback(seq, JSON::Object{}, QueuedResponse{});
    });
  }

  void AI::LLM::getContextStats (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto context = this->manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto json = JSON::Object::Entries {
        {"data", context->json()}
      };
      return callback(seq, json, QueuedResponse{});
    });
  }

  void AI::LLM::dumpContextState (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto context = this->manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      const auto state = context->dump();
      const auto response = QueuedResponse {
        .body = state.shared(),
        .length = state.size()
      };

      return callback(seq, JSON::Object{}, response);
    });
  }

  void AI::LLM::restoreContextState (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const bytes::Buffer& state,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      const auto context = this->manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to load context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      if (!context->restore(state)) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Failed to restore context"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      return callback(seq, JSON::Object{}, QueuedResponse{});
    });
  }

  void AI::Chat::list (
    const ipc::Message::Seq& seq,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      Lock lock(this->mutex);
      JSON::Array sessions;
      for (const auto& entry : this->sessions) {
        sessions.push(entry.second->json());
      }
      const auto json = JSON::Object::Entries {
        {"data", sessions}
      };
      return callback(seq, json, QueuedResponse{});
    });
  }

  void AI::Chat::history (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      if (!this->sessions.contains(id)) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Context does not exist for session"}
          }}
        };
        return callback(seq, json, QueuedResponse{});
      }

      JSON::Array history;
      const auto session = this->sessions.at(id);
      for (const auto& entry : session->messages) {
        history.push(entry.json());
      }
      const auto json = JSON::Object::Entries {
        {"data", history}
      };
      return callback(seq, json, QueuedResponse{});
    });
  }

  void AI::Chat::generate (
    const ipc::Message::Seq& seq,
    const ai::chat::ID id,
    const GenerateOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      SharedPointer<ai::chat::Session> session;
      auto context = this->services.ai.llm.manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Context does not exist for session"}
          }}
        };

        return callback(seq, json, QueuedResponse{});
      }

      do {
        Lock lock(this->mutex);
        if (!this->sessions.contains(id)) {
          this->sessions.insert_or_assign(id, std::make_shared<ai::chat::Session>(context, ai::chat::Session::Options {
            id
          }));
        }

        session = this->sessions.at(id);
      } while (0);

      const auto success = session->generate(options.prompt, { .antiprompts = options.antiprompts }, [=, this](auto buffer, auto eog) {
        const auto json = JSON::Object::Entries {
          {"source", "ai.chat.session.generate"},
          {"data", JSON::Object::Entries {
            {"eog", eog},
            {"id", id}
          }}
        };

        const auto queuedResponse = QueuedResponse {
          .body = buffer.shared(),
          .length = buffer.size()
        };

        callback("-1", json, queuedResponse);
      });

      if (success) {
        return callback(seq, JSON::Object {}, QueuedResponse{});
      } else {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Generation failed"}
          }}
        };

        return callback(seq, json, QueuedResponse{});
      }
    });
  }

  void AI::Chat::message (
    const ipc::Message::Seq& seq,
    const ai::llm::ID id,
    const GenerateOptions& options,
    const Callback& callback
  ) {
    this->queue.push([=, this](){
      SharedPointer<ai::chat::Session> session;
      auto context = this->services.ai.llm.manager.getContext(id);
      if (context == nullptr) {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"type", "NotFoundError"},
            {"message", "Context does not exist for session"}
          }}
        };

        return callback(seq, json, QueuedResponse{});
      }

      do {
        Lock lock(this->mutex);
        if (!this->sessions.contains(id)) {
          this->sessions.insert_or_assign(id, std::make_shared<ai::chat::Session>(context, ai::chat::Session::Options {
            id
          }));
        }
        session = this->sessions.at(id);
      } while (0);

      const auto success = session->chat(options.prompt, { .antiprompts = options.antiprompts }, [=, this](auto id, auto buffer, auto eog) {
        const auto json = JSON::Object::Entries {
          {"source", "ai.chat.session.message"},
          {"data", JSON::Object::Entries {
            {"eog", eog},
            {"id", id}
          }}
        };

        const auto queuedResponse = QueuedResponse {
          .body = buffer.shared(),
          .length = buffer.size()
        };

        callback("-1", json, queuedResponse);
      });

      if (success) {
        return callback(seq, JSON::Object {}, QueuedResponse{});
      } else {
        const auto json = JSON::Object::Entries {
          {"err", JSON::Object::Entries {
            {"message", "Generation failed"}
          }}
        };

        return callback(seq, json, QueuedResponse{});
      }
    });
  }
}
