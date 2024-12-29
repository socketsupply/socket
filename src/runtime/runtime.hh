#ifndef SOCKET_RUNTIME_RUNTIME_H
#define SOCKET_RUNTIME_RUNTIME_H

#include "core.hh"
#include "config.hh"
#include "bridge.hh"
#include "window.hh"
#include "webview.hh"
#include "options.hh"
#include "context.hh"

#include "core/services.hh"

namespace ssc::runtime {
  class Runtime : public context::RuntimeContext {
    public:
      using DispatchCallback = Function<void()>;
      using UserConfig = Map<String, String>;

      struct Options : public ssc::runtime::Options {
        UserConfig userConfig = config::getUserConfig();
        loop::Loop::Options loop;
        core::Services::Features features;
      };

      // managers
      window::Manager windowManager;
      bridge::Manager bridgeManager;

      context::Dispatcher dispatcher;
      UserConfig userConfig;
      core::Services services;
      Options options;

      Runtime (const Options&);
      Runtime () = delete;
      Runtime (const Runtime&) = delete;
      Runtime (Runtime&&) = delete;
      virtual ~Runtime();

      Runtime& operator = (const Runtime&) = delete;
      Runtime& operator = (Runtime&&) = delete;

      void init ();
      bool start ();
      bool stop ();
      bool resume ();
      bool pause();
      bool destroy ();
      bool dispatch (const DispatchCallback&);

      bool stopped () const;
      bool paused () const;
  };
}
#endif
