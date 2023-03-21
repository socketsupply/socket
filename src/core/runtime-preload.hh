#ifndef RUNTIME_PRELOAD_HH
#define RUNTIME_PRELOAD_HH

#include "../window/options.hh"

namespace SSC {
  struct PreloadOptions {
    bool module = false;
  };

  inline String createPreload (
    const WindowOptions opts,
    const PreloadOptions preloadOptions
  ) {
#ifdef _WIN32
    // Escape backslashes in paths.
    size_t last_pos = 0;
    while ((last_pos = opts.argv.find('\\', last_pos)) != std::string::npos) {
      opts.argv.replace(last_pos, 1, "\\\\");
      last_pos += 2;
    }
#endif

    auto preload = SSC::String(
      "\n;(() => {                                                           \n"
      "  if (globalThis.__args) return;                                      \n"
      "  globalThis.__args = {}                                              \n"
      "  const env = '" + opts.env + "';                                     \n"
      "  Object.defineProperties(globalThis.__args, {                        \n"
      "  argv: {                                                             \n"
      "    value: [" + opts.argv + "],                                       \n"
      "    enumerable: true                                                  \n"
      "  },                                                                  \n"
      "  config: {                                                           \n"
      "    value: {},                                                        \n"
      "    enumerable: true,                                                 \n"
      "    writable: true,                                                   \n"
      "    configurable: true                                                \n"
      "  },                                                                  \n"
      "  debug: {                                                            \n"
      "    value: " + std::to_string(opts.debug) + ",                        \n"
      "    enumerable: true                                                  \n"
      "  },                                                                  \n"
      "  env: {                                                              \n"
      "    value: Object.fromEntries(new URLSearchParams(env)),              \n"
      "    enumerable: true                                                  \n"
      "  },                                                                  \n"
      "  index: {                                                            \n"
      "    value: Number('" + std::to_string(opts.index) + "'),              \n"
      "    enumerable: true                                                  \n"
      "  }                                                                   \n"
      "})                                                                    \n"
      "Object.freeze(globalThis.__args.argv)                                 \n"
      "Object.freeze(globalThis.__args.env)                                  \n"
    );

    const auto start = opts.argv.find("--test=");
    if (start != std::string::npos) {
      auto end = opts.argv.find("'", start);
      if (end == std::string::npos) {
        end = opts.argv.size();
      }
      const auto file = opts.argv.substr(start + 7, end - start - 7);
      if (file.size() > 0) {
        preload += "                                                         \n"
          "document.addEventListener('DOMContentLoaded', () => {             \n"
          "  const script = document.createElement('script')                 \n"
          "  script.setAttribute('type', 'module')                           \n"
          "  script.setAttribute('src', '" + file + "')                      \n"
          "  document.head.appendChild(script)                               \n"
          "});                                                               \n"
          ;
      }
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
        preload += "    globalThis.__args.config[key] = " + value + "\n";
      } else {
        preload += "    const value = '" + encodeURIComponent(value) + "'\n";
        preload += "    if (!isNaN(value) && !Number.isNaN(parseFloat(value))) {\n";
        preload += "      globalThis.__args.config[key] = parseFloat(value);\n";
        preload += "    } else { \n";
        preload += "      let val = decodeURIComponent(value);\n";
        preload += "      try { val = JSON.parse(val) } catch (err) {}\n";
        preload += "      globalThis.__args.config[key] = val;\n";
        preload += "    }\n";
      }

      preload += "  })();\n";
    }

    preload += "  Object.freeze(globalThis.__args.config);\n";
    preload += "})();\n";

    if (preloadOptions.module) {
      preload += "import 'socket:internal/init'\n";
    } else {
      preload += "import('socket:internal/init');\n";
    }
    //preload += "catch (err) { import('socket:internal/init'); }\n";
    return preload;
  }

  inline SSC::String createPreload (WindowOptions opts) {
    return createPreload(opts, PreloadOptions {});
  }
}
#endif
