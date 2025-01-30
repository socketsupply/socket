#include "../javascript.hh"
#include "../runtime.hh"
#include "../crypto.hh"
#include "../string.hh"

#include "../context.hh"

using ssc::runtime::javascript::createJavaScript;
using ssc::runtime::string::trim;
using ssc::runtime::crypto::rand64;

namespace ssc::runtime::context {
#if SOCKET_RUNTIME_PLATFORM_ANDROID
  AndroidContext::AndroidContext (const AndroidContext& context)
    : buildInformation(context.buildInformation),
      contentResolver(context.contentResolver),
      jvm(context.jvm),
      activity(context.activity),
      looper(context.looper),
      self(context.self),
      isEmulator(context.isEmulator.load(std::memory_order_acquire))
  {}

  AndroidContext::AndroidContext (AndroidContext&& context)
    : buildInformation(context.buildInformation),
      contentResolver(context.contentResolver),
      jvm(context.jvm),
      activity(context.activity),
      looper(context.looper),
      self(context.self),
      isEmulator(context.isEmulator.load(std::memory_order_acquire))
  {}

  AndroidContext& AndroidContext::operator = (const AndroidContext& context) {
    this->buildInformation = context.buildInformation;
    this->contentResolver = context.contentResolver;
    this->jvm = context.jvm;
    this->activity = context.activity;
    this->looper = context.looper;
    this->self = context.self;
    this->isEmulator = context.isEmulator.load(std::memory_order_acquire);
    return *this;
  }

  AndroidContext& AndroidContext::operator = (AndroidContext&& context) {
    this->buildInformation = context.buildInformation;
    this->contentResolver = context.contentResolver;
    this->jvm = context.jvm;
    this->activity = context.activity;
    this->looper = context.looper;
    this->self = context.self;
    this->isEmulator = context.isEmulator.load(std::memory_order_acquire);

    context.buildInformation = {};
    context.contentResolver = {};
    context.jvm = nullptr;
    context.activity = {};
    context.looper = {};
    context.self = nullptr;
    context.isEmulator = false;
    return *this;
  }

  void AndroidContext::configure (
    android::JVMEnvironment jvm,
    android::Activity activity
  ) {
    this->jvm = jvm;
    this->activity = activity;
    this->contentResolver.activity = activity;
    this->contentResolver.jvm = jvm;
  }
#endif

  RuntimeContext* RuntimeContext::getRuntimeContext () {
    return this;
  }

  const RuntimeContext* RuntimeContext::getRuntimeContext () const {
    return this;
  }

  Runtime* RuntimeContext::getRuntime () {
    return static_cast<Runtime*>(this);
  }

  const Runtime* RuntimeContext::getRuntime () const {
    return static_cast<const Runtime*>(this);
  }

  String RuntimeContext::createQueuedResponse (
    const String& seq,
    const String& params,
    QueuedResponse queuedResponse
  ) {
    if (queuedResponse.id == 0) {
      queuedResponse.id = rand64();
    }

    const auto script = createJavaScript("queued-response.js",
      "const globals = await import('socket:internal/globals');              \n"
      "const id = `" + std::to_string(queuedResponse.id) + "`;               \n"
      "const seq = `" + seq + "`;                                            \n"
      "const workerId = `" + queuedResponse.workerId + "`.trim() || null;    \n"
      "const headers = `" + trim(queuedResponse.headers.str()) + "`          \n"
      "  .trim()                                                             \n"
      "  .split(/[\\r\\n]+/)                                                 \n"
      "  .filter(Boolean)                                                    \n"
      "  .map((header) => header.trim());                                    \n"
      "                                                                      \n"
      "let params = `" + params + "`;                                        \n"
      "                                                                      \n"
      "try {                                                                 \n"
      "  params = JSON.parse(params);                                        \n"
      "} catch (err) {                                                       \n"
      "  console.error(err.stack || err, params);                            \n"
      "}                                                                     \n"
      "                                                                      \n"
      "globals.get('RuntimeQueuedResponses').dispatch(                       \n"
      "  id,                                                                 \n"
      "  seq,                                                                \n"
      "  params,                                                             \n"
      "  headers,                                                            \n"
      "  { workerId }                                                        \n"
      ");                                                                    \n"
    );

    Lock lock(this->mutex);
    this->queuedResponses[queuedResponse.id] = std::move(queuedResponse);
    return script;
  }

  DispatchContext::~DispatchContext () {}
}
