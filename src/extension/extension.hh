#ifndef SSC_EXTENSION_H
#define SSC_EXTENSION_H

#if defined(_WIN32)
#  include <libloaderapi.h>
  // TODO
#else
#  include <dlfcn.h>
#endif

#include "../../include/socket/extension.h"
#include "../process/process.hh"
#include "../core/json.hh"
#include "../core/core.hh"
#include "../ipc/ipc.hh"

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

          template <typename T> T* alloc (unsigned int size) {
            auto memory = new T[size]{0};
            push([memory]() { delete [] memory; });
            return memory;
          }
        };

        const Extension* extension = nullptr;
        IPC::Router* router = nullptr;
        Context* context = nullptr;
        void *internal = nullptr;
        const void *data = nullptr;

        Memory memory;
        State state = State::None;
        Error error;
        std::atomic<bool> retained = false;
        Map config;

        Context () = default;
        Context (const Extension* extension);
        Context (const Context* context);
        Context (const Context& context);
        Context (IPC::Router* router);

        void retain ();
        void release ();
      };

      using Map = std::map<String, std::shared_ptr<Extension>>;
      using Entry = std::shared_ptr<const Extension>;
      using Initializer = std::function<bool(Context*, const void*)>;
      using Deinitializer = std::function<bool(Context*, const void*)>;

      Context context;
      const void *data = nullptr;

      // registration
      String name = "";
      String version = "";
      String description = "";
      unsigned long abi = 0;
      Initializer initializer = nullptr;
      Deinitializer deinitializer = nullptr;

      static String getExtensionsDirectory (const String& name);
      static const Extension::Map& all ();
      static const Entry get (const String& name);
      static void create (const String& name, const Initializer initializer);
      static bool load (const String& name);
      static bool unload (const String& name);
      static bool isLoaded (const String& name);

      static bool initialize (
        const Context* ctx,
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
    sapi_context (sapi_context* ctx) : SSC::Extension::Context(ctx) {}
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

  struct sapi_ipc_router : public SSC::IPC::Router {};
  struct sapi_ipc_message : public SSC::IPC::Message {};

  struct sapi_ipc_result : public SSC::IPC::Result {
    sapi_context_t* context = nullptr;
    sapi_ipc_result () = default;
    sapi_ipc_result (sapi_context_t* ctx)
      : context(ctx)
    {}
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
    sapi_json_boolean (const bool boolean)
      : SSC::JSON::Boolean(boolean)
    {}
    sapi_json_boolean (sapi_context_t* ctx, const bool boolean)
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
};

#endif
