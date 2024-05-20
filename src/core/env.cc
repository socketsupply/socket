#include <stdlib.h>

#include "config.hh"
#include "env.hh"

namespace SSC::Env {
  bool has (const char* name) {
    static auto userConfig = getUserConfig();

    if (userConfig[String("env_") + name].size() > 0) {
      return true;
    }

  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    char* value = nullptr;
    size_t size = 0;
    auto result = _dupenv_s(&value, &size, name);

    if (value && value[0] == '\0') {
      free(value);
      return false;
    }

    free(value);

    if (size == 0 || result != 0) {
      return false;
    }
  #else
    auto value = getenv(name);

    if (value == nullptr || value[0] == '\0') {
      return false;
    }
  #endif

    return true;
  }

  bool has (const String& name) {
    return has(name.c_str());
  }

  String get (const char* name) {
    static auto userConfig = getUserConfig();

    if (userConfig[String("env_") + name].size() > 0) {
      return userConfig[String("env_") + name];
    }

    #if SOCKET_RUNTIME_PLATFORM_WINDOWS
      char* variableValue = nullptr;
      std::size_t valueSize = 0;
      auto query = _dupenv_s(&variableValue, &valueSize, name);

      String result;
      if (query == 0 && variableValue != nullptr && valueSize > 0) {
        result.assign(variableValue, valueSize - 1);
        free(variableValue);
      }

      return result;
    #else
      auto v = getenv(name);

      if (v != nullptr) {
        return String(v);
      }

      return String("");
    #endif
  }

  String get (const String& name) {
    return get(name.c_str());
  }

  String get (const String& name, const String& fallback) {
    const auto value = get(name);

    if (value.size() == 0) {
      return fallback;
    }

    return value;
  }

  void set (const String& name, const String& value) {
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    _putenv((name + "=" + value).c_str());
  #else
    setenv(name.c_str(), value.c_str(), 1);
  #endif
  }

  void set (const char* name) {
  #if SOCKET_RUNTIME_PLATFORM_WINDOWS
    _putenv(name);
  #else
    auto parts = split(String(name), '=');
    set(parts[0], parts[1]);
  #endif
  }
}
