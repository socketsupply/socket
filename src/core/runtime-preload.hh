#ifndef RUNTIME_PRELOAD_HH
#define RUNTIME_PRELOAD_HH

#include "../window/options.hh"

namespace SSC {
  inline SSC::String createPreload (WindowOptions opts) {
    SSC::String cleanCwd = SSC::String(opts.cwd);
    std::replace(cleanCwd.begin(), cleanCwd.end(), '\\', '/');

    auto preload = SSC::String(
      "\n;(() => {                                                               \n"
      "  window.__args = {}                                                      \n"
      "  Object.defineProperties(window.__args, {                                \n"
      "  arch: {                                                                 \n"
      "    value: '" + platform.arch + "',                                       \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  os: {                                                                   \n"
      "    value: '" + platform.os + "',                                         \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  cwd: {                                                                  \n"
      "    value: () => '" + cleanCwd + "',                                      \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  debug: {                                                                \n"
      "    value: " + std::to_string(opts.debug) + ",                            \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  env: {                                                                  \n"
      "    value: Object.fromEntries(new URLSearchParams('" +  opts.env + "')),  \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  index: {                                                                \n"
      "    value: Number('" + std::to_string(opts.index) + "'),                  \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  argv: {                                                                 \n"
      "    value: [" + opts.argv + "],                                           \n"
      "    enumerable: true                                                      \n"
      "  },                                                                      \n"
      "  config: {                                                               \n"
      "    value: {},                                                            \n"
      "    enumerable: true,                                                     \n"
      "    writable: true,                                                       \n"
      "    configurable: true                                                    \n"
      "  }                                                                       \n"
      "})                                                                        \n"
      "Object.freeze(window.__args.argv)                                         \n"
      "Object.freeze(window.__args.env)                                          \n"
    );

    if (platform.mac || platform.linux || platform.win) {
      preload += "                                                      \n"
        "if (window?.parent?.port > 0) {                                \n"
        "  window.addEventListener('menuItemSelected', e => {           \n"
        "    window.location.reload();                                  \n"
        "  });                                                          \n"
        "}                                                              \n";
    }

    // fill in the config
    for (auto const &tuple : opts.appData) {
      auto key = trim(tuple.first);
      auto value = trim(tuple.second);

      // skip empty key/value and comments
      if (key.size() == 0 || value.size() == 0 || key.rfind(";", 0) == 0) {
        continue;
      }

      preload += "  ;(() => { \n";
      preload += "    const key = decodeURIComponent('" + encodeURIComponent(key) + "').toLowerCase()\n";

      if (value == "true" || value == "false") {
        preload += "    window.__args.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!isNaN(value) && !Number.isNaN(parseFloat(value))) {\n";
        preload += "      window.__args.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      window.__args.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.freeze(window.__args.config);\n";
    preload += "})();\n";
    return preload;
  }
}
#endif
