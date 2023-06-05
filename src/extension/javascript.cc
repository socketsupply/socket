#include "extension.hh"

void sapi_javascript_evaluate (
  sapi_context_t* ctx,
  const char* name,
  const char* source
) {
  if (ctx == nullptr || name == nullptr || source == nullptr) return;
  auto script = SSC::createJavaScript(name, source);
  ctx->router->evaluateJavaScript(script);
}
