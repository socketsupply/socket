#include "common.hh"

// These rely on project-specific, compile-time variables.
namespace SSC {
  const Map getSettingsSource () {
    #include "ssc.conf" // NOLINT
    return configToMap(config);
  }

  const char* getDevHost () {
    static const char* host = STR_VALUE(HOST);
    return host;
  }

  int getDevPort () {
    return PORT;
  }
}
