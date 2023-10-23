#include "core/config.hh"
#include "core/string.hh"
#include "core/types.hh"
#include "core/ini.hh"

#if defined(__cplusplus)
// These rely on project-specific, compile-time variables.
namespace SSC {
  bool isDebugEnabled () {
    return DEBUG == 1;
  }

  const Config& getUserConfig () {
    #include "user-config-bytes.hh" // NOLINT
                                    //
    static const auto config = Config(std::string(
      (const char*) __ssc_config_bytes,
      sizeof(__ssc_config_bytes)
    ));

    return config;
  }

  const char* getDevHost () {
    static const char* host = CONVERT_TO_STRING(HOST);
    return host;
  }

  int getDevPort () {
    return PORT;
  }
}
#endif
