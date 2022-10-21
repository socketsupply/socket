#ifndef RUNTIME_PRELOAD_HH
#define RUNTIME_PRELOAD_HH

#include "../window/options.hh"

namespace SSC {
  inline SSC::String createPreload (WindowOptions opts) {
    SSC::String cleanCwd = SSC::String(opts.cwd);
    std::replace(cleanCwd.begin(), cleanCwd.end(), '\\', '/');

    auto preload = SSC::String(
      "\n;(() => {                                                    \n"
      "window.__args = {                                              \n"
      "  arch: '" + platform.arch + "',                               \n"
      "  cwd: () => '" + cleanCwd + "',                               \n"
      "  debug: " + std::to_string(opts.debug) + ",                   \n"
      "  env: {},                                                     \n"
      "  executable: '" + opts.executable + "',                       \n"
      "  index: Number('" + std::to_string(opts.index) + "'),         \n"
      "  os: '" + platform.os + "',                                   \n"
      "  platform: '" + platform.os + "',                             \n"
      "  port: Number('" + std::to_string(opts.port) + "'),           \n"
      "  title: '" + opts.title + "',                                 \n"
      "  env: {},                                                     \n"
      "  version: '" + opts.version + "',                             \n"
      "};                                                             \n"
      "Object.assign(                                                 \n"
      "  window.__args.env,                                           \n"
      "  Object.fromEntries(new URLSearchParams('" +  opts.env + "')) \n"
      ");                                                             \n"
      "window.__args.argv = [" + opts.argv + "];                      \n"
    );

    if (platform.mac || platform.linux || platform.win) {
      preload += "                                                      \n"
        "if (window?.parent?.port > 0) {                                \n"
        "  window.addEventListener('menuItemSelected', e => {           \n"
        "    window.location.reload();                                  \n"
        "  });                                                          \n"
        "}                                                              \n"
        "const uri = `ipc://process.open`;                              \n"
        "if (window?.webkit?.messageHandlers?.external?.postMessage) {  \n"
        "  window.webkit.messageHandlers.external.postMessage(uri);     \n"
        "} else if (window?.chrome?.webview?.postMessage) {             \n"
        "  window.chrome.webview.postMessage(uri);                      \n"
        "}                                                              \n";
    }

    for (auto const &tuple : opts.appData) {
      auto key = trim(tuple.first);
      auto value = trim(tuple.second);

      // skip empty key/value and comments
      if (key.size() == 0 || value.size() == 0 || key.rfind("#", 0) == 0) {
        continue;
      }

      preload += "  ;(() => { \n";
      preload += "    const key = decodeURIComponent('" + encodeURIComponent(key) + "').toLowerCase()\n";

      if (value == "true" || value == "false") {
        preload += "    window.__args.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!Number.isNaN(parseFloat(value))) {\n";
        preload += "      window.__args.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      window.__args.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.seal(Object.freeze(window.__args.config));\n";

    // deprecate usage of 'window.system' in favor of 'window.__args'
    preload += SSC::String(
      "  Object.defineProperty(window, 'system', {         \n"
      "    configurable: false,                            \n"
      "    enumerable: false,                              \n"
      "    writable: false,                                \n"
      "    value: new Proxy(window.__args, {               \n"
      "      get (target, prop, receiver) {                \n"
      "        console.warn(                               \n"
      "          `window.system.${prop} is deprecated. ` + \n"
      "          `Use window.__args.${prop} instead.`      \n"
      "         );                                         \n"
      "        return Reflect.get(...arguments);           \n"
      "      }                                             \n"
      "    })                                              \n"
      "  });                                               \n"
    );

    preload += "})();\n";
    return preload;
  }
}
#endif
