#ifndef SOCKET_RUNTIME_EXTENSION_EXTENSION_H
#define SOCKET_RUNTIME_EXTENSION_EXTENSION_H

#include "../../include/socket/extension.h"
#include "../core/process.hh"
#include "../core/core.hh"
#include "../ipc/ipc.hh"

#if SOCKET_RUNTIME_PLATFORM_WINDOWS
#define SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME ".dll"
#else
#define SOCKET_RUNTIME_EXTENSION_FILENAME_EXTNAME ".so"
#endif

namespace SSC {
  class Extension {
    public:
      struct Context {
        enum class State {
          Error = -1,
          None = 0,
          Init = 1,
          Idle = 2,
          Route = 3
        };

        struct Error {
          int code = 0;
          String name = "";
          String message = "";
          String location = "";
        };

        struct Policy {
          String name;
          bool allowed = false;
          Policy (const String& name, bool allowed)
            : name(name), allowed(allowed)
          {}
        };

        struct Memory {
          std::vector<std::function<void()>> pool;
          Mutex mutex;

          ~Memory ();
          void release ();
          void push (std::function<void()> callback);
          template <typename T, typename C, typename... Args> T* alloc (
            C* ctx,
            Args... args
          ) {
            auto memory = new T(args...);
            memory->context = ctx;
            push([memory]() { delete memory; });
            return memory;
          }

          template <typename T, typename... Args> T* alloc (Args... args) {
            auto memory = new T(args...);
            push([memory]() { delete memory; });
            return memory;
          }

          template <typename T> T* alloc (size_t size) {
            auto memory = new T[size]{0};
            push([memory]() { delete [] memory; });
            return memory;
          }
        };

        using PolicyMap = Map<String, Policy>;

        const Extension* extension = nullptr;
        IPC::Router* router = nullptr;
        Context* context = nullptr;
        void *internal = nullptr;
        const void *data = nullptr;

        Memory memory;
        State state = State::None;
        Error error;
        std::atomic<unsigned int> retain_count = 0;
        PolicyMap policies;
        Map<String, String> config;

        Context () = default;
        Context (const Extension* extension);
        Context (const Context* context);
        Context (const Context& context);
        Context (IPC::Router* router);
        Context (const Context& context, IPC::Router* router);

        void retain ();
        bool release ();

        void setPolicy (const String& name, bool allowed);
        const Policy& getPolicy (const String& name) const;
        bool hasPolicy (const String& name) const;
        bool isAllowed (const String& name) const;
      };

      using Map = SSC::Map<String, std::shared_ptr<Extension>>;
      using Entry = std::shared_ptr<const Extension>;
      using Initializer = std::function<bool(Context*, const void*)>;
      using Deinitializer = std::function<bool(Context*, const void*)>;
      using RouterContexts = SSC::Map<IPC::Router*, Context*>;

      RouterContexts contexts;
      Context context;
      const void *data = nullptr;
      const sapi_extension_registration_t* registration = nullptr;

      // registration
      String name = "";
      String version = "";
      String description = "";
      unsigned long abi = 0;
      void *handle = nullptr;
      Initializer initializer = nullptr;
      Deinitializer deinitializer = nullptr;

      static String getExtensionsDirectory (const String& name);
      static const Extension::Map& all ();
      static const Entry get (const String& name);
      static Context* getContext (const String& name);
      static bool setHandle (const String& name, void* handle);
      static void create (const String& name, const Initializer initializer);
      static bool load (const String& name);
      static bool unload (Context* ctx, const String& name, bool shutdown);
      static bool isLoaded (const String& name);
      static String getExtensionType (const String& name);
      static String getExtensionPath (const String& name);

      static void setRouterContext (
        const String& name,
        IPC::Router* router,
        Context* context
      );

      static Context* getRouterContext (
        const String& name,
        IPC::Router* router
      );

      static void removeRouterContext (
        const String& name,
        IPC::Router* router
      );

      static bool initialize (
        Context* ctx,
        const String& name,
        const void* data = nullptr // owned by caller
      );

      Extension (const String& name, const Initializer initializer);
      Extension (Extension& extension);
  };
}

extern "C" {
  typedef sapi_extension_registration_t* (*sapi_extension_registration_entry)();
  struct sapi_context : public SSC::Extension::Context {
    sapi_context () = default;
    sapi_context (sapi_context* ctx)
      : SSC::Extension::Context(reinterpret_cast<SSC::Extension::Context*>(ctx))
    {}
  };

  struct sapi_process_exec : public SSC::ExecOutput {
    sapi_context_t* context = nullptr;
    sapi_process_exec () = default;
    sapi_process_exec (const SSC::ExecOutput& result)
      : SSC::ExecOutput(result)
    {}
    sapi_process_exec (sapi_context_t* ctx, const SSC::ExecOutput& result)
      : context(ctx), SSC::ExecOutput(result)
    {}
  };

  struct sapi_process_spawn : public SSC::Process {
    public:
      sapi_context_t* context = nullptr;

      sapi_process_spawn (
        const char* command,
        const char* argv,
        SSC::Vector<SSC::String> env,
        const char* path,
        sapi_process_spawn_stderr_callback_t onstdout,
        sapi_process_spawn_stderr_callback_t onstderr,
        sapi_process_spawn_exit_callback_t onexit
      ) : SSC::Process(
        command,
        argv,
        env,
        path,
        [this, onstdout] (auto output) { if (onstdout) { onstdout(this, output.c_str(), output.size()); }},
        [this, onstderr] (auto output) { if (onstderr) { onstderr(this, output.c_str(), output.size()); }},
        [this, onexit] (auto code) { if (onexit) { onexit(this, std::stoi(code)); }}
      )
      {}
  };

  struct sapi_ipc_router : public SSC::IPC::Router {};
  struct sapi_ipc_message : public SSC::IPC::Message {};

  struct sapi_ipc_result : public SSC::IPC::Result {
    sapi_context_t* context = nullptr;
    sapi_ipc_result () {
      this->id = SSC::rand64();
    }
  };

  struct sapi_json_any : public SSC::JSON::Any {
    sapi_context_t* context = nullptr;
  };

  struct sapi_json_null : public SSC::JSON::Null {
    sapi_context_t* context = nullptr;
  };

  struct sapi_json_object : public SSC::JSON::Object {
    sapi_context_t* context = nullptr;
  };

  struct sapi_json_array : public SSC::JSON::Array {
    sapi_context_t* context = nullptr;
  };

  struct sapi_json_boolean : public SSC::JSON::Boolean {
    sapi_context_t* context = nullptr;
    sapi_json_boolean () = default;
    sapi_json_boolean (bool boolean)
      : SSC::JSON::Boolean(boolean)
    {}
    sapi_json_boolean (sapi_context_t* ctx, bool boolean)
      : context(ctx), SSC::JSON::Boolean(boolean)
    {}
  };

  struct sapi_json_number : public SSC::JSON::Number {
    sapi_context_t* context = nullptr;
    sapi_json_number () = default;
    sapi_json_number (int64_t number)
      : SSC::JSON::Number(number)
    {}
    sapi_json_number (sapi_context_t* ctx, int64_t number)
      : context(ctx), SSC::JSON::Number(number)
    {}
  };

  struct sapi_json_string : public SSC::JSON::String {
    sapi_context_t* context = nullptr;
    sapi_json_string () = default;
    sapi_json_string (const char* string)
      : SSC::JSON::String(string)
    {}
    sapi_json_string (sapi_context_t* ctx, const char* string)
      : context(ctx), SSC::JSON::String(string)
    {}
  };

  struct sapi_json_raw : public SSC::JSON::Raw {
    sapi_context_t* context = nullptr;
    sapi_json_raw (
      const char* source
    ) : SSC::JSON::Raw(SSC::String(source))
    {}

    sapi_json_raw (
      sapi_context_t* ctx,
      const char* source
    ) : context(ctx), SSC::JSON::Raw(SSC::String(source))
    {}
  };
};
#endif
