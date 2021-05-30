#ifndef WEBVIEW_H
#define WEBVIEW_H

typedef void *webview_t;

#define WEBVIEW_HINT_NONE 0  // Width and height are default size
#define WEBVIEW_HINT_MIN 1   // Width and height are minimum bounds
#define WEBVIEW_HINT_MAX 2   // Width and height are maximum bounds
#define WEBVIEW_HINT_FIXED 3 // Window size can not be changed by a user

#ifndef WEBVIEW_HEADER

#if !defined(WEBVIEW_GTK) && !defined(WEBVIEW_COCOA) && !defined(WEBVIEW_EDGE)
#if defined(__linux__)
#define WEBVIEW_GTK
#elif defined(__APPLE__)
#define WEBVIEW_COCOA
#elif defined(_WIN32)
#define WEBVIEW_EDGE
#else
#error "please, specify webview backend"
#endif
#endif

#include <functional>
#include <map>
#include <string>

namespace Opkit {
  using dispatch_fn_t = std::function<void()>;
  std::map<std::string, std::string> appData;
} // namespace Opkit

#if defined(WEBVIEW_GTK)

#include "linux.h"

#elif defined(WEBVIEW_COCOA)

#include "darwin.h"

#elif defined(WEBVIEW_EDGE)

#include "win32.h"

#endif /* WEBVIEW_GTK, WEBVIEW_COCOA, WEBVIEW_EDGE */

namespace Opkit {

class webview : public browser_engine {
  public:
    webview(bool debug = false, void *wnd = nullptr)
      : browser_engine(debug, wnd) {}

    void navigate(const std::string url) {
      browser_engine::navigate(url);
    }

    using binding_t = std::function<void(std::string, std::string, void *)>;
    using binding_ctx_t = std::pair<binding_t *, void *>;

    using sync_binding_t = std::function<void(std::string, std::string)>;
    using sync_binding_ctx_t = std::pair<webview *, sync_binding_t>;

    void ipc(const std::string name, sync_binding_t fn) {
      ipc(
        name,
        [](std::string seq, std::string req, void *arg) {
          auto pair = static_cast<sync_binding_ctx_t *>(arg);
          pair->second(seq, req);
        },
        new sync_binding_ctx_t(this, fn)
      );
    }

    void ipc(const std::string name, binding_t f, void *arg) {
      auto js = "(function() { const name = '" + name + "';" + R"(
        const IPC = window._ipc = (window._ipc || { nextSeq: 1 });
        
        window.main = (window.main || {
          __resolve__: msg => {
            const data = msg.trim().split(';');
            const internal = data[0] === 'internal';
            const status = Number(data[1]);
            const seq = Number(data[2]);
            const method = status === 0 ? 'resolve' : 'reject';
            const value = internal ? data[3] : JSON.parse(decodeURIComponent(data[3]));

            if (!window._ipc[seq] || !window._ipc[seq][method]) return
            window._ipc[seq][method](value);
            window._ipc[seq] = undefined;
          },

          __send__: (name, value) => {
            const seq = IPC.nextSeq++
            const promise = new Promise((resolve, reject) => {
              IPC[seq] = {
                resolve: resolve,
                reject: reject,
              }
            })

            let encoded

            if (['setTitle', 'openExternal'].includes(name)) {
              encoded = value || ' '
            } else if (name === 'contextMenu') {
              encoded = Object
                .entries(value)
                .flatMap(o => o.join(':'))
                .join('_')
            } else {
              try {
                encoded = encodeURIComponent(JSON.stringify(value))
              } catch (err) {
                return Promise.reject(err.message)
              }
            }

            window.external.invoke(`ipc;${seq};${name};${encoded}`)
            return promise
          }
        });

        window.main[name] = value => window.main.__send__(name, value);
      })())";

      init(js);
      bindings[name] = new binding_ctx_t(new binding_t(f), arg);
    }

    void dialog(std::string seq) {
      dispatch([=]() {
        auto result = createNativeDialog(
          NOC_FILE_DIALOG_OPEN | NOC_FILE_DIALOG_DIR,
          NULL,
          NULL,
          NULL
        );

        eval(
          "(() => {"
          "  window._ipc[" + seq + "].resolve(`" + result + "`);"
          "  delete window._ipc[" + seq + "];"
          "})();"
        );
      });
    }

    void resolve(const std::string msg) {
      dispatch([=]() {
        eval("(() => { window.main.__resolve__(`" + msg + "`); })()");
      });
    }

    void emit(const std::string event, const std::string data) {
      dispatch([=]() {
        eval(
          "(() => {"
          "  let detail;"
          "  try {"
          "    detail = JSON.parse(decodeURIComponent(`" + data + "`));"
          "  } catch (err) {"
          "    console.error(`Unable to parse (${detail})`);"
          "    return;"
          "  }"
          "  const event = new window.CustomEvent('" + event + "', { detail });"
          "  window.dispatchEvent(event);"
          "})()"
        );
      });
    }

    void binding(const std::string msg) {
      auto parts = split(msg, ';');

      auto name = parts[0];
      auto args = replace(msg, "^\\w+;", "");

      if (bindings.find(name) == bindings.end()) {
        return;
      }

      auto fn = bindings[name];

      dispatch([=]() {
        (*fn->first)("0", args, fn->second);
      });
    }

  private:
    void on_message(const std::string msg) {
      auto parts = split(msg, ';');

      auto seq = parts[1];
      auto name = parts[2];
      auto args = parts[3];

      if (bindings.find(name) == bindings.end()) {
        return;
      }

      auto fn = bindings[name];
      (*fn->first)(seq, args, fn->second);
    }
    std::map<std::string, binding_ctx_t *> bindings;
  };
} // namespace Opkit

#endif /* WEBVIEW_HEADER */
#endif /* WEBVIEW_H */
