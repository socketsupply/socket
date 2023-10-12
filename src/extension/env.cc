#include "extension.hh"

const char* sapi_env_get (
  sapi_context_t* ctx,
  const char* name
) {
  if (ctx == nullptr || name == nullptr)  return nullptr;
  if (!ctx->isAllowed("env_get")) {
    sapi_debug(ctx, "'env_get' is not allowed.");
    return nullptr;
  }

  static const auto userConfig = SSC::getUserConfig();

  if (!userConfig.contains("build_env")) {
    return nullptr;
  }

  static const auto allowed = SSC::split(userConfig.at("build_env"), ' ');

  if (std::find(allowed.begin(), allowed.end(), name) == allowed.end()) {
    return nullptr;
  }

  auto value = SSC::Env::get(name);
  if (value.size() == 0) {
    return nullptr;
  }

  auto pointer = ctx->memory.alloc<char>(value.size());
  return reinterpret_cast<const char*>(
    memcpy(pointer, value.c_str(), value.size())
  );
}
