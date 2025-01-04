#include "extension.hh"

sapi_context_t* sapi_context_create (
  sapi_context_t* parent,
  bool retained
) {
  if (parent && !parent->isAllowed("context_create")) {
    sapi_debug(parent, "'context_create' is not allowed.");
    return nullptr;
  }

  retained = retained || parent == nullptr;
  auto context = retained
    ? new sapi_context_t(parent)
    : parent->memory.alloc<sapi_context_t>(parent, parent);

  if (retained || parent == nullptr) {
    context->retain();
  }

  return context;
}

bool sapi_context_set_data (
  sapi_context_t* ctx,
  const void* data
) {
  if (ctx == nullptr) return false;
  ctx->data = data;
  return true;
}

bool sapi_context_dispatch (
  sapi_context_t* ctx,
  const void* data,
  sapi_context_dispatch_callback callback
) {
  if (ctx == nullptr) return false;
  if (ctx->router == nullptr) return false;

  if (!ctx->isAllowed("context_dispatch")) {
    sapi_debug(ctx, "'context_dispatch' is not allowed.");
    return false;
  }

  return ctx->router->bridge.dispatch([=]() {
    callback(ctx, data);
  });
}

void sapi_context_retain (sapi_context_t* ctx) {
  if (ctx == nullptr) return;
  if (!ctx->isAllowed("context_retain")) {
    sapi_debug(ctx, "'context_retain' is not allowed.");
    return;
  }

  ctx->retain();
}

bool sapi_context_retained (const sapi_context_t* ctx) {
  if (ctx == nullptr) return false;
  return ctx->retain_count > 0;
}

void sapi_context_release (sapi_context_t* ctx) {
  if (ctx == nullptr) return;
  if (!ctx->isAllowed("context_release")) {
    sapi_debug(ctx, "'context_release' is not allowed.");
    return;
  }
  if (ctx->release()) {
    delete ctx;
  }
}

uv_loop_t* sapi_context_get_loop (const sapi_context_t* ctx) {
  if (ctx == nullptr) return nullptr;
  if (ctx->router == nullptr) return nullptr;
  if (!ctx->isAllowed("context_get_loop")) {
    sapi_debug(ctx, "'context_get_loop' is not allowed.");
    return nullptr;
  }

  return ctx->router->bridge.context.loop.get();
}

const sapi_ipc_router_t* sapi_context_get_router (const sapi_context_t* ctx) {
  if (ctx == nullptr) return nullptr;
  if (ctx->router == nullptr) return nullptr;
  if (!ctx->isAllowed("context_get_router")) {
    sapi_debug(ctx, "'context_get_router' is not allowed.");
    return nullptr;
  }
  return reinterpret_cast<const sapi_ipc_router_t*>(ctx->router);
}

const void * sapi_context_get_data (const sapi_context_t* context) {
  return context != nullptr ? context->data : nullptr;
}

const sapi_context_t* sapi_context_get_parent (const sapi_context_t* context) {
  return context != nullptr ? reinterpret_cast<sapi_context_t*>(context->context) : nullptr;
}

void sapi_context_error_reset (sapi_context_t* context) {
  if (context == nullptr) return;
  context->error.code = 0;
  context->error.name = "";
  context->error.message = "";
  context->error.location = "";
}

void sapi_context_error_set_code (
  sapi_context_t* context,
  int code
) {
  if (context == nullptr) return;
  context->error.code = code;
  context->state = ssc::extension::Extension::Context::State::Error;
}

int sapi_context_error_get_code (const sapi_context_t* context) {
  if (context == nullptr) return -1;
  return context->error.code;
}

void sapi_context_error_set_name (
  sapi_context_t* context,
  const char* name
) {
  if (context == nullptr) return;
  context->error.name = name;
  context->state = ssc::extension::Extension::Context::State::Error;
}

const char* sapi_context_error_get_name (const sapi_context_t* context) {
  if (context == nullptr) return nullptr;
  return context->error.name.c_str();
}

void sapi_context_error_set_message (
  sapi_context_t* context,
  const char* message
) {
  if (context == nullptr) return;
  context->error.message = message;
  context->state = ssc::extension::Extension::Context::State::Error;
}

const char* sapi_context_error_get_message (const sapi_context_t* context) {
  if (context == nullptr) return nullptr;
  return context->error.message.c_str();
}

void sapi_context_error_set_location (
  sapi_context_t* context,
  const char* location
) {
  if (context == nullptr) return;
  context->error.location = location;
  context->state = ssc::extension::Extension::Context::State::Error;
}

const char* sapi_context_error_get_location (const sapi_context_t* context) {
  if (context == nullptr) return nullptr;
  return context->error.location.c_str();
}

const char * sapi_context_config_get (
  const sapi_context_t* context,
  const char* key
) {
  if (context == nullptr || key == nullptr) return nullptr;
  if (!context->config.contains(key)) return nullptr;
  return context->config.at(key).c_str();
}

void sapi_context_config_set (
  sapi_context_t* context,
  const char* key,
  const char* value
) {
  if (context == nullptr || key == nullptr) return;
  context->config[key] = value;
}

void* sapi_context_alloc (sapi_context_t* context, unsigned int size) {
  if (context == nullptr || size == 0) {
    return nullptr;
  }

  return reinterpret_cast<void*>(context->memory.alloc<unsigned char>(size));
}
