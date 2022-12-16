#include "common.hh"

// These rely on project-specific, compile-time variables.
namespace SSC {
  const Map getSettingsSource () {
    #include "ini.hh" // NOLINT
    return parseConfig(ini);
  }

  const char* getDevHost () {
    static const char* host = STR_VALUE(HOST);
    return host;
  }

  int getDevPort () {
    return PORT;
  }
}
