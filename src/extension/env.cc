#include "extension.hh"

using ssc::runtime::config::getUserConfig;
using ssc::runtime::string::split;

const char* sapi_env_get (
  sapi_context_t* ctx,
  const char* name
) {
  if (ctx == nullptr || name == nullptr)  return nullptr;
  if (!ctx->isAllowed("env_get")) {
    sapi_debug(ctx, "'env_get' is not allowed.");
    return nullptr;
  }

  static const auto userConfig = getUserConfig();

  if (!userConfig.contains("build_env")) {
    return nullptr;
  }

  static const auto allowed = split(userConfig.at("build_env"), ' ');

  if (std::find(allowed.begin(), allowed.end(), name) == allowed.end()) {
    return nullptr;
  }

  auto value = ssc::runtime::env::get(name);
  if (value.size() == 0) {
    return nullptr;
  }

  auto pointer = ctx->memory.alloc<char>(value.size());
  return reinterpret_cast<const char*>(
    memcpy(pointer, value.c_str(), value.size())
  );
}
