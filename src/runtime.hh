#ifndef SOCKET_RUNTIME_H
#define SOCKET_RUNTIME_H

#include "runtime/app.hh"
#include "runtime/cwd.hh"
#include "runtime/core.hh"
#include "runtime/bridge.hh"
#include "runtime/env.hh"
#include "runtime/filesystem.hh"
#include "runtime/io.hh"
#include "runtime/ini.hh"
#include "runtime/ipc.hh"
#include "runtime/json.hh"
#include "runtime/javascript.hh"
#include "runtime/loop.hh"
#include "runtime/os.hh"
#include "runtime/platform.hh"
#include "runtime/process.hh"
#include "runtime/runtime.hh"
#include "runtime/string.hh"
#include "runtime/udp.hh"
#include "runtime/url.hh"
#include "runtime/version.hh"
#include "runtime/webview.hh"
#include "runtime/window.hh"

namespace ssc::runtime {
  inline const auto VERSION_FULL_STRING = version::VERSION_FULL_STRING;
  inline const auto VERSION_HASH_STRING = version::VERSION_HASH_STRING;
  inline const auto VERSION_STRING = version::VERSION_STRING;

  using App = app::App;
  using Bridge = bridge::Bridge;
  using Loop = loop::Loop;
  using Process = process::Process;
  using URL = url::URL;
  using Window = window::Window;
}
#endif
