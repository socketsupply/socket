#ifndef SOCKET_RUNTIME_CORE_SERVICES_AI_H
#define SOCKET_RUNTIME_CORE_SERVICES_AI_H

#include "../../ai.hh"
#include "../../ipc.hh"
#include "../../core.hh"

namespace ssc::runtime::core::services {
  class AI : public core::Service {
    public:

      class LLM : public core::Service {
        public:
          struct LoadModelOptions : public ai::llm::Model::Options {};
          struct LoadLoRAOptions : public ai::llm::LoRA::Options {
            LoadModelOptions model;
          };

          struct CreateContextOptions : public ai::llm::Context::Options {
            LoadModelOptions model;
          };

          ai::llm::Manager manager;

          LLM (const Options& options)
            : core::Service(options)
          {
            if (options.enabled) {
              this->manager.init();
              this->queue.limit = 8;
            }
          }

          void loadModel (
            const ipc::Message::Seq&,
            const LoadModelOptions&,
            const Callback&
          );

          void listModels (
            const ipc::Message::Seq&,
            const Callback&
          );

          void loadLoRA (
            const ipc::Message::Seq&,
            const LoadLoRAOptions&,
            const Callback&
          );

          void attachLoRa (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const ai::llm::ID,
            const ai::llm::LoRA::AttachOptions&,
            const Callback&
          );

          void detachLoRa (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const ai::llm::ID,
            const Callback&
          );

          void createContext (
            const ipc::Message::Seq&,
            const CreateContextOptions&,
            const Callback&
          );

          void destroyContext (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const Callback&
          );

          void getContextStats (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const Callback&
          );

          void dumpContextState (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const Callback&
          );

          void restoreContextState (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const bytes::Buffer&,
            const Callback&
          );
      };

      class Chat : public core::Service {
        public:
          struct GenerateOptions {
            String prompt;
            Vector<String> antiprompts;
          };

          Mutex mutex;
          Map<ai::chat::ID, SharedPointer<ai::chat::Session>> sessions;

          Chat (const Options& options)
            : core::Service(options)
          {
            if (options.enabled) {
              this->queue.limit = 8;
            }
          }

          void list (
            const ipc::Message::Seq&,
            const Callback&
          );

          void history (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const Callback&
          );

          void generate (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const GenerateOptions&,
            const Callback&
          );

          void message  (
            const ipc::Message::Seq&,
            const ai::llm::ID,
            const GenerateOptions&,
            const Callback&
          );
      };

      LLM llm;
      Chat chat;

      AI (const Options& options)
        : core::Service(options),
          chat(options),
          llm(options)
      {}
  };
}
#endif
