#include "core.hh"
#include "json.hh"

namespace SSC {
  String createJavaScript (const String& name, const String& source) {
    return String(
      ";(async () => {                                                       \n"
      "  console.assert(                                                     \n"
      "    typeof globalThis.__RUNTIME_INIT_NOW__ === 'number',              \n"
      "    '__RUNTIME_INIT_NOW__ check failed. ' +                           \n"
      "    'The webview environment may not be initialized correctly.'       \n"
      "  );                                                                  \n"
      "  " + trim(source) + ";                                          \n"
      "})();                                                                 \n"
      "//# sourceURL=" + name + "                                            \n"
    );
  }

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  ) {
    return getEmitToRenderProcessJavaScript(event, value, "window", JSON::Object {});
  }

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value,
    const String& target,
    const JSON::Object& options
  ) {
    return createJavaScript("emit-to-render-process.js",
      "const name = decodeURIComponent(`" + event + "`);                   \n"
      "const value = `" + value + "`;                                      \n"
      "const target = " + target + ";                                      \n"
      "const options = " + options.str() + ";                              \n"
      "let detail = value;                                                 \n"
      "                                                                    \n"
      "if (typeof value === 'string') {                                    \n"
      "  try {                                                             \n"
      "    detail = decodeURIComponent(value);                             \n"
      "    detail = JSON.parse(detail);                                    \n"
      "  } catch (err) {                                                   \n"
      "    if (!detail) {                                                  \n"
      "      console.error(`${err.message} (${value})`);                   \n"
      "      return;                                                       \n"
      "    }                                                               \n"
      "  }                                                                 \n"
      "}                                                                   \n"
      "                                                                    \n"
      "const event = new CustomEvent(name, { detail, ...options });        \n"
      "target.dispatchEvent(event);                                        \n"
    );
  }

  String getResolveMenuSelectionJavaScript (
    const String& seq,
    const String& title,
    const String& parent
  ) {
    return createJavaScript("resolve-menu-selection.js",
      "const detail = {                                           \n"
      "  title: decodeURIComponent(`" + title + "`),              \n"
      "  parent: decodeURIComponent(`" + parent + "`),            \n"
      "  state: '0'                                               \n"
      "};                                                         \n"
      "                                                           \n"
      " if (" + seq + " > 0 && globalThis._ipc['R" + seq + "']) {     \n"
      "   globalThis._ipc['R" + seq + "'].resolve(detail);            \n"
      "   delete globalThis._ipc['R" + seq + "'];                     \n"
      "   return;                                                 \n"
      " }                                                         \n"
      "                                                           \n"
      "const event = new globalThis.CustomEvent('menuItemSelected', { \n"
      "  detail                                                   \n"
      "});                                                        \n"
      "                                                           \n"
      "globalThis.dispatchEvent(event);                               \n"
    );
  }

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return createJavaScript("resolve-to-render-process.js",
      "const seq = String('" + seq + "');                    \n"
      "const value = '" + value + "';                        \n"
      "const index = globalThis.__args.index;                    \n"
      "const state = Number('" + state + "');                \n"
      "const eventName = `resolve-${index}-${seq}`;          \n"
      "let detail = value;                                   \n"
      "                                                      \n"
      "if (typeof value === 'string') {                      \n"
      "  try {                                               \n"
      "    detail = decodeURIComponent(value);               \n"
      "    detail = JSON.parse(detail);                      \n"
      "  } catch (err) {                                     \n"
      "    if (!detail) {                                    \n"
      "      console.error(`${err.message} (${value})`);     \n"
      "      return;                                         \n"
      "    }                                                 \n"
      "  }                                                   \n"
      "}                                                     \n"
      "                                                      \n"
      "if (detail?.err) {                                    \n"
      "  let err = detail?.err ?? detail;                    \n"
      "  if (typeof err === 'string') {                      \n"
      "    err = new Error(err);                             \n"
      "  }                                                   \n"
      "                                                      \n"
      "  detail = { err };                                   \n"
      "} else if (detail?.data) {                            \n"
      "  detail = { ...detail }                              \n"
      "} else {                                              \n"
      "  detail = { data: detail }                           \n"
      "}                                                     \n"
      "                                                      \n"
      "const event = new CustomEvent(eventName, { detail }); \n"
      "globalThis.dispatchEvent(event);                          \n"
    );
  }
}
