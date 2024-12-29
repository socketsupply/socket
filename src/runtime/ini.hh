#ifndef SOCKET_RUNTIME_RUNTIME_INI_H
#define SOCKET_RUNTIME_RUNTIME_INI_H

#include "platform.hh"

namespace ssc::runtime::INI {
  using Map = Map<String, String>;
  Map parse (const String& source);
  Map parse (const String& source, const String& keyPathSeparator);
  String serialize (const Map&);
}
#endif
