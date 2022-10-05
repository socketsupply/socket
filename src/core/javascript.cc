#include "core.hh"

namespace SSC {
  String createJavaScript (const String& name, const String& source) {
    return String(
      ";(() => {\n" + trim(source) + "\n})();\n"
      "//# sourceURL=" + name + "\n"
    );
  }

  String getEmitToRenderProcessJavaScript (
    const String& event,
    const String& value
  ) {
    return createJavaScript("emit-to-render-process.js",
      "const name = decodeURIComponent(`" + event + "`); \n"
      "const value = `" + value + "`;                    \n"
      "window._ipc.emit(name, value);                    \n"
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
      " if (" + seq + " > 0 && window._ipc['R" + seq + "']) {     \n"
      "   window._ipc['R" + seq + "'].resolve(detail);            \n"
      "   delete window._ipc['R" + seq + "'];                     \n"
      "   return;                                                 \n"
      " }                                                         \n"
      "                                                           \n"
      "const event = new window.CustomEvent('menuItemSelected', { \n"
      "  detail                                                   \n"
      "});                                                        \n"
      "                                                           \n"
      "window.dispatchEvent(event);                               \n"
    );
  }

  String getResolveToRenderProcessJavaScript (
    const String& seq,
    const String& state,
    const String& value
  ) {
    return createJavaScript("resolve-to-render-proecss.js",
      "const seq = String('" + seq + "');      \n"
      "const state = Number('" + state + "');  \n"
      "const value = '" + value + "';          \n"
      "window._ipc.resolve(seq, state, value);"
    );
  }
}
