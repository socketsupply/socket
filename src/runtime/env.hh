#ifndef SOCKET_RUNTIME_ENV_H
#define SOCKET_RUNTIME_ENV_H

#include "platform.hh"

namespace ssc::runtime::env {
  bool has (const char* name);
  bool has (const String& name);

  String get (const char* name);
  String get (const String& name);
  String get (const String& name, const String& fallback);

  void set (const String& name, const String& value);
  void set (const char* name);
}
#endif
