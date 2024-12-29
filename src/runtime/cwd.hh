#ifndef SOCKET_RUNTIME_cWD_H
#define SOCKET_RUNTIME_cWD_H

#include "platform.hh"

namespace ssc::runtime {
  void setcwd (const String& value);
  const String getcwd_state_value ();
  const String getcwd ();
}
#endif
