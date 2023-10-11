#ifndef SSC_CONFIG_H
#define SSC_CONFIG_H

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
#include <map>
#include <string>

namespace SSC {
  // from init.cc
  extern const std::map<std::string, std::string> getUserConfig ();
  extern bool isDebugEnabled ();
  extern const char* getDevHost ();
  extern int getDevPort ();
}
#endif

#endif
