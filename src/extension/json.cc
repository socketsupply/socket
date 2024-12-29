#include <string.h>
#include "extension.hh"

const sapi_json_type_t sapi_json_typeof (const sapi_json_any_t* json) {
  if (json->isNull()) return SAPI_JSON_TYPE_NULL;
  if (json->isObject()) return SAPI_JSON_TYPE_OBJECT;
  if (json->isArray()) return SAPI_JSON_TYPE_ARRAY;
  if (json->isBoolean()) return SAPI_JSON_TYPE_BOOLEAN;
  if (json->isNumber()) return SAPI_JSON_TYPE_NUMBER;
  if (json->isString()) return SAPI_JSON_TYPE_STRING;
  if (json->isEmpty()) return SAPI_JSON_TYPE_EMPTY;
  if (json->isRaw()) return SAPI_JSON_TYPE_RAW;
  return SAPI_JSON_TYPE_ANY;
}

sapi_json_object_t* sapi_json_object_create (sapi_context_t* ctx) {
  return ctx->memory.alloc<sapi_json_object_t>(ctx);
}

sapi_json_array_t* sapi_json_array_create (sapi_context_t* ctx) {
  return ctx->memory.alloc<sapi_json_array_t>(ctx);
}

sapi_json_string_t* sapi_json_string_create (
  sapi_context_t* ctx,
  const char* string
) {
  return ctx->memory.alloc<sapi_json_string_t>(ctx, string);
}

sapi_json_boolean_t* sapi_json_boolean_create (
  sapi_context_t* ctx,
  bool boolean
) {
  return ctx->memory.alloc<sapi_json_boolean_t>(ctx, boolean);
}

sapi_json_number_t* sapi_json_number_create (
  sapi_context_t* ctx,
  const double number
 ) {
  return ctx->memory.alloc<sapi_json_number_t>(ctx, number);
}

sapi_json_any_t* sapi_json_raw_from (
  sapi_context_t* ctx,
  const char* source
) {
  return reinterpret_cast<sapi_json_any_t*>(
    ctx->memory.alloc<sapi_json_raw_t>(ctx, source)
  );
}

const char * sapi_json_stringify_value (const sapi_json_any_t* json) {
  SSC::String string;
  switch (sapi_json_typeof(json)) {
    case SAPI_JSON_TYPE_NULL:
      string = "null";
      break;

    case SAPI_JSON_TYPE_OBJECT:
      string = reinterpret_cast<const SSC::JSON::Object*>(json)->str();
      break;

    case SAPI_JSON_TYPE_ARRAY:
      string = reinterpret_cast<const SSC::JSON::Array*>(json)->str();
      break;
    case SAPI_JSON_TYPE_BOOLEAN:
      string = reinterpret_cast<const SSC::JSON::Boolean*>(json)->str();
      break;
    case SAPI_JSON_TYPE_NUMBER:
      string = reinterpret_cast<const SSC::JSON::Number*>(json)->str();
      break;
    case SAPI_JSON_TYPE_STRING:
      string = reinterpret_cast<const SSC::JSON::String*>(json)->str();
      break;
    case SAPI_JSON_TYPE_RAW:
      string = reinterpret_cast<const SSC::JSON::Raw*>(json)->str();
      break;

    case SAPI_JSON_TYPE_EMPTY:
    case SAPI_JSON_TYPE_ANY:
      break;
  }

  size_t length = string.size();

  if (length > 0) {
    auto bytes = json->context->memory.alloc<char>(length + 1);
    if (bytes != nullptr) {
    #if defined(_WIN32)
      strncat_s(bytes, length + 1, string.c_str(), length);
    #else
      strncat(bytes, string.c_str(), length);
    #endif
    }

    return bytes;
  }

  return nullptr;
}

void sapi_json_object_set_value (
  sapi_json_object_t* json,
  const char* key,
  sapi_json_any_t* any
) {
  if (json == nullptr || key == nullptr) return;

  if (any == nullptr) {
    json->set(key, SSC::JSON::Null());
  } else if (any->type > SSC::JSON::Type::Any) {
    if (any->isObject()) {
      auto object = reinterpret_cast<SSC::JSON::Object*>(any);
      json->set(key, SSC::JSON::Object(object->data));
    } else if (any->isArray()) {
      auto array = reinterpret_cast<SSC::JSON::Array*>(any);
      json->set(key, SSC::JSON::Array(array->data));
    } else if (any->isString()) {
      auto string = reinterpret_cast<SSC::JSON::String*>(any);
      json->set(key, SSC::JSON::String(string->data));
    } else if (any->isBoolean()) {
      auto boolean = reinterpret_cast<SSC::JSON::Boolean*>(any);
      json->set(key, SSC::JSON::Boolean(boolean->data));
    } else if (any->isNumber()) {
      auto number = reinterpret_cast<SSC::JSON::Number*>(any);
      json->set(key, SSC::JSON::Number(number->data));
    } else if (any->isRaw()) {
      auto raw= reinterpret_cast<SSC::JSON::Raw*>(any);
      json->set(key, SSC::JSON::Raw(raw->data));
    }
  }
}

sapi_json_any_t* sapi_json_object_get (
  const sapi_json_object_t* json,
  const char* key
) {
  if (json->has(key)) {
    auto pointer = json->data.at(key).data.get();
    return reinterpret_cast<sapi_json_any_t*>(pointer);
  }

  return nullptr;
}

void sapi_json_array_set_value (
  sapi_json_array_t* json,
  unsigned int index,
  sapi_json_any_t* any
) {
  if (json == nullptr || any == nullptr) return;

  if (any == nullptr) {
    json->set(index, SSC::JSON::Null());
  } else if (any->type > SSC::JSON::Type::Any) {
    if (any->isObject()) {
      auto object = reinterpret_cast<SSC::JSON::Object*>(any);
      json->set(index, SSC::JSON::Object(object->data));
    } else if (any->isArray()) {
      auto array = reinterpret_cast<SSC::JSON::Array*>(any);
      json->set(index, SSC::JSON::Array(array->data));
    } else if (any->isString()) {
      auto string = reinterpret_cast<SSC::JSON::String*>(any);
      json->set(index, SSC::JSON::String(string->data));
    } else if (any->isBoolean()) {
      auto boolean = reinterpret_cast<SSC::JSON::Boolean*>(any);
      json->set(index, SSC::JSON::Boolean(boolean->data));
    } else if (any->isNumber()) {
      auto number = reinterpret_cast<SSC::JSON::Number*>(any);
      json->set(index, SSC::JSON::Number(number->data));
    } else if (any->isRaw()) {
      auto raw= reinterpret_cast<SSC::JSON::Raw*>(any);
      json->set(index, SSC::JSON::Raw(raw->data));
    }
  }
}

void sapi_json_array_push_value (
  sapi_json_array_t* json,
  sapi_json_any_t* any
) {
  if (json == nullptr || any == nullptr) return;

  if (any == nullptr) {
    json->push(SSC::JSON::Null());
  } else if (any->type > SSC::JSON::Type::Any) {
    if (any->isObject()) {
      auto object = reinterpret_cast<SSC::JSON::Object*>(any);
      json->push(SSC::JSON::Object(object->data));
    } else if (any->isArray()) {
      auto array = reinterpret_cast<SSC::JSON::Array*>(any);
      json->push(SSC::JSON::Array(array->data));
    } else if (any->isString()) {
      auto string = reinterpret_cast<SSC::JSON::String*>(any);
      json->push(SSC::JSON::String(string->data));
    } else if (any->isBoolean()) {
      auto boolean = reinterpret_cast<SSC::JSON::Boolean*>(any);
      json->push(SSC::JSON::Boolean(boolean->data));
    } else if (any->isNumber()) {
      auto number = reinterpret_cast<SSC::JSON::Number*>(any);
      json->push(SSC::JSON::Number(number->data));
    } else if (any->isRaw()) {
      auto raw= reinterpret_cast<SSC::JSON::Raw*>(any);
      json->push(SSC::JSON::Raw(raw->data));
    }
  }
}

sapi_json_any_t* sapi_json_array_get (
  const sapi_json_array_t* json,
  unsigned int index
) {
  if (json->has(index)) {
    auto pointer = json->data.at(index).data.get();
    return reinterpret_cast<sapi_json_any_t*>(pointer);
  }

  return nullptr;
}

sapi_json_any_t* sapi_json_array_pop (
  sapi_json_array_t* json
) {
  auto pointer = json->pop().data.get();
  return reinterpret_cast<sapi_json_any_t*>(pointer);
}
