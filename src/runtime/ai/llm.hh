#ifndef SOCKET_RUNTIME_AI_LLM_H
#define SOCKET_RUNTIME_AI_LLM_H

#include <llama.h>

#include "../json.hh"
#include "../bytes.hh"
#include "../crypto.hh"

namespace ssc::runtime::ai::llm {
  // forward
  class Context;

  using ID = uint64_t;

  class Model {
    public:
      struct Options {
        String name = "";
        String directory = "";
        size_t gpuLayerCount = 99;
        ai::llm::ID id = 0;
      };

      Mutex mutex;
      ID id = crypto::rand64();
      const Options options;
      String name = "";
      Path filename;
      Atomic<bool> isLoaded = false;

      // llamacpp
      llama_model_params params;
      llama_model* model = nullptr;
      const llama_vocab* vocab = nullptr;

      Model (const Options&);
      ~Model();
      Model (const Model&) = delete;
      Model (Model&&) = delete;
      Model& operator = (const Model&) = delete;
      Model& operator = (Model&&) = delete;

      virtual bool load ();
      bool loaded () const;
      JSON::Object json () const;
  };

  class LoRA {
    public:
      struct Options {
        String name = "";
        String directory = "";
        ai::llm::ID id = 0;
      };

      struct AttachOptions {
        float scale = 0.5f;
      };

      Mutex mutex;
      ID id = crypto::rand64();
      const Options options;
      String name = "";
      Path filename;
      Atomic<bool> isLoaded = false;
      SharedPointer<Model> model = nullptr;

      // llamacpp
      llama_adapter_lora* lora = nullptr;

      LoRA (SharedPointer<Model>, const Options&);
      ~LoRA ();

      LoRA (const LoRA&) = delete;
      LoRA (LoRA&&) = delete;
      LoRA& operator = (const LoRA&) = delete;
      LoRA& operator = (LoRA&&) = delete;

      bool attach (SharedPointer<Context>, const AttachOptions&);
      bool detach (SharedPointer<Context>);

      virtual bool load ();
      bool loaded () const;
      JSON::Object json () const;
  };

  class Context {
    public:
      struct Options {
        size_t size = 2048;
        uint32_t dist = LLAMA_DEFAULT_SEED;
        float minP = 0.05f;
        float temp = 0.8f;
        int topK = 0;
        int topP = 0;
        ID id = 0;
      };

      Mutex mutex;
      ID id = crypto::rand64();
      SharedPointer<Model> model = nullptr;
      Vector<SharedPointer<LoRA>> loras;
      Options options;

      // llamacpp
      llama_context_params params;
      llama_context* context = nullptr;
      llama_sampler* sampler = nullptr;

      Context (SharedPointer<Model>, const Options&);
      ~Context();
      Context (const Context&) = delete;
      Context (Context&&) = delete;
      Context& operator = (const Context&) = delete;
      Context& operator = (Context&&) = delete;

      size_t size () const;
      size_t used () const;
      JSON::Object json () const;
      bool restore (const bytes::Buffer&);
      const bytes::Buffer dump () const;
  };

  class Manager {
    public:
      Mutex mutex;
      Atomic<bool> isInitialized = false;
      Map<String, SharedPointer<Model>> models;
      Map<ID, SharedPointer<Context>> contexts;
      Map<ID, SharedPointer<LoRA>> loras;

      Manager () = default;
      Manager (const Manager&) = delete;
      Manager (Manager&&) = delete;
      Manager& operator = (const Manager&) = delete;
      Manager& operator = (Manager&&) = delete;

      bool init ();

      SharedPointer<Model> loadModel (const Model::Options&);
      bool unloadModel (ID);
      bool unloadModel (const String&);

      SharedPointer<LoRA> loadLoRA (ID);
      SharedPointer<LoRA> loadLoRA (SharedPointer<Model>, const LoRA::Options&);
      bool unloadLora (ID);

      SharedPointer<Context> createContext (SharedPointer<Model>, const Context::Options&);
      SharedPointer<Context> getContext (ID);
      bool destroyContext (ID);
  };
}
#endif
