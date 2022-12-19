#include "common.hh"

// These rely on project-specific, compile-time variables.
namespace SSC {
  bool isDebugEnabled () {
    return DEBUG == 1;
  }

  const Map getSettingsSource () {
    #include "user-config-bytes.hh" // NOLINT
    return parseConfig(std::string(
      (const char*) __ssc_config_bytes,
      sizeof(__ssc_config_bytes)
    ));
  }

  const char* getDevHost () {
    static const char* host = STR_VALUE(HOST);
    return host;
  }

  int getDevPort () {
    return PORT;
  }
}
