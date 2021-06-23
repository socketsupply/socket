#include <string>

constexpr auto gPreload = R"JS(
;document.addEventListener('DOMContentLoaded', () => {
  window.external.invoke('ipc://ready');
});

const IPC = window._ipc = { nextSeq: 1 };

window._ipc.resolve = async (seq, status, value) => {
  value = decodeURIComponent(value)
  const method = status === 0 ? 'resolve' : 'reject';

  try {
    value = JSON.parse(value);
  } catch (err) {
    console.error(`${err.message} (${value})`)
    return
  }

  if (!window._ipc[seq] || !window._ipc[seq][method]) return
  await window._ipc[seq][method](value);
  delete window._ipc[seq];
}

window._ipc.send = (name, value) => {
  const seq = window._ipc.nextSeq++

  const promise = new Promise((resolve, reject) => {
    window._ipc[seq] = {
      resolve: resolve,
      reject: reject,
    }
  })

  try {
    if (typeof value === 'object') {
      value = JSON.stringify(value)
    }

    value = new URLSearchParams({
      index: window.process.index,
      seq,
      value
    }).toString()
  } catch (err) {
    console.error(`${err.message} (${value})`)
    return Promise.reject(err.message)
  }

  window.external.invoke(`ipc://${name}?${value}`)
  return promise
}

window._ipc.emit = (name, value) => {
  let detail

  try {
    detail = JSON.parse(decodeURIComponent(value))
  } catch (err) {
    console.error(`Unable to parse (${detail})`)
    return
  }

  const event = new window.CustomEvent(name, { detail })
  window.dispatchEvent(event)
}

window.system.send = value => {
  return window._ipc.send('request', value)
}

window.system.openExternal = value => {
  return window._ipc.send('external', value)
}

window.system.setTitle = value => {
  return window._ipc.send('title', { value })
}

window.system.dialog = value => {
  return window._ipc.send('dialog', value)
}

window.system.setContextMenu = value => {
  value = Object
    .entries(value)
    .flatMap(o => o.join(':'))
    .join('_')
  return window._ipc.send('context', value)
};
)JS";

struct PreloadOptions {
  int index;
  int debug;
  std::string title;
  std::string executable;
  std::string version;
  std::string argv;
  std::string toString();
};

std::string PreloadOptions::toString () {
  return std::string(
    "(() => {"
    "  window.system = {};\n"
    "  window.process = {};\n"
    "  window.process.index = Number('" + std::to_string(this->index) + "');\n"
    "  window.process.title = '" + this->title + "';\n"
    "  window.process.executable = '" + this->executable + "';\n"
    "  window.process.version = '" + this->version + "';\n"
    "  window.process.debug = " + std::to_string(this->debug) + ";\n"
    "  window.process.argv = [" + this->argv + "];\n"
    "  " + gPreload + "\n"
    "})()\n"
    "//# sourceURL=preload.js"
  );
}