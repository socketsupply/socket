#include "extension.hh"

bool sapi_ipc_router_map (
  sapi_context_t* ctx,
  const char* name,
  sapi_ipc_router_message_callback_t callback,
  const void* data
) {
  if (
    ctx == nullptr ||
    ctx->router == nullptr ||
    ctx->state > ssc::extension::Extension::Context::State::Init
  ) {
    return false;
  }

  if (!ctx->isAllowed("ipc_router_map")) {
    sapi_debug(ctx, "'ipc_router_map' is not allowed.");
    return false;
  }

  ctx->router->map(name, [ctx, data, callback](
    auto message,
    auto router,
    auto reply
  ) mutable {
    auto context = sapi_context_create(ctx, true);
    if (context == nullptr) {
      return;
    }

    context->data = data;
    context->internal = new ssc::runtime::ipc::Router::ReplyCallback(reply);
    callback(
      context,
      (sapi_ipc_message_t*) &message,
      reinterpret_cast<const sapi_ipc_router_t*>(&router)
    );
  });

  return true;
}

bool sapi_ipc_router_unmap (sapi_context_t* ctx, const char* name) {
  if (ctx == nullptr || ctx->router == nullptr) {
    return false;
  }

  if (!ctx->isAllowed("ipc_router_unmap")) {
    sapi_debug(ctx, "'ipc_router_unmap' is not allowed.");
    return false;
  }

  ctx->router->unmap(name);

  return true;
}

uint64_t sapi_ipc_router_listen (
  sapi_context_t* ctx,
  const char* name,
  sapi_ipc_router_message_callback_t callback,
  const void* data
) {
  if (ctx == nullptr || ctx->router == nullptr || name == nullptr) {
    return 0;
  }

  if (!ctx->isAllowed("ipc_router_listen")) {
    sapi_debug(ctx, "'ipc_router_listen' is not allowed.");
    return 0;
  }

  return ctx->router->listen(name, [data, callback](
    auto message,
    auto router,
    auto reply
  ) mutable {
    sapi_context_t context;
    context.router = router;
    context.data = data;
    callback(
      &context,
      (sapi_ipc_message_t*)(&message),
      reinterpret_cast<const sapi_ipc_router_t*>(&router)
    );
  });
}

bool sapi_ipc_router_unlisten (
  sapi_context_t* ctx,
  const char* name,
  uint64_t token
) {
  if (ctx == nullptr || ctx->router == nullptr || name == nullptr) {
    return false;
  }

  if (!ctx->isAllowed("ipc_router_unlisten")) {
    sapi_debug(ctx, "'ipc_router_unlisten' is not allowed.");
    return false;
  }

  return ctx->router->unlisten(name, token);
}

bool sapi_ipc_reply_with_error (sapi_ipc_result_t* result, const char* error) {
  sapi_context* context = sapi_ipc_result_get_context(result);
  sapi_json_string* errorJson = sapi_json_string_create(context, error);
  sapi_json_object* errorObject = sapi_json_object_create(context);
  sapi_json_object_set(errorObject, "message", errorJson);
  sapi_ipc_result_set_json_error(result, ((sapi_json_any*)errorObject));
  return sapi_ipc_reply(result);
}

bool sapi_ipc_reply (const sapi_ipc_result_t* result) {
  if (result == nullptr) return false;
  if (result->context == nullptr) return false;

  if (!result->context->isAllowed("ipc_router_reply")) {
    sapi_debug(result->context, "'ipc_router_reply' is not allowed.");
    return 0;
  }

  auto success = false;
  auto context = result->context;
  auto internal = context->internal;
  auto fn = reinterpret_cast<ssc::runtime::ipc::Router::ReplyCallback*>(internal);

  if (fn != nullptr) {
    (*fn)(*reinterpret_cast<const ssc::runtime::ipc::Result*>(result));
    success = true;
    delete fn;
  }

  // if retained, then then caller must eventually call `sapi_context_release()`
  if (context->release()) {
    delete context;
  }

  return success;
}

bool sapi_ipc_set_cancellation_handler (
  sapi_ipc_result_t* result,
  void (*handler)(void*),
  void* data
) {
  if (result == nullptr || result->message.cancel == nullptr) {
    return false;
  }
  auto message = result->message;
  *message.cancel = {
    .handler = handler,
    .data = data
  };
  return true;
}

bool sapi_ipc_send_chunk (
  sapi_ipc_result_t* result,
  const unsigned char* chunk,
  size_t chunk_size,
  bool finished
) {
  if (result == nullptr) {
    return false;
  }
  if (!result->message.isHTTP) {
    std::string error =
        "IPC method '" + result->message.name + "' must be invoked with HTTP";
    return sapi_ipc_reply_with_error(result, error.c_str());
  }
  auto send_chunk_ptr = result->queuedResponse.chunkStreamCallback;
  if (send_chunk_ptr == nullptr) {
    debug(
        "Cannot use 'sapi_ipc_send_chunk' before setting the \"Transfer-Encoding\""
        " header to \"chunked\"");
    return false;
  }
  bool success = (*send_chunk_ptr)(chunk, chunk_size, finished);
  if (finished) {
    auto context = result->context;
    if (context->release()) {
      delete context;
    }
  }
  return success;
}

bool sapi_ipc_send_event (
  sapi_ipc_result_t* result,
  const char* name,
  const unsigned char* data,
  bool finished
) {
  if (result == nullptr) {
    return false;
  }
  if (!result->message.isHTTP) {
    std::string error =
        "IPC method '" + result->message.name + "' must be invoked with HTTP";
    return sapi_ipc_reply_with_error(result, error.c_str());
  }
  auto send_event_ptr = result->queuedResponse.eventStreamCallback;
  if (send_event_ptr == nullptr) {
    debug(
        "Cannot use 'sapi_ipc_send_event' before setting the \"Content-Type\""
        " header to \"text/event-stream\"");
    return false;
  }
  bool success = (*send_event_ptr)(name, data, finished);
  if (finished) {
    auto context = result->context;
    if (context->release()) {
      delete context;
    }
  }
  return success;
}

bool sapi_ipc_send_bytes (
  sapi_context_t* ctx,
  sapi_ipc_message_t* message,
  unsigned int size,
  unsigned char* bytes,
  const char* headers
) {
  if (!ctx || !ctx->router || !bytes || !size) {
    return false;
  }

  auto queuedResponse = ssc::runtime::QueuedResponse {
    .id = 0,
    .ttl = 0,
    .body = nullptr,
    .length = size,
    .headers = ssc::runtime::String(headers ? headers : "")
  };

  if (bytes != nullptr && size > 0) {
    queuedResponse.body = std::make_shared<unsigned char[]>(size);
    memcpy(queuedResponse.body.get(), bytes, size);
  }

  if (message) {
    auto result = ssc::runtime::ipc::Result(
      message->seq,
      *message,
      ssc::runtime::JSON::null,
      queuedResponse
    );

    return ctx->router->bridge.send(result.seq, result.str(), result.queuedResponse);
  }

  auto result = ssc::runtime::ipc::Result(ssc::runtime::JSON::null);
  return ctx->router->bridge.send(result.seq, result.str(), queuedResponse);
}

bool sapi_ipc_send_bytes_with_result (
  sapi_context_t* ctx,
  sapi_ipc_result_t* result,
  unsigned int size,
  unsigned char* bytes,
  const char* headers
) {
  if (!ctx || !ctx->router || !bytes || !size || !result) {
    return false;
  }

  auto queuedResponse = ssc::runtime::QueuedResponse {
    .id = 0,
    .ttl = 0,
    .body = nullptr,
    .length = size,
    .headers = ssc::runtime::String(headers ? headers : "")
  };

  if (bytes != nullptr && size > 0) {
    queuedResponse.body = std::make_shared<unsigned char[]>(size);
    memcpy(queuedResponse.body.get(), bytes, size);
  }

  return ctx->router->bridge.send(result->seq, result->str(), queuedResponse);
}

bool sapi_ipc_send_json (
  sapi_context_t* ctx,
  sapi_ipc_message_t* message,
  sapi_json_any_t* json
) {
  ssc::runtime::JSON::Any value = nullptr;

  if (!ctx || !ctx->router || !json) {
    return false;
  }

  if (json->type > ssc::runtime::JSON::Type::Any) {
    if (json->isObject()) {
      auto object = reinterpret_cast<const ssc::runtime::JSON::Object*>(json);
      value = ssc::runtime::JSON::Object(object->data);
    } else if (json->isArray()) {
      auto array = reinterpret_cast<const ssc::runtime::JSON::Array*>(json);
      value = ssc::runtime::JSON::Array(array->data);
    } else if (json->isString()) {
      auto string = reinterpret_cast<const ssc::runtime::JSON::String*>(json);
      value = ssc::runtime::JSON::String(string->data);
    } else if (json->isBoolean()) {
      auto boolean = reinterpret_cast<const ssc::runtime::JSON::Boolean*>(json);
      value = ssc::runtime::JSON::Boolean(boolean->data);
    } else if (json->isNumber()) {
      auto number = reinterpret_cast<const ssc::runtime::JSON::Number*>(json);
      value = ssc::runtime::JSON::Number(number->data);
    } else if (json->isRaw()) {
      auto raw = reinterpret_cast<const ssc::runtime::JSON::Raw*>(json);
      value = ssc::runtime::JSON::Raw(raw->data);
    }
  } else {
    value = nullptr;
  }

  if (message) {
    auto result = ssc::runtime::ipc::Result(
      message->seq,
      *message,
      value
    );

    return ctx->router->bridge.send(result.seq, result.str(), result.queuedResponse);
  }

  auto result = ssc::runtime::ipc::Result(value);
  return ctx->router->bridge.send(result.seq, result.str(), result.queuedResponse);
}

bool sapi_ipc_send_json_with_result (
  sapi_context_t* ctx,
  sapi_ipc_result_t* result,
  sapi_json_any_t* json
) {
  ssc::runtime::JSON::Any value = nullptr;

  if (!ctx || !ctx->router || !result || !json) {
    return false;
  }

  if (json->type > ssc::runtime::JSON::Type::Any) {
    if (json->isObject()) {
      auto object = reinterpret_cast<const ssc::runtime::JSON::Object*>(json);
      value = ssc::runtime::JSON::Object(object->data);
    } else if (json->isArray()) {
      auto array = reinterpret_cast<const ssc::runtime::JSON::Array*>(json);
      value = ssc::runtime::JSON::Array(array->data);
    } else if (json->isString()) {
      auto string = reinterpret_cast<const ssc::runtime::JSON::String*>(json);
      value = ssc::runtime::JSON::String(string->data);
    } else if (json->isBoolean()) {
      auto boolean = reinterpret_cast<const ssc::runtime::JSON::Boolean*>(json);
      value = ssc::runtime::JSON::Boolean(boolean->data);
    } else if (json->isNumber()) {
      auto number = reinterpret_cast<const ssc::runtime::JSON::Number*>(json);
      value = ssc::runtime::JSON::Number(number->data);
    } else if (json->isRaw()) {
      auto raw = reinterpret_cast<const ssc::runtime::JSON::Raw*>(json);
      value = ssc::runtime::JSON::Raw(raw->data);
    }
  } else {
    value = nullptr;
  }

  auto res = ssc::runtime::ipc::Result(result->seq, result->message, value);
  return ctx->router->bridge.send(res.seq, res.str(), res.queuedResponse);
}

bool sapi_ipc_emit (
  sapi_context_t* ctx,
  const char* name,
  const char* data
) {
  return ctx && ctx->router ? ctx->router->bridge.emit(name, ssc::runtime::String(data)) : false;
}

bool sapi_ipc_invoke (
  sapi_context_t* ctx,
  const char* url,
  unsigned int size,
  const unsigned char* bytes,
  sapi_ipc_router_result_callback_t callback
) {
  if (ctx == nullptr || ctx->router == nullptr) return false;
  auto uri = ssc::runtime::String(url);

  if (!uri.starts_with("ipc://")) {
    uri = "ipc://" + uri;
  }

  ssc::runtime::SharedPointer<unsigned char[]> data = nullptr;

  if (bytes != nullptr && size > 0) {
    data.reset(new unsigned char[size]{0});
    memcpy(data.get(), bytes, size);
  }

  return ctx->router->invoke(uri, data, size, [ctx, callback](auto result) {
    callback(
      reinterpret_cast<const sapi_ipc_result_t*>(&result),
      reinterpret_cast<const sapi_ipc_router_t*>(&ctx->router)
    );
  });
}

sapi_ipc_result_t* sapi_ipc_result_from_json (
  sapi_context_t* ctx,
  sapi_ipc_message_t* message,
  const sapi_json_any_t* json
) {
  if (ctx == nullptr) return nullptr;
  auto result= ctx->memory.alloc<sapi_ipc_result_t>(ctx);
  result->context = ctx;

  if (json) sapi_ipc_result_set_json(result, json);
  if (message) {
    sapi_ipc_result_set_seq(result, message->seq.c_str());
    sapi_ipc_result_set_message(result, message);
  }

  return result;
}

sapi_ipc_result_t* sapi_ipc_result_create (
  sapi_context_t* ctx,
  sapi_ipc_message_t* message
) {
  if (ctx == nullptr) return nullptr;
  auto result= ctx->memory.alloc<sapi_ipc_result_t>(ctx);
  if (message) {
    sapi_ipc_result_set_seq(result, message->seq.c_str());
    sapi_ipc_result_set_message(result, message);
  }
  return result;
}

int sapi_ipc_message_get_index (const sapi_ipc_message_t* message) {
  return message ? message->index : -1;
}

const char* sapi_ipc_message_get_value (const sapi_ipc_message_t* message) {
  if (message == nullptr || message->value.size() == 0) return nullptr;
  return message->value.c_str();
}

const char* sapi_ipc_message_get_name (const sapi_ipc_message_t* message) {
  if (message == nullptr || message->name.size() == 0) return nullptr;
  return message->name.c_str();
}

const char* sapi_ipc_message_get_seq (const sapi_ipc_message_t* message) {
  if (message == nullptr || message->seq.size() == 0) return nullptr;
  return message->seq.c_str();
}

const char* sapi_ipc_message_get_uri (const sapi_ipc_message_t* message) {
  if (message->uri.size() == 0) return nullptr;
  return message->uri.c_str();
}

const char* sapi_ipc_message_get (
  const sapi_ipc_message_t* message,
  const char* key
) {
  if (!message || !key || !message->contains(key)) return nullptr;
  const auto& value = message->at(key);
  if (value.size() == 0) return nullptr;
  return value.c_str();
}

sapi_ipc_message_t* sapi_ipc_message_clone (
  sapi_context_t* context,
  const sapi_ipc_message_t* message
) {
  if (message == nullptr) return nullptr;
  if (context == nullptr) return nullptr;

  return context->memory.alloc<sapi_ipc_message_t>(*message);
}

sapi_ipc_result_t* sapi_ipc_result_clone (
  sapi_context_t* context,
  const sapi_ipc_result_t* result
) {
  if (result == nullptr) return nullptr;
  if (context == nullptr) return nullptr;

  return context->memory.alloc<sapi_ipc_result_t>(context, *result);
}

const unsigned char* sapi_ipc_message_get_bytes (
  const sapi_ipc_message_t* message
) {
  if (!message) return nullptr;
  return message->buffer.data();
}

unsigned int sapi_ipc_message_get_bytes_size (
  const sapi_ipc_message_t* message
) {
  if (!message || !message->buffer.data()) return 0;
  return static_cast<unsigned int>(message->buffer.size());
}

void sapi_ipc_result_set_seq (sapi_ipc_result_t* result, const char* seq) {
  if (result && seq) {
    result->seq = seq;
  }
}

const char* sapi_ipc_result_get_seq (const sapi_ipc_result_t* result) {
  return result ? result->seq.c_str() : nullptr;
}

void sapi_ipc_result_set_message (
  sapi_ipc_result_t* result,
  sapi_ipc_message_t* message
) {
  if (result && message) {
    result->message = *message;
    result->source = message->name;
    result->seq = message->seq;
  }
}

const sapi_ipc_message_t* sapi_ipc_result_get_message (
  const sapi_ipc_result_t* result
) {
  return result
    ? reinterpret_cast<const sapi_ipc_message_t*>(&result->message)
    : nullptr;
}

sapi_context_t* sapi_ipc_result_get_context (
  const sapi_ipc_result_t* result
) {
  return result ? result->context : nullptr;
}

void sapi_ipc_result_set_json (
  sapi_ipc_result_t* result,
  const sapi_json_any_t* json
) {
  if (result == nullptr) return;
  if (json == nullptr) {
    result->value = nullptr;
  } else if (json->type > ssc::runtime::JSON::Type::Any) {
    if (json->isObject()) {
      auto object = reinterpret_cast<const ssc::runtime::JSON::Object*>(json);
      result->value = ssc::runtime::JSON::Object(object->data);
    } else if (json->isArray()) {
      auto array = reinterpret_cast<const ssc::runtime::JSON::Array*>(json);
      result->value = ssc::runtime::JSON::Array(array->data);
    } else if (json->isString()) {
      auto string = reinterpret_cast<const ssc::runtime::JSON::String*>(json);
      result->value = ssc::runtime::JSON::String(string->data);
    } else if (json->isBoolean()) {
      auto boolean = reinterpret_cast<const ssc::runtime::JSON::Boolean*>(json);
      result->value = ssc::runtime::JSON::Boolean(boolean->data);
    } else if (json->isNumber()) {
      auto number = reinterpret_cast<const ssc::runtime::JSON::Number*>(json);
      result->value = ssc::runtime::JSON::Number(number->data);
    } else if (json->isRaw()) {
      auto raw = reinterpret_cast<const ssc::runtime::JSON::Raw*>(json);
      result->value = ssc::runtime::JSON::Raw(raw->data);
    }
  }
}

const sapi_json_any_t* sapi_ipc_result_get_json (
  const sapi_ipc_result_t* result
) {
  return result
    ? reinterpret_cast<const sapi_json_any_t*>(&result->value)
    : nullptr;
}

void sapi_ipc_result_set_json_data (
  sapi_ipc_result_t* result,
  const sapi_json_any_t* json
) {
  if (result == nullptr) return;
  if (json == nullptr) {
    result->data = nullptr;
  } else if (json->type > ssc::runtime::JSON::Type::Any) {
    if (json->isObject()) {
      auto object = reinterpret_cast<const ssc::runtime::JSON::Object*>(json);
      result->data = ssc::runtime::JSON::Object(object->data);
    } else if (json->isArray()) {
      auto array = reinterpret_cast<const ssc::runtime::JSON::Array*>(json);
      result->data = ssc::runtime::JSON::Array(array->data);
    } else if (json->isString()) {
      auto string = reinterpret_cast<const ssc::runtime::JSON::String*>(json);
      result->data = ssc::runtime::JSON::String(string->data);
    } else if (json->isBoolean()) {
      auto boolean = reinterpret_cast<const ssc::runtime::JSON::Boolean*>(json);
      result->data = ssc::runtime::JSON::Boolean(boolean->data);
    } else if (json->isNumber()) {
      auto number = reinterpret_cast<const ssc::runtime::JSON::Number*>(json);
      result->data = ssc::runtime::JSON::Number(number->data);
    } else if (json->isRaw()) {
      auto raw = reinterpret_cast<const ssc::runtime::JSON::Raw*>(json);
      result->data = ssc::runtime::JSON::Raw(raw->data);
    }
  }
}

const sapi_json_any_t* sapi_ipc_result_get_json_data (
  const sapi_ipc_result_t* result
) {
  return result
    ? reinterpret_cast<const sapi_json_any_t*>(&result->data)
    : nullptr;
}

void sapi_ipc_result_set_json_error (
  sapi_ipc_result_t* result,
  const sapi_json_any_t* json
) {
  if (result == nullptr) return;
  if (json == nullptr) {
    result->err = nullptr;
  } else if (json->type > ssc::runtime::JSON::Type::Any) {
    if (json->isObject()) {
      auto object = reinterpret_cast<const ssc::runtime::JSON::Object*>(json);
      result->err = ssc::runtime::JSON::Object(object->data);
    } else if (json->isArray()) {
      auto array = reinterpret_cast<const ssc::runtime::JSON::Array*>(json);
      result->err = ssc::runtime::JSON::Array(array->data);
    } else if (json->isString()) {
      auto string = reinterpret_cast<const ssc::runtime::JSON::String*>(json);
      result->err = ssc::runtime::JSON::String(string->data);
    } else if (json->isBoolean()) {
      auto boolean = reinterpret_cast<const ssc::runtime::JSON::Boolean*>(json);
      result->err = ssc::runtime::JSON::Boolean(boolean->data);
    } else if (json->isNumber()) {
      auto number = reinterpret_cast<const ssc::runtime::JSON::Number*>(json);
      result->err = ssc::runtime::JSON::Number(number->data);
    } else if (json->isRaw()) {
      auto raw = reinterpret_cast<const ssc::runtime::JSON::Raw*>(json);
      result->err  = ssc::runtime::JSON::Raw(raw->data);
    }
  }
}

const sapi_json_any_t* sapi_ipc_result_get_json_error (
  const sapi_ipc_result_t* result
) {
  return result
    ? reinterpret_cast<const sapi_json_any_t*>(&result->err)
    : nullptr;
}

void sapi_ipc_result_set_bytes (
  sapi_ipc_result_t* result,
  unsigned int size,
  unsigned char* bytes
) {
  if (result && size && bytes) {
    result->queuedResponse.length = size;
    result->queuedResponse.body = std::make_shared<unsigned char[]>(size);
    memcpy(result->queuedResponse.body.get(), bytes, size);
  }
}

unsigned char* sapi_ipc_result_get_bytes (
  const sapi_ipc_result_t* result
) {
  return result
    ? reinterpret_cast<unsigned char*>(result->queuedResponse.body.get())
    : nullptr;
}

size_t sapi_ipc_result_get_bytes_size (
  const sapi_ipc_result_t* result
) {
  return result ? result->queuedResponse.length : 0;
}

void sapi_ipc_result_set_header (
  sapi_ipc_result_t* result,
  const char* name,
  const char* value
) {
  if (result && name && value) {
    result->headers.set(name, value);

    if (result->headers.get("content-type") == "text/event-stream") {
      result->context->retain();
      result->queuedResponse = ssc::runtime::QueuedResponse();
      result->queuedResponse.eventStreamCallback = std::make_shared<ssc::runtime::QueuedResponse::EventStreamCallback>(
        [result](const char* name, const unsigned char* data, bool finished) {
          return false;
        }
      );
    } else if (result->headers.get("transfer-encoding") == "chunked") {
      result->context->retain();
      result->queuedResponse = ssc::runtime::QueuedResponse();
      result->queuedResponse.chunkStreamCallback = std::make_shared<ssc::runtime::QueuedResponse::ChunkStreamCallback>(
        [result](const unsigned char* chunk, size_t chunk_size, bool finished) {
          return false;
        }
      );
    }
  }
}

const char* sapi_ipc_result_get_header (
  const sapi_ipc_result_t* result,
  const char* name
) {
  if (result && result->headers.has(name)) {
    return result->headers.get(name).value.string.c_str();
  }

  return nullptr;
}
