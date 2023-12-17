#ifndef SOCKET_RUNTIME_TEST_EXTENSIONS_WASM_H
#define SOCKET_RUNTIME_TEST_EXTENSIONS_WASM_H

#include <socket/extension.h>
#include "ok.hh"

#define _CONVERT_TO_STRING(value) #value
#define CONVERT_TO_STRING(value) _CONVERT_TO_STRING(value)

#define test(condition) ({               \
  if ((condition)) {                     \
    ok(CONVERT_TO_STRING(condition));    \
  } else {                               \
    notok(CONVERT_TO_STRING(condition)); \
  }                                      \
})


extern "C" {
  void initialize_libc_tests ();
  void initialize_sapi_tests (sapi_context_t*);
}

#endif
