#ifndef RUNTIME_PRELOAD_HH
#define RUNTIME_PRELOAD_HH

#include "runtime-preload-sources.hh"

#include "../app/app.hh"
#include "../window/window.hh"

namespace SSC {
  inline std::string createPreload (WindowOptions opts) {
    std::string cleanCwd = std::string(opts.cwd);
    std::replace(cleanCwd.begin(), cleanCwd.end(), '\\', '/');

    auto preload = std::string(
      ";(() => {"
      "  window.parent = {};\n"
      "  window.process = {};\n"
      "  window.process.index = Number('" + std::to_string(opts.index) + "');\n"
      "  window.process.port = Number('" + std::to_string(opts.port) + "');\n"
      "  window.process.cwd = () => '" + cleanCwd + "';\n"
      "  window.process.title = '" + opts.title + "';\n"
      "  window.process.executable = '" + opts.executable + "';\n"
      "  window.process.version = '" + opts.version + "';\n"
      "  window.process.debug = " + std::to_string(opts.debug) + ";\n"
      "  window.process.platform = '" + platform.os + "';\n"
      "  window.process.arch = '" + platform.arch + "';\n"
      "  window.process.env = Object.fromEntries(new URLSearchParams('" +  opts.env + "'));\n"
      "  window.process.config = Object.create({\n"
      "    get size () { return Object.keys(this).length }, \n"
      "    get (key) { \n"
      "      if (typeof key !== 'string') throw new TypeError('Expecting key to be a string.')\n"
      "      key = key.toLowerCase()\n"
      "      return key in this ? this[key] : null \n"
      "    }\n"
      "  });\n"
      "  window.process.argv = [" + opts.argv + "];\n"
      "  " + gPreload + "\n"
      "  " + opts.preload + "\n"
    );

    if (opts.headless) {
      preload += "                                                      \n"
        "console.log = (...args) => {                                   \n"
        "  const { index } = window.process;                            \n"
        "  const value = args                                           \n"
        "    .map((v) => typeof v === 'string' ? v : JSON.stringify(v)) \n"
        "    .map(encodeURIComponent)                                   \n"
        "    .join('');                                                 \n"
        "  const uri = `ipc://stdout?index=${index}&value=${value}`;    \n"
        "  window.external.invoke(uri);                                 \n"
        "};                                                             \n"
        "console.warn = console.error = console.log;                    \n";
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
        preload += "    window.process.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!Number.isNaN(parseFloat(value))) {\n";
        preload += "      window.process.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      window.process.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.seal(Object.freeze(window.process.config));\n";

    // depreceate usage of 'window.system' in favor of 'window.parent'
    preload += std::string(
      "  Object.defineProperty(window, 'system', {          \n"
      "    configurable: false,                             \n"
      "    enumerable: false,                               \n"
      "    writable: false,                                 \n"
      "    value: new Proxy(window.parent, {                \n"
      "      get (target, prop, receiver) {                 \n"
      "        console.warn(                                \n"
      "          `window.system.${prop} is depreceated. ` + \n"
      "          `Use window.parent.${prop} instead.`       \n"
      "         );                                          \n"
      "        return Reflect.get(...arguments);            \n"
      "      }                                              \n"
      "    })                                               \n"
      "  });                                                \n"
    );

    preload += "})();\n";
    preload += "//# sourceURL=preload.js\n";
    return preload;
  }
}
#endif
