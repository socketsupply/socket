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
    auto argv = opts.argv;
  #ifdef _WIN32
    // Escape backslashes in paths.
    size_t last_pos = 0;
    while ((last_pos = argv.find('\\', last_pos)) != std::string::npos) {
      argv.replace(last_pos, 1, "\\\\");
      last_pos += 2;
    }
  #endif

    auto preload = SSC::String(
      ";(() => {                                                             \n"
      "  if (globalThis.__args) return;                                      \n"
      "  globalThis.__args = {}                                              \n"
      "  const env = '" + opts.env + "';                                     \n"
      "  Object.defineProperties(globalThis.__args, {                        \n"
      "  argv: {                                                             \n"
      "    value: [" +argv + "],                                             \n"
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
      "});                                                                   \n"
      "                                                                      \n"
    );

    const auto start = argv.find("--test=");
    if (start != std::string::npos) {
      auto end = argv.find("'", start);
      if (end == std::string::npos) {
        end = argv.size();
      }
      const auto file = argv.substr(start + 7, end - start - 7);
      if (file.size() > 0) {
        preload += (
          "  document.addEventListener('DOMContentLoaded', () => {           \n"
          "    const script = document.createElement('script')               \n"
          "    script.setAttribute('type', 'module')                         \n"
          "    script.setAttribute('src', '" + file + "')                    \n"
          "    document.head.appendChild(script)                             \n"
          "  });                                                             \n"
        );
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

      preload += (
        "  ;(() => {                                                         \n"
        "    let key = decodeURIComponent(                                   \n"
        "      '" + encodeURIComponent(key) + "'                             \n"
        "    )                                                               \n"
      );

      if (key.starts_with("env_")) {
        preload += (
          "    const k = key.slice(4);                                       \n"
          "    const value = `" + value + "`;                                \n"
          "    globalThis.__args.env[k] = value;                             \n"
          "    globalThis.__args.config[key] = value;                        \n"
        );
      } else if (value == "true" || value == "false") {
        preload += (
          "    const k = key.toLowerCase();                                  \n"
          "    globalThis.__args.config[k] = " + value + "                   \n"
        );
      } else {
        preload += (
          "    const k = key.toLowerCase();                                  \n"
          "    const value = '" + encodeURIComponent(value) + "'             \n"
          "    if (!isNaN(value) && !Number.isNaN(parseFloat(value))) {      \n"
          "      globalThis.__args.config[k] = parseFloat(value) ;           \n"
          "    } else {                                                      \n"
          "      let val = decodeURIComponent(value);                        \n"
          "      try { val = JSON.parse(val) } catch (err) {}                \n"
          "      globalThis.__args.config[k] = val;                          \n"
          "    }                                                             \n"
        );
      }

      preload += "  })();\n";
    }

    preload += (
      "  Object.freeze(globalThis.__args.config);                            \n"
      "  Object.freeze(globalThis.__args.argv);                              \n"
      "  Object.freeze(globalThis.__args.env);                               \n"
      "})();                                                                 \n"
    );

    if (preloadOptions.module) {
      preload += "import 'socket:internal/init'\n";
    } else {
      preload += "import('socket:internal/init');\n";
    }

    return preload;
  }

  inline SSC::String createPreload (WindowOptions opts) {
    return createPreload(opts, PreloadOptions {});
  }
}
#endif
