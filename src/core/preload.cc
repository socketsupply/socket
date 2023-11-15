#include "codec.hh"
#include "preload.hh"
#include "string.hh"

namespace SSC {
  String createPreload (
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

    auto preload = String(
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
      "    value: Boolean(" + std::to_string(opts.debug) + "),               \n"
      "    enumerable: true                                                  \n"
      "  },                                                                  \n"
      "  headless: {                                                         \n"
      "    value: Boolean(" + std::to_string((int) opts.headless) + "),      \n"
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

    if (opts.appData.contains("webview_watch") && opts.appData.at("webview_watch") == "true") {
      if (
        !opts.appData.contains("webview_watch_reload") ||
        opts.appData.at("webview_watch_reload") != "false"
      ) {
          preload += (
            "  globalThis.addEventListener('filedidchange', () => {            \n"
            "  location.reload()                                               \n"
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
      "                                                                      \n"
      "  try {                                                               \n"
      "    const event = '__runtime_init__';                                 \n"
      "    let onload = null                                                 \n"
      "    Object.defineProperty(globalThis, 'onload', {                     \n"
      "      get: () => onload,                                              \n"
      "      set (value) {                                                   \n"
      "        const opts = { once: true };                                  \n"
      "        if (onload) {                                                 \n"
      "          globalThis.removeEventListener(event, onload, opts);        \n"
      "          onload = null;                                              \n"
      "        }                                                             \n"
      "                                                                      \n"
      "        if (typeof value === 'function') {                            \n"
      "          onload = value;                                             \n"
      "          globalThis.addEventListener(event, onload, opts);           \n"
      "        }                                                             \n"
      "      }                                                               \n"
      "    });                                                               \n"
      "  } catch {}                                                          \n"
      "})();                                                                 \n"
    );

    preload += (
      "if (document.readyState === 'complete') {                             \n"
      "  import('socket:internal/init').catch(console.error);                \n"
      "} else {                                                              \n"
      "  document.addEventListener('readystatechange', () => {               \n"
      "    if (/interactive|complete/.test(document.readyState)) {           \n"
      "      import('socket:internal/init').catch(console.error);            \n"
      "    }                                                                 \n"
      "  }, { once: true });                                                 \n"
      "}                                                                     \n"
    );

    return preload;
  }
}
