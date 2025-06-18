#include "../javascript.hh"
#include "../string.hh"

using namespace ssc::runtime::string;

namespace ssc::runtime::javascript {
  String createJavaScript (const String& name, const String& source) {
    return String(
      ";(async () => {                                                       \n"
      "  const globals = await import('socket:internal/globals');            \n"
      "  if (!globalThis.__RUNTIME_INIT_NOW__) {                             \n"
      "    await new Promise((resolve) => {                                  \n"
      "      globalThis.addEventListener('__runtime_init__', resolve, {      \n"
      "        once: true                                                    \n"
      "      });                                                             \n"
      "    });                                                               \n"
      "  }                                                                   \n"
      "                                                                      \n"
      "  console.assert(                                                     \n"
      "    typeof globalThis.__RUNTIME_INIT_NOW__ === 'number',              \n"
      "    '__RUNTIME_INIT_NOW__ check failed. ' +                           \n"
      "    'The webview environment may not be initialized correctly.'       \n"
      "  );                                                                  \n"
      "                                                                      \n"
      "  globals.get('RuntimeExecution').runInAsyncScope(async () => {       \n"
      "    " + trim(source) + ";                                             \n"
      "  });                                                                 \n"
      "})();                                                                 \n"
      "undefined;                                                            \n"
      "//# sourceURL=" + name + "                                            \n"
    );
  }

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  ) {
    return getEmitToRenderProcessJavaScript(event, value, "globalThis", JSON::Object {});
  }

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value,
    const String& target,
    const JSON::Object& options
  ) {
    const auto jsonValue = JSON::Any(replace(value, "\\\\", "\\\\")).str();

    return createJavaScript("emit-to-render-process.js",
      "const name = decodeURIComponent(`" + event + "`);                     \n"
      "const value = " + jsonValue + ";                                      \n"
      "const target = " + target + ";                                        \n"
      "const options = " + options.str() + ";                                \n"
      "let detail = value;                                                   \n"
      "                                                                      \n"
      "if (typeof value === 'string') {                                      \n"
      "  try {                                                               \n"
      "    detail = decodeURIComponent(value);                               \n"
      "    if (detail === '') detail = '{}';                                 \n"
      "    detail = JSON.parse(detail);                                      \n"
      "  } catch (err) {                                                     \n"
      "    if (!detail) {                                                    \n"
      "      console.error(`${err.message} (${value})`);                     \n"
      "      return;                                                         \n"
      "    }                                                                 \n"
      "  }                                                                   \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (name === 'applicationurl') {                                      \n"
      "  const event = new ApplicationURLEvent(name, detail);                \n"
      "  if (event.isValid) {                                                \n"
      "    target.dispatchEvent(event);                                      \n"
      "  }                                                                   \n"
      "  return;                                                             \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (name === 'hotkey') {                                              \n"
      "  const event = new HotKeyEvent(name, detail);                        \n"
      "  target.dispatchEvent(event);                                        \n"
      "  return;                                                             \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (name === 'message') {                                             \n"
      "  const event = new MessageEvent(name, { data: detail });             \n"
      "  target.dispatchEvent(event);                                        \n"
      "  return;                                                             \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (name === 'drag') {                                                \n"
      "  return globalThis.dispatchEvent(new CustomEvent('platformdrag', {   \n"
      "    detail                                                            \n"
      "  }));                                                                \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (name === 'dropin' || name === 'drop') {                           \n"
      "  return globalThis.dispatchEvent(new CustomEvent('platformdrop', {   \n"
      "    detail: {                                                         \n"
      "      ...detail,                                                      \n"
      "      files: Array.from(detail?.src || detail?.files || [])           \n"
      "        .filter(Boolean)                                              \n"
      "    }                                                                 \n"
      "  }));                                                                \n"
      "}                                                                     \n"
      "const event = new CustomEvent(name, { detail, ...options });          \n"
      "target.dispatchEvent(event);                                          \n"
    );
  }

  String getResolveMenuSelectionJavaScript (
    const String& seq,
    const String& title,
    const String& parent,
    const String type
  ) {
    return createJavaScript("resolve-menu-selection.js",
      "const detail = {                                                      \n"
      "  title: decodeURIComponent(`" + title + "`),                         \n"
      "  parent: decodeURIComponent(`" + parent + "`),                       \n"
      "  type: `" + type +"`,                                                \n"
      "  state: '0'                                                          \n"
      "};                                                                    \n"
      "                                                                      \n"
      "const event = new globalThis.CustomEvent('menuItemSelected', {        \n"
      "  detail                                                              \n"
      "});                                                                   \n"
      "                                                                      \n"
      "globalThis.dispatchEvent(event);                                      \n"
    );
  }

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return createJavaScript("resolve-to-render-process.js",
      "const seq = String('" + seq + "');                                    \n"
      "const value = '" + value + "';                                        \n"
      "const index = globalThis.__args.index;                                \n"
      "const state = Number('" + state + "');                                \n"
      "const eventName = `resolve-${index}-${seq}`;                          \n"
      "let detail = value;                                                   \n"
      "                                                                      \n"
      "if (typeof value === 'string') {                                      \n"
      "  try {                                                               \n"
      "    detail = decodeURIComponent(value);                               \n"
      "    detail = JSON.parse(detail);                                      \n"
      "  } catch (err) {                                                     \n"
      "    if (!detail) {                                                    \n"
      "      console.error(`${err.message} (${value})`);                     \n"
      "      return;                                                         \n"
      "    }                                                                 \n"
      "  }                                                                   \n"
      "}                                                                     \n"
      "                                                                      \n"
      "if (detail?.err) {                                                    \n"
      "  let err = detail?.err || detail;                                    \n"
      "  if (typeof err === 'string') {                                      \n"
      "    err = new Error(err);                                             \n"
      "  }                                                                   \n"
      "                                                                      \n"
      "  detail = { err };                                                   \n"
      "} else if (detail?.data) {                                            \n"
      "  detail = { ...detail };                                             \n"
      "} else {                                                              \n"
      "  detail = { data: detail };                                          \n"
      "}                                                                     \n"
      "                                                                      \n"
      "const event = new CustomEvent(eventName, { detail });                 \n"
      "globalThis.dispatchEvent(event);                                      \n"
    );
  }
}
