#ifndef RUNTIME_PRELOAD_HH
#define RUNTIME_PRELOAD_HH

#include "runtime-preload-sources.hh"

#include "../window/options.hh"

namespace SSC {
  inline SSC::String createPreload (WindowOptions opts) {
    SSC::String cleanCwd = SSC::String(opts.cwd);
    std::replace(cleanCwd.begin(), cleanCwd.end(), '\\', '/');

    auto preload =SSC::String(gPreload) + SSC::String(
      "\n;(() => {                                                    \n"
      "Object.assign(window.parent, {                                 \n"
      "  arch: '" + platform.arch + "',                               \n"
      "  cwd: () => '" + cleanCwd + "',                               \n"
      "  debug: " + std::to_string(opts.debug) + ",                   \n"
      "  executable: '" + opts.executable + "',                       \n"
      "  index: Number('" + std::to_string(opts.index) + "'),         \n"
      "  platform: '" + platform.os + "',                             \n"
      "  port: Number('" + std::to_string(opts.port) + "'),           \n"
      "  title: '" + opts.title + "',                                 \n"
      "  version: '" + opts.version + "',                             \n"
      "});                                                            \n"
      "Object.assign(                                                 \n"
      "  window.parent.env,                                           \n"
      "  Object.fromEntries(new URLSearchParams('" +  opts.env + "')) \n"
      ");                                                             \n"
      "window.parent.argv = [" + opts.argv + "];                      \n"
      "" + opts.preload
    );

    if (opts.headless) {
      preload += "                                                      \n"
        "console.log = (...args) => {                                   \n"
        "  const { index } = window.parent;                             \n"
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
        preload += "    window.parent.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!Number.isNaN(parseFloat(value))) {\n";
        preload += "      window.parent.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      window.parent.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.seal(Object.freeze(window.parent.config));\n";

    // depreceate usage of 'window.system' in favor of 'window.parent'
    preload += SSC::String(
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
    return preload;
  }
}
#endif
