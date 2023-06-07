#include "extension.hh"

void sapi_javascript_evaluate (
  sapi_context_t* ctx,
  const char* name,
  const char* source
) {
  if (ctx == nullptr || name == nullptr || source == nullptr) return;
  if (
    !ctx->isAllowed("javascript") &&
    !ctx->isAllowed("javascript_evaluate")
  ) {
    sapi_debug(ctx, "'javascript_evaluate' is not allowed.");
    return;
  }

  auto script = SSC::createJavaScript(name, source);
  ctx->router->evaluateJavaScript(script);
}
