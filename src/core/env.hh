#ifndef SOCKET_RUNTIME_CORE_ENV_H
#define SOCKET_RUNTIME_CORE_ENV_H

#include "../platform/types.hh"

namespace SSC::Env {
  bool has (const char* name);
  bool has (const String& name);

  String get (const char* name);
  String get (const String& name);
  String get (const String& name, const String& fallback);

  void set (const String& name, const String& value);
  void set (const char* name);
}

#endif
