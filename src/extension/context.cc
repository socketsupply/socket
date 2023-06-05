#include "extension.hh"

sapi_context_t* sapi_context_create (sapi_context_t* parent) {
  auto context = parent == nullptr
    ? new sapi_context_t
    : parent->memory.alloc<sapi_context_t>();

  if (parent == nullptr) {
    context->retained = true;
  } else {
    context->context = parent;
  }

  return context;
}

bool sapi_context_dispatch (
  sapi_context_t* ctx,
  const void* data,
  sapi_context_dispatch_callback callback
) {
  if (ctx == nullptr) return false;
  if (ctx->router == nullptr) return false;
  if (ctx->router->bridge == nullptr) return false;
  if (ctx->router->bridge->core == nullptr) return false;

  return ctx->router->dispatch([&]() {
    ctx->router->bridge->core->dispatchEventLoop([ctx, data, callback] () {
      callback(ctx, data);
    });
  });
}

void sapi_context_retain (sapi_context_t* ctx) {
  if (ctx == nullptr) return;
  ctx->retain();
}

bool sapi_context_retained (const sapi_context_t* ctx) {
  if (ctx == nullptr) return false;
  return ctx->retained;
}

void sapi_context_release (sapi_context_t* ctx) {
  if (ctx == nullptr) return;
  ctx->release();
  delete ctx;
}

uv_loop_t* sapi_context_get_loop (const sapi_context_t* ctx) {
  if (ctx == nullptr) return nullptr;
  if (ctx->router == nullptr) return nullptr;
  if (ctx->router->bridge == nullptr) return nullptr;
  if (ctx->router->bridge->core == nullptr) return nullptr;
  return ctx->router->bridge->core->getEventLoop();
}

const sapi_ipc_router_t* sapi_context_get_router (const sapi_context_t* ctx) {
  if (ctx == nullptr) return nullptr;
  if (ctx->router == nullptr) return nullptr;
  return reinterpret_cast<const sapi_ipc_router_t*>(ctx->router);
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
  const int code
) {
  if (context == nullptr) return;
  context->error.code = code;
  context->state = SSC::Extension::Context::State::Error;
}

const int sapi_context_error_get_code (const sapi_context_t* context) {
  if (context == nullptr) return -1;
  return context->error.code;
}

void sapi_context_error_set_name (
    sapi_context_t* context,
    const char* name
    ) {
  if (context == nullptr) return;
  context->error.name = name;
  context->state = SSC::Extension::Context::State::Error;
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
  context->state = SSC::Extension::Context::State::Error;
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
  context->state = SSC::Extension::Context::State::Error;
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
