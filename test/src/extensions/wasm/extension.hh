#ifndef SOCKET_RUNTIME_TEST_EXTENSIONS_WASM_H
#define SOCKET_RUNTIME_TEST_EXTENSIONS_WASM_H

#include <socket/extension.h>
#include "ok.hh"

extern "C" {
  void initialize_libc_tests ();
  void initialize_sapi_tests ();
}

#endif
