#ifndef SSC_IPC_H
#define SSC_IPC_H

#include "bridge.hh"
#include "client.hh"
#include "message.hh"
#include "navigator.hh"
#include "result.hh"
#include "router.hh"
#include "scheme_handlers.hh"

namespace SSC::IPC {
  inline String getResolveToMainProcessMessage (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return String("ipc://resolve?seq=" + seq + "&state=" + state + "&value=" + value);
  }
}
#endif
