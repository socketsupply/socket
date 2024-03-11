#include "codec.hh"
#include "core.hh"
#include "preload.hh"
#include "string.hh"
#include "config.hh"

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

    String preload = "";

    if (preloadOptions.wrap) {
      preload += "<script type=\"text/javascript\">\n";
    }

    preload += String(
      ";(() => {                                                             \n"
      "  if (globalThis.__args) return;                                      \n"
      "  globalThis.__args = {}                                              \n"
      "  const env = '" + opts.env + "';                                     \n"
      "  Object.defineProperties(globalThis.__args, {                        \n"
      "  argv: {                                                             \n"
      "    value: [" +argv + "],                                             \n"
      "    enumerable: true                                                  \n"
      "  },                                                                  \n"
      "  client: {                                                           \n"
      "    configurable: false,                                              \n"
      "    enumerable: true,                                                 \n"
      "    writable: false,                                                  \n"
      "    value: {                                                          \n"
      "      id: globalThis.window && globalThis.top !== globalThis.window   \n"
      "        ? '" + std::to_string(rand64()) + "'                          \n"
      "        : globalThis.window && globalThis.top                         \n"
      "          ? '" + std::to_string(opts.clientId) + "'                   \n"
      "          : null,                                                     \n"
      "      type: globalThis.window                                         \n"
      "        ? 'window'                                                    \n"
      "        : 'worker',                                                   \n"
      "      frameType:                                                      \n"
      "        globalThis.window && globalThis.top !== globalThis.window     \n"
      "        ? 'nested'                                                    \n"
      "        : globalThis.window && globalThis.top                         \n"
      "          ? 'top-level'                                               \n"
      "          : 'none'                                                    \n"
      "    }                                                                 \n"
      "  },                                                                  \n"
      "  config: {                                                           \n"
      "    configurable: false,                                              \n"
      "    enumerable: true,                                                 \n"
      "    writable: false,                                                  \n"
      "    value: {}                                                         \n"
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

    if (opts.index == 0) {
      const auto start = argv.find("--test=");
      if (start != std::string::npos) {
        auto end = argv.find("'", start);
        if (end == std::string::npos) {
          end = argv.size();
        }
        const auto file = argv.substr(start + 7, end - start - 7);
        if (file.size() > 0) {
          preload += (
            "  globalThis.RUNTIME_TEST_FILENAME = `" + file + "`;              \n"
            "  document.addEventListener('DOMContentLoaded', () => {           \n"
            "    const script = document.createElement('script')               \n"
            "    script.setAttribute('type', 'module')                         \n"
            "    script.setAttribute('src', `" + file + "`)                    \n"
            "    document.head.appendChild(script)                             \n"
            "  });                                                             \n"
          );
        }
      }
    }

    // buffer `applicationurl` events
    preload += (
        "  Object.defineProperty(                                            \n"
        "    globalThis,                                                     \n"
        "    'APPLICATION_URL_EVENT_BACKLOG',                                \n"
        "    { enumerable: false, configurable: false, value: [] }           \n"
        "  );                                                                \n"
        "                                                                    \n"
        "  globalThis.addEventListener('applicationurl', (e) => {            \n"
        "    if (document.readyState !== 'complete') {                       \n"
        "      APPLICATION_URL_EVENT_BACKLOG.push(e);                        \n"
        "    }                                                               \n"
        "  });                                                               \n"
        "                                                                    \n"
        "  globalThis.addEventListener('__runtime_init__', () => {           \n"
        "    if (Array.isArray(APPLICATION_URL_EVENT_BACKLOG)) {             \n"
        "      for (const event of APPLICATION_URL_EVENT_BACKLOG) {          \n"
        "        globalThis.dispatchEvent(                                   \n"
        "          new ApplicationURLEvent(event.type, event)                \n"
        "        );                                                          \n"
        "      }                                                             \n"
        "    }                                                               \n"
        "                                                                    \n"
        "    APPLICATION_URL_EVENT_BACKLOG.splice(                           \n"
        "      0,                                                            \n"
        "      APPLICATION_URL_EVENT_BACKLOG.length - 1                      \n"
        "    );                                                              \n"
        "  }, { once: true });                                               \n"
    );

    if (opts.userConfig.contains("webview_watch") && opts.userConfig.at("webview_watch") == "true") {
      if (
        !opts.userConfig.contains("webview_watch_reload") ||
        opts.userConfig.at("webview_watch_reload") != "false"
      ) {
          preload += (
            "  globalThis.addEventListener('filedidchange', () => {          \n"
            "    location.reload()                                           \n"
            "  });                                                           \n"
        );
      }
    }

    // fill in the config
    for (auto const &tuple : opts.userConfig) {
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
      "  Object.freeze(globalThis.__args.client);                            \n"
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

    if (preloadOptions.module) {
      if (preloadOptions.wrap) {
        preload += "</script>\n";
        preload += "<script type=\"module\">\n";
      }

      preload += (
        "import 'socket:internal/init'                                       \n"
      );

      if (preloadOptions.userScript.size() > 0) {
        preload += preloadOptions.userScript;
      }

      if (preloadOptions.wrap) {
        preload += "</script>\n";
      }
    } else {
      preload += (
        "if (document.readyState === 'complete') {                           \n"
        "  import('socket:internal/init')                                    \n"
        "    .then(async () => {                                             \n"
        "      " + preloadOptions.userScript + "                             \n"
        "    })                                                              \n"
        "    .catch(console.error);                                          \n"
        "} else {                                                            \n"
        "  document.addEventListener('readystatechange', () => {             \n"
        "    if (/interactive|complete/.test(document.readyState)) {         \n"
        "      import('socket:internal/init')                                \n"
        "        .then(async () => {                                         \n"
        "          " + preloadOptions.userScript + "                         \n"
        "        })                                                          \n"
        "        .catch(console.error);                                      \n"
        "    }                                                               \n"
        "  }, { once: true });                                               \n"
        "}                                                                   \n"
      );

      if (preloadOptions.wrap) {
        preload += "</script>\n";
      }
    }

    if (preloadOptions.wrap) {
      preload += "<script>\n";
    }

    preload += (
      ";(async function preloadCommonJSScope () {                            \n"
      "  const href = encodeURIComponent(globalThis.location.href);          \n"
      "  const source = `socket:module?ref=${href}`;                         \n"
      "                                                                      \n"
      "  const { Module } = await import(source);                            \n"
      "  const path = await import('socket:path');                           \n"
      "                                                                      \n"
      "  const require = Module.createRequire(globalThis.location.href);     \n"
      "  const __filename = Module.main.filename;                            \n"
      "  const __dirname = path.dirname(__filename);                         \n"
      "                                                                      \n"
      "  Object.defineProperties(globalThis, {                               \n"
      "    require: {                                                        \n"
      "      configurable: true,                                             \n"
      "      enumerable: false,                                              \n"
      "      writable: false,                                                \n"
      "      value: require,                                                 \n"
      "    },                                                                \n"
      "    module: {                                                         \n"
      "      configurable: true,                                             \n"
      "      enumerable: false,                                              \n"
      "      writable: false,                                                \n"
      "      value: Module.main,                                             \n"
      "    },                                                                \n"
      "    exports: {                                                        \n"
      "      configurable: true,                                             \n"
      "      enumerable: false,                                              \n"
      "      get: () => Module.main.exports,                                 \n"
      "    },                                                                \n"
      "    __dirname: {                                                      \n"
      "      configurable: true,                                             \n"
      "      enumerable: false,                                              \n"
      "      writable: false,                                                \n"
      "      value: __dirname,                                               \n"
      "    },                                                                \n"
      "    __filename: {                                                     \n"
      "      configurable: true,                                             \n"
      "      enumerable: false,                                              \n"
      "      writable: false,                                                \n"
      "      value: require,                                                 \n"
      "    },                                                                \n"
      "  });                                                                 \n"
      "                                                                      \n"
      "  globalThis.addEventListener('popstate', preloadCommonJSScope);      \n"
      "})();                                                                 \n"
    );

    if (opts.runtimePrimordialOverrides.size() > 0) {
      preload += (
        "globalThis.__RUNTIME_PRIMORDIAL_OVERRIDES__ = (                     \n"
        "  " + opts.runtimePrimordialOverrides + "                           \n"
        ")                                                                   \n"
      );
    }

    if (preloadOptions.wrap) {
      preload += "</script>\n";
    }

    return preload;
  }
}
