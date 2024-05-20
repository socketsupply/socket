#ifndef SOCKET_RUNTIME_CORE_IO_H
#define SOCKET_RUNTIME_CORE_IO_H

#include "../platform/types.hh"

namespace SSC::IO {
  void write (const String& input, bool isErrorOutput = false);
}

#endif
