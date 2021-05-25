#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "platform.h"

#ifndef WEBVIEW_API
#define WEBVIEW_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef void *webview_t;

//
// Creates a new webview instance. If debug is non-zero - developer tools will
// be enabled (if the platform supports them). Window parameter can be a
// pointer to the native window handle. If it's non-null - then child WebView
// is embedded into the given parent window. Otherwise a new window is created.
// Depending on the platform, a GtkWindow, NSWindow or HWND pointer can be
// passed here.
//
WEBVIEW_API
  webview_t webview_create(int debug, void *window);

//
// Destroys a webview and closes the native window.
//
WEBVIEW_API
  void webview_destroy(webview_t w);

//
// Runs the main loop until it's terminated. After this function exits - you
// must destroy the webview.
//
WEBVIEW_API
  void webview_run(webview_t w);

//
// Stops the main loop. It is safe to call this function from another other
// background thread.
//
WEBVIEW_API
  void webview_terminate(webview_t w);

//
// Posts a function to be executed on the main thread. You normally do not need
// to call this function, unless you want to tweak the native window.
//
WEBVIEW_API
  void webview_dispatch(webview_t w, void (*fn)(webview_t w, void *arg), void *arg);

//
// Returns a native window handle pointer. When using GTK backend the pointer
// is GtkWindow pointer, when using Cocoa backend the pointer is NSWindow
// pointer, when using Win32 backend the pointer is HWND pointer.
//
WEBVIEW_API
  void *webview_get_window(webview_t w);

//
// Updates the title of the native window. Must be called from the UI thread.
//
WEBVIEW_API
  void webview_set_title(webview_t w, const char *title);

#define WEBVIEW_HINT_NONE 0  // Width and height are default size
#define WEBVIEW_HINT_MIN 1   // Width and height are minimum bounds
#define WEBVIEW_HINT_MAX 2   // Width and height are maximum bounds
#define WEBVIEW_HINT_FIXED 3 // Window size can not be changed by a user

//
// Updates native window size. See WEBVIEW_HINT constants.
//
WEBVIEW_API
  void webview_set_size(webview_t w, int width, int height, int hints);
//
// Navigates webview to the given URL. URL may be a data URI, i.e.
// "data:text/text,<html>...</html>". It is often ok not to url-encode it
// properly, webview will re-encode it for you.
//
WEBVIEW_API
  void webview_navigate(webview_t w, const char *url);

//
// Injects JavaScript code at the initialization of the new page. Every time
// the webview will open a the new page - this initialization code will be
// executed. It is guaranteed that code is executed before window.onload.
//
WEBVIEW_API
  void webview_init(webview_t w, const char *js);

//
// Evaluates arbitrary JavaScript code. Evaluation happens asynchronously, also
// the result of the expression is ignored. Use RPC bindings if you want to
// receive notifications about the results of the evaluation.
//
WEBVIEW_API
  void webview_eval(webview_t w, const char *js);

//
// Binds a native C callback so that it will appear under the given name as a
// global JavaScript function. Internally it uses webview_init(). Callback
// receives a request string and a user-provided argument pointer. Request
// string is a JSON array of all the arguments passed to the JavaScript
// function.
//
WEBVIEW_API
  void webview_ipc(
    webview_t w,
    const char *name,
    void (*fn)(const char *seq, const char *req, void *arg),
    void *arg
  );

//
// Allows to return a value from the native binding. Original request pointer
// must be provided to help internal RPC engine match requests with responses.
// If status is zero - result is expected to be a valid JSON result value.
// If status is not zero - result is an error JSON object.
//
WEBVIEW_API
  void webview_return(
    webview_t w,
    const char *seq,
    int status,
    const char *result
  );

#ifdef __cplusplus
}
#endif

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

#include <atomic>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include <cstring>

namespace Opkit {
using dispatch_fn_t = std::function<void()>;
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

      window[name] = value => {
        const seq = IPC.nextSeq++
        const promise = new Promise((resolve, reject) => {
          IPC[seq] = {
            resolve: resolve,
            reject: reject,
          }
        })

        let encoded

        if (name === 'setTitle') {
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
      eval(
        "(() => {"
        "  const data = `" + msg + "`.trim().split(';');"
        "  const internal = data[0] === 'internal';"
        "  const status = Number(data[1]);"
        "  const seq = Number(data[2]);"
        "  const method = status === 0 ? 'resolve' : 'reject';"
        "  const value = internal ? data[3] : JSON.parse(decodeURIComponent(data[3]));"
        "  window._ipc[seq][method](value);"
        "  window._ipc[seq] = undefined;"
        "})()"
      );
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

WEBVIEW_API webview_t webview_create(int debug, void *wnd) {
  return new Opkit::webview(debug, wnd);
}

WEBVIEW_API void webview_destroy(webview_t w) {
  delete static_cast<Opkit::webview *>(w);
}

WEBVIEW_API void webview_run(webview_t w) {
  static_cast<Opkit::webview *>(w)->run();
}

WEBVIEW_API void webview_terminate(webview_t w) {
  static_cast<Opkit::webview *>(w)->terminate();
}

WEBVIEW_API void webview_dispatch(
  webview_t w,
  void (*fn)(webview_t, void *),
  void *arg) {
    static_cast<Opkit::webview *>(w)->dispatch([=]() { fn(w, arg); });
}

WEBVIEW_API void *webview_get_window(webview_t w) {
  return static_cast<Opkit::webview *>(w)->window();
}

WEBVIEW_API void webview_set_title(webview_t w, const char *title) {
  static_cast<Opkit::webview *>(w)->setTitle(title);
}

WEBVIEW_API void webview_set_size(
  webview_t w,
  int width,
  int height,
  int hints) {
    static_cast<Opkit::webview *>(w)->setSize(width, height, hints);
}

WEBVIEW_API void webview_navigate(webview_t w, const char *url) {
  static_cast<Opkit::webview *>(w)->navigate(url);
}

WEBVIEW_API void webview_init(webview_t w, const char *js) {
  static_cast<Opkit::webview *>(w)->init(js);
}

WEBVIEW_API void webview_eval(webview_t w, const char *js) {
  static_cast<Opkit::webview *>(w)->eval(js);
}

WEBVIEW_API void webview_ipc(
  webview_t w,
  const char *name,
  void (*fn)(const char *seq, const char *req, void *arg),
  void *arg) {
  static_cast<Opkit::webview *>(w)->ipc(
    name,
    [=](std::string seq, std::string req, void *arg) {
      fn(seq.c_str(), req.c_str(), arg);
    },
    arg
  );
}

WEBVIEW_API void webview_return(
  webview_t w,
  const char *seq,
  int status,
  const char *result) {
    static_cast<Opkit::webview *>(w)->resolve(result);
}

#endif /* WEBVIEW_HEADER */
#endif /* WEBVIEW_H */
