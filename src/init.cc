#if defined(__cplusplus)
#include <socket/_user-config-bytes.hh>

extern "C" {

  // implemented in `init.cc`
  const unsigned char* socket_runtime_init_get_user_config_bytes () {
    return __socket_runtime_user_config_bytes;
  }

  unsigned int socket_runtime_init_get_user_config_bytes_size () {
    return sizeof(__socket_runtime_user_config_bytes);
  }

  bool socket_runtime_init_is_debug_enabled () {
  #if DEBUG
    return true;
  #endif
    return false;
  }

  const char* socket_runtime_init_get_dev_host () {
  #if defined(HOST)
    return HOST;
  #endif
    return "";
  }

  int socket_runtime_init_get_dev_port () {
  #if defined(PORT)
    return PORT;
  #endif
    return 0;
  }
}
#endif
