#ifndef SOCKET_RUNTIME_CORE_INI_H
#define SOCKET_RUNTIME_CORE_INI_H

#include "../platform/types.hh"

namespace SSC::INI {
  Map parse (const String& source);
  Map parse (const String& source, const String& keyPathSeparator);
}
#endif
