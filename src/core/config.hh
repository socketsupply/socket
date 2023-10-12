#ifndef SSC_CORE_CONFIG_H
#define SSC_CORE_CONFIG_H

// TODO(@jwerle): remove this and any need for it
#ifndef SSC_SETTINGS
#define SSC_SETTINGS ""
#endif

#ifndef SSC_VERSION
#define SSC_VERSION ""
#endif

#ifndef SSC_VERSION_HASH
#define SSC_VERSION_HASH ""
#endif

// TODO(@jwerle): use a better name
#if !defined(WAS_CODESIGNED)
#define WAS_CODESIGNED 0
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef DEBUG
#define DEBUG 0
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef HOST
#define HOST "localhost"
#endif

// TODO(@jwerle): stop using this and prefer a namespaced macro
#ifndef PORT
#define PORT 0
#endif

#if defined(__cplusplus)
#include "types.hh"

namespace SSC {
  // implemented in `init.cc`
  extern const Map getUserConfig ();
  extern bool isDebugEnabled ();
  extern const char* getDevHost ();
  extern int getDevPort ();
}
#endif

#endif
