#include "common.hh"

// These rely on project-specific, compile-time variables.
namespace SSC {
  const char* getSettingsSource () {
    static const char* source = STR_VALUE(SSC_SETTINGS);
    return source;
  }

  bool isDebugEnabled () {
  #if defined(DEBUG) && DEBUG
    return true;
  #endif
    return false;
  }

  const char* getDevHost () {
    static const char* host = STR_VALUE(HOST);
    return host;
  }

  int getDevPort () {
    return PORT;
  }
}
