#ifndef SOCKET_RUNTIME_IO_H
#define SOCKET_RUNTIME_IO_H

#include <iostream>

#include "platform.hh"
#include "env.hh"

namespace ssc::runtime::io {
  void write (const String& input, bool isErrorOutput = false);
}
#endif
