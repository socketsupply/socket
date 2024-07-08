#include "../core/core.hh"
#include "preload.hh"

namespace SSC::IPC {
  static constexpr auto DEFAULT_REFERRER_POLICY = "unsafe-url";
  static constexpr auto RUNTIME_PRELOAD_META_BEGIN_TAG = (
    R"HTML(<meta name="begin-runtime-preload">)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_META_END_TAG = (
    R"HTML(<meta name="end-runtime-preload">)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG = (
    R"HTML(<script type="text/javascript">)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_JAVASCRIPT_END_TAG = (
    R"HTML(</script>)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_MODULE_BEGIN_TAG = (
    R"HTML(<script type="module">)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_MODULE_END_TAG = (
    R"HTML(</script>)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_IMPORTMAP_BEGIN_TAG = (
    R"HTML(<script type="importmap">)HTML"
  );

  static constexpr auto RUNTIME_PRELOAD_IMPORTMAP_END_TAG = (
    R"HTML(</script>)HTML"
  );

  const Preload Preload::compile (const Options& options) {
    auto preload = Preload(options);
    preload.compile();
    return preload;
  }

  Preload::Preload (const Options& options)
    : options(options)
  {
    this->configure();
    if (this->options.userConfig.size() == 0) {
      this->options.userConfig = getUserConfig();
    }
  }

  void Preload::configure () {
    this->configure(this->options);
  }

  void Preload::configure (const Options& options) {
    this->options = options;
    this->headers = options.headers;
    this->metadata = options.metadata;

    if (options.features.useHTMLMarkup) {
      if (!this->metadata.contains("referrer")) {
        this->metadata["referrer"] = DEFAULT_REFERRER_POLICY;
      }

      if (!this->headers.has("referrer-policy")) {
        if (this->metadata["referrer"].size() > 0) {
          this->headers.set("referrer-policy", this->metadata["referrer"]);
        }
      }
    }

    if (Env::has("SOCKET_RUNTIME_VM_DEBUG")) {
      this->options.env["SOCKET_RUNTIME_VM_DEBUG"] = Env::get("SOCKET_RUNTIME_VM_DEBUG");
    }

    if (Env::has("SOCKET_RUNTIME_NPM_DEBUG")) {
      this->options.env["SOCKET_RUNTIME_NPM_DEBUG"] = Env::get("SOCKET_RUNTIME_NPM_DEBUG");
    }

    if (Env::has("SOCKET_RUNTIME_SHARED_WORKER_DEBUG")) {
      this->options.env["SOCKET_RUNTIME_SHARED_WORKER_DEBUG"] = Env::get("SOCKET_RUNTIME_SHARED_WORKER_DEBUG");
    }

    if (Env::has("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG")) {
      this->options.env["SOCKET_RUNTIME_SERVICE_WORKER_DEBUG"] = Env::get("SOCKET_RUNTIME_SERVICE_WORKER_DEBUG");
    }
  }

  Preload& Preload::append (const String& source) {
    this->buffer.push_back(source);
    return *this;
  }

  const String& Preload::compile () {
    Vector<String> buffers;

    auto args = JSON::Object {
      JSON::Object::Entries {
        {"argv", JSON::Array {}},
        {"client", JSON::Object {}},
        {"config", JSON::Object {}},
        {"debug", this->options.debug},
        {"headless", this->options.headless},
        {"env", JSON::Object {}},
        {"index", this->options.index}
      }
    };

    for (const auto& value : this->options.argv) {
      args["argv"].as<JSON::Array>().push(value);
    }

    // 1. compile metadata if `options.features.useHTMLMarkup == true`, otherwise skip
    if (this->options.features.useHTMLMarkup) {
      for (const auto &entry : this->metadata) {
        if (entry.second.size() == 0) {
          continue;
        }

        buffers.push_back(tmpl(
          R"HTML(<meta name="{{name}}" content="{{content}}">)HTML",
          Map {
            {"name", trim(entry.first)},
            {"content", trim(decodeURIComponent(entry.second))}
          }
        ));
      }
    }

    // 2. compile headers if `options.features.useHTMLMarkup == true`, otherwise skip
    if (this->options.features.useHTMLMarkup) {
      for (const auto &entry : this->headers) {
        buffers.push_back(tmpl(
          R"HTML(<meta http-equiv="{{header}}" content="{{value}}">)HTML",
          Map {
            {"header", toHeaderCase(entry.name)},
            {"value", trim(decodeURIComponent(entry.value.str()))}
          }
        ));
      }
    }

    // 3. compile preload `<meta name="begin-runtime-preload">` "BEGIN" tag if `options.features.useHTMLMarkup == true`, otherwise skip
    if (this->options.features.useHTMLMarkup) {
      buffers.push_back(RUNTIME_PRELOAD_META_BEGIN_TAG);
    }

    if (this->options.features.useGlobalArgs) {
      // 4. compile preload `<script>` prefix if `options.features.useHTMLMarkup == true`, otherwise skip
      if (this->options.features.useHTMLMarkup) {
        buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG);
      }

      // 5. compile `globalThis.__args` object
      buffers.push_back(";(() => {");
      buffers.push_back(trim(tmpl(
        R"JAVASCRIPT(
          Object.defineProperty(globalThis, '__args', {
            configurable: false,
            enumerable: false,
            writable: false,
            value: {}
          })
          Object.defineProperties(globalThis.__args, {
            argv: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {{argv}}
            },
            client: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {}
            },
            config: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {}
            },
            debug: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {{debug}}
            },
            env: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {}
            },
            headless: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {{headless}}
            },
            index: {
              configurable: false,
              enumerable: true,
              writable: false,
              value: {{index}}
            },
          })
        )JAVASCRIPT",
        Map {
          {"argv", args["argv"].str()},
          {"debug", args["debug"].str()},
          {"headless", args["headless"].str()},
          {"index", args["index"].str()},
        }
      )));

      // 6. compile `globalThis.__args.client` values
      buffers.push_back(trim(tmpl(
        R"JAVASCRIPT(
        Object.defineProperties(globalThis.__args.client, {
          id: {
            configurable: false,
            enumerable: true,
            writable: false,
            value: globalThis.window && globalThis.top !== globalThis.window
              ? '{{id}}'
              : globalThis.window && globalThis.top
                ? '{{clientId}}'
                : null
          },
          type: {
            configurable: false,
            enumerable: true,
            writable: true,
            value: globalThis.window ? 'window' : 'worker'
          },
          parent: {
            configurable: false,
            enumerable: true,
            writable: false,
            value: globalThis.parent !== globalThis
	            ? globalThis.parent?.__args?.client ?? null
	            : null
          },
          top: {
            configurable: false,
            enumerable: true,
            get: () => globalThis.top
	            ? globalThis.top.__args?.client ?? null
	            : globalThis.__args.client
          },
          frameType: {
            configurable: false,
            enumerable: true,
            writable: true,
            value: globalThis.window && globalThis.top !== globalThis.window
              ? 'nested'
              : globalThis.window && globalThis.top
                ? 'top-level'
                : 'none'
          },
        })
        )JAVASCRIPT",
        Map {
          {"id", std::to_string(rand64())},
          {"clientId", std::to_string(this->options.client.id)}
        }
      )));

      // 7. compile `globalThis.__args.config` values
      buffers.push_back(R"JAVASCRIPT(const __RAW_CONFIG__ = {})JAVASCRIPT");

      for (const auto& entry : this->options.userConfig) {
        const auto key = trim(entry.first);
        const auto value = trim(entry.second);

        // skip empty key/value and comments
        if (key.size() == 0 || value.size() == 0 || key.rfind(";", 0) == 0 || key.rfind("#", 0) == 0) {
          continue;
        }

        buffers.push_back(tmpl(
          R"JAVASCRIPT(__RAW_CONFIG__['{{key}}'] = '{{value}}')JAVASCRIPT",
          Map {{"key", key}, {"value", encodeURIComponent(value)}}
        ));
      }

      buffers.push_back(R"JAVASCRIPT(
        for (const key in __RAW_CONFIG__) {
          let value = __RAW_CONFIG__[key]
          try { value = decodeURIComponent(value) } catch {}
          globalThis.__args.config[key] = value
          if (key.startsWith('env_')) {
            globalThis.__args.env[key.slice(4)] = value
          }
        }
      )JAVASCRIPT");

      // 8. compile `globalThis.__args.env` values
      buffers.push_back(R"JAVASCRIPT(const __RAW_ENV__ = {})JAVASCRIPT");

      for (const auto& entry : this->options.env) {
        const auto key = trim(entry.first);
        const auto value = trim(entry.second);

        // skip empty keys and valaues
        if (key.size() == 0 || value.size() == 0) {
          continue;
        }

        buffers.push_back(tmpl(
          R"JAVASCRIPT(__RAW_ENV__['{{key}}'] = '{{value}}')JAVASCRIPT",
          Map {{"key", key}, {"value", encodeURIComponent(value)}}
        ));
      }

      buffers.push_back(R"JAVASCRIPT(
        for (const key in __RAW_ENV__) {
          let value = __RAW_ENV__[key]
          try { value = decodeURIComponent(value) } catch {}
          try { value = JSON.parse(value) } catch {}
          globalThis.__args.env[key] = value
        }
      )JAVASCRIPT");

      // 9. compile test script import if `options.features.useTestScript == true`
      if (this->options.features.useTestScript && this->options.index == 0) {
        String pathname;
        for (const auto& value : this->options.argv) {
          const auto start = value.find("--test=");
          if (start != String::npos) {
            auto end = value.find("'", start);

            if (end == String::npos) {
              end = value.size();
            }

            const auto file = value.substr(start + 7, end - start - 7);
            if (file.size() > 0) {
              pathname = file;
              break;
            }
          }
        }

        if (pathname.size() == 0) {
          buffers.push_back(R"JAVASCRIPT(
            console.warn('Preload: A test script entry was requested, but was not given')
          )JAVASCRIPT");
        } else {
          buffers.push_back(tmpl(
            R"JAVASCRIPT(
              Object.defineProperty(globalThis, 'RUNTIME_TEST_FILENAME', {
                configurable: false,
                enumerable: false,
                writable: false,
                value: String(new URL('{{pathname}}', globalThis.location.href)
              })
            )JAVASCRIPT",
            Map {{"pathname", pathname}}
          ));
        }
      }

      // 10. compile listeners for `globalThis`
      buffers.push_back(R"JAVASCRIPT(
        if (globalThis.document) {
          Object.defineProperties(globalThis, {
            RUNTIME_APPLICATION_URL_EVENT_BACKLOG: {
              configurable: false,
              enumerable: false,
              writable: false,
              value: []
            }
          })

          globalThis.addEventListener('applicationurl', (event) => {
            if (globalThis.document.readyState !== 'complete') {
              globalThis.RUNTIME_APPLICATION_URL_EVENT_BACKLOG.push(event)
            }
          })

          globalThis.addEventListener('__runtime_init__', () => {
            const backlog = globalThis.RUNTIME_APPLICATION_URL_EVENT_BACKLOG
            if (Array.isArray(backlog)) {
              for (const event of backlog) {
                if (typeof ApplicationURLEvent === 'function') {
                  globalThis.dispatchEvent(new ApplicationURLEvent(event.type, event))
                }
              }

              backlog.splice(0, backlog.length)
            }
          }, { once: true })

          if (
            globalThis.__args.config.webview_watch === true &&
            globalThis.__args.config.webview_watch_reload !== false
          ) {
            globalThis.addEventListener('filedidchange', () => {
              globalThis.location.reload()
            })
          }
        }
      )JAVASCRIPT");

      // 11. freeze `globalThis.__args` values
      buffers.push_back(R"JAVASCRIPT(
        Object.freeze(globalThis.__args.client)
        Object.freeze(globalThis.__args.config)
        Object.freeze(globalThis.__args.argv)
        Object.freeze(globalThis.__args.env)
      )JAVASCRIPT");
      buffers.push_back("})();");

      if (this->options.features.useHTMLMarkup) {
        buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_END_TAG);
      }

      // 12. compile preload `<script>` prefix if `options.features.useHTMLMarkup == true`, otherwise skip
      if (this->options.features.useHTMLMarkup) {
        if (this->options.features.useESM) {
          buffers.push_back(RUNTIME_PRELOAD_MODULE_BEGIN_TAG);
        } else {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG);
        }
      }

      // 13. compile "internal init" import
      if (this->options.features.useHTMLMarkup && this->options.features.useESM) {
        buffers.push_back(tmpl(
          R"JAVASCRIPT(
            import 'socket:internal/init'
            {{userScript}}
          )JAVASCRIPT",
          Map {{"userScript", this->options.userScript}}
        ));
      } else {
        buffers.push_back(";(() => {");
        buffers.push_back(tmpl(
          R"JAVASCRIPT(
            async function userScriptCallback () {
              {{userScript}}
            }

            if (globalThis.document && globalThis.document.readyState !== 'complete') {
              globalThis.document.addEventListener('readystatechange', () => {
                if(/interactive|complete/.test(globalThis.document.readyState)) {
                  import('socket:internal/init')
                    .then(userScriptCallback)
                    .catch(console.error)
                }
              })
            } else {
              import('socket:internal/init')
                .then(userScriptCallback)
                .catch(console.error)
            }
          )JAVASCRIPT",
          Map {{"userScript", this->options.userScript}}
        ));
        buffers.push_back("})();");
      }

      // 14. compile preload `</script>` prefix if `options.features.useHTMLMarkup == true`, otherwise skip
      if (this->options.features.useHTMLMarkup) {
        if (this->options.features.useESM) {
          buffers.push_back(RUNTIME_PRELOAD_MODULE_END_TAG);
        } else {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_END_TAG);
        }
      }

      // 15. compile "global CommonJS" if `options.features.useGlobalCommonJS == true`, otherwise skip
      if (this->options.features.useGlobalCommonJS) {
        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG);
        }

        buffers.push_back(R"JAVASCRIPT(
          if (globalThis.document) {
            ;(async function GlobalCommonJSScope () {
              const href = encodeURIComponent(globalThis.location.href)
              const source = `socket:module?ref=${href}`

              const { Module } = await import(source)
              const path = await import('socket:path')
              const require = Module.createRequire(globalThis.location.href)
              const __filename = Module.main.filename
              const __dirname = path.dirname(__filename)

              Object.defineProperties(globalThis, {
                module: {
                  configurable: true,
                  enumerable: false,
                  writable: false,
                  value: Module.main.scope
                },
                require: {
                  configurable: true,
                  enumerable: false,
                  writable: false,
                  value: require
                },
                __dirname: {
                  configurable: true,
                  enumerable: false,
                  writable: false,
                  value: __dirname
                },
                __filename: {
                  configurable: true,
                  enumerable: false,
                  writable: false,
                  value: __filename
                }
              })

              // reload global CommonJS scope on 'popstate' events
              globalThis.addEventListener('popstate', GlobalCommonJSScope)
            })();
          }
        )JAVASCRIPT");
        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_END_TAG);
        }
      }

      // 16. compile "global NodeJS" if `options.features.useGlobalNodeJS == true`, otherwise skip
      if (this->options.features.useGlobalNodeJS) {
        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG);
        }
        buffers.push_back(R"JAVASCRIPT(
          if (globalThis.document) {
            ;(async function GlobalNodeJSScope () {
              const process = await import('socket:process')
              Object.defineProperties(globalThis, {
                process: {
                  configurable: false,
                  enumerable: false,
                  writable: false,
                  value: process.default
                },

                global: {
                  configurable: false,
                  enumerable: false,
                  writable: false,
                  value: globalThis
                }
              })
            })();
          }
        )JAVASCRIPT");
        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_END_TAG);
        }
      }

      // 17. compile `__RUNTIME_PRIMORDIAL_OVERRIDES__` -- assumes value is a valid JSON string
      if (this->options.RUNTIME_PRIMORDIAL_OVERRIDES.size() > 0) {
        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_BEGIN_TAG);
        }

        buffers.push_back(tmpl(
          R"JAVASCRIPT(
            Object.defineProperty(globalThis, '__RUNTIME_PRIMORDIAL_OVERRIDES__', {
              configurable: false,
              enumerable: false,
              writable: false,
              value: {{RUNTIME_PRIMORDIAL_OVERRIDES}}
            })
          )JAVASCRIPT",
          Map {{"RUNTIME_PRIMORDIAL_OVERRIDES", this->options.RUNTIME_PRIMORDIAL_OVERRIDES}}
        ));

        if (this->options.features.useHTMLMarkup) {
          buffers.push_back(RUNTIME_PRELOAD_JAVASCRIPT_END_TAG);
        }
      }
    }

    // 18. compile preload `<meta name="end-runtime-preload">` "END" tag if `options.features.useHTMLMarkup == true`, otherwise skip
    if (this->options.features.useHTMLMarkup) {
      buffers.push_back(RUNTIME_PRELOAD_META_END_TAG);
    }

    // 19. clear existing compiled state
    this->compiled.clear();
    // 20. compile core buffers
    for (const auto& buffer : buffers) {
      this->compiled += buffer + "\n";
    }
    // 21. user preload buffers
    if (this->buffer.size() > 0) {
      this->compiled += ";(() => {\n";
      this->compiled += join(this->buffer, '\n');
      this->compiled += "})();\n";
    }
    if (this->options.features.useHTMLMarkup == false) {
      // 22. compile 'sourceURL' source map value if `options.features.useHTMLMarkup == false`
      this->compiled += "//# sourceURL=socket:<runtime>/preload.js";
    }
    return this->compiled;
  }

  const String& Preload::str () const {
    return this->compiled;
  }

  const String Preload::insertIntoHTML (
    const String& html,
    const InsertIntoHTMLOptions& options
  ) const {
    if (html.size() == 0) {
      return "";
    }

    auto protocolHandlerSchemes = options.protocolHandlerSchemes;
    auto userConfig = this->options.userConfig;
    auto preload = this->str();
    auto output = html;
    if (
      html.find(RUNTIME_PRELOAD_IMPORTMAP_BEGIN_TAG) == String::npos &&
      userConfig.contains("webview_importmap") &&
      userConfig.at("webview_importmap").size() > 0
    ) {
      auto resource = FileResource(Path(userConfig.at("webview_importmap")));

      if (resource.exists()) {
        const auto bytes = resource.read();

        if (bytes != nullptr) {
          preload = (
            tmpl(
              R"HTML(<script type="importmap">{{importmap}}</script>)HTML",
              Map {{"importmap", String(bytes, resource.size())}}
           ) + preload
          );
        }
      }
    }

    protocolHandlerSchemes.push_back("node:");
    protocolHandlerSchemes.push_back("npm:");

    output = tmpl(output, Map {
      {"protocol_handlers", join(protocolHandlerSchemes, " ")}
    });

    if (output.find("<meta name=\"runtime-preload-injection\" content=\"disabled\"") != String::npos) {
      preload = "";
    } else if (output.find("<meta content=\"disabled\" name=\"runtime-preload-injection\"") != String::npos) {
      preload = "";
    }

    const auto existingImportMapCursor = output.find(RUNTIME_PRELOAD_IMPORTMAP_BEGIN_TAG);
    bool preloadWasInjected = false;

    if (existingImportMapCursor != String::npos) {
      const auto closingScriptTag = output.find(
        RUNTIME_PRELOAD_JAVASCRIPT_END_TAG,
        existingImportMapCursor
      );

      if (closingScriptTag != String::npos) {
        output = (
          output.substr(0, closingScriptTag + 9) +
          preload +
          output.substr(closingScriptTag + 9)
        );

        preloadWasInjected = true;
      }
    }

    if (!preloadWasInjected) {
      if (output.find("<head>") != String::npos) {
        output = replace(output, "<head>", String("<head>" + preload));
      } else if (output.find("<body>") != String::npos) {
        output = replace(output, "<body>", String("<body>" + preload));
      } else if (output.find("<html>") != String::npos) {
        output = replace(output, "<html>", String("<html>" + preload));
      } else {
        output = preload + output;
      }
    }

    return output;
  }
}
