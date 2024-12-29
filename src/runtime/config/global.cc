#include "../config.hh"
#include "../ini.hh"

namespace ssc::runtime::config {
  bool isDebugEnabled () {
    return socket_runtime_init_is_debug_enabled();
  }

  const Map<String, String> getUserConfig () {
    const auto bytes = socket_runtime_init_get_user_config_bytes();
    const auto size = socket_runtime_init_get_user_config_bytes_size();
    const auto string = String(reinterpret_cast<const char*>(bytes), size);
    const auto userConfig = INI::parse(string);
    return userConfig;
  }

  const String getDevHost () {
    return socket_runtime_init_get_dev_host();
  }

  int getDevPort () {
    return socket_runtime_init_get_dev_port();
  }
}
