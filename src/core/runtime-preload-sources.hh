#ifndef RUNTIME_PRELOAD_SOURCES_HH
#define RUNTIME_PRELOAD_SOURCES_HH

namespace SSC {
//
// THIS FILE WAS AUTO GENERATED ON: Tue Oct  4 11:29:46 +04 2022
//
// This file contains JavaScipt that is injected into the webview before
// any user code is executed.
//

// file= runtime.js
constexpr auto gPreload = R"JS(
;(() => {
window.parent = new class Parent {}
window.process = new class Process {
  arch = null
  argv = []
  config = new class ProcessConfig {
    get size () {
      return Object.keys(this).length
    }

    get (key) {
      if (typeof key !== 'string') throw new TypeError('Expecting key to be a string.')
      key = key.toLowerCase()
      return key in this ? this[key] : null
    }
  }

  debug = false
  env = {}
  executable = null
  index = 0
  port = 0
  title = null
  version = null

  cwd () {
    return null
  }
}

document.addEventListener('DOMContentLoaded', () => {
  queueMicrotask(async () => {
    try {
      const index = window.process?.index || 0
      const result = window.external.invoke(`ipc://event?value=domcontentloaded&index=${index}`)
      if (result.catch) result.catch(console.error)
    } catch (err) { console.error(err) }
  })
})

window._ipc = new class IPC {
  nextSeq = 1
  streams = {}

  async resolve (seq, status, value) {
    if (typeof value === 'string') {
      let didDecodeURIComponent = false
      try {
        value = decodeURIComponent(value)
        didDecodeURIComponent = true
      } catch (err) {
        console.error(`${err.message} (${value})`)
        return
      }

      try {
        value = JSON.parse(value)
      } catch (err) {
        if (!didDecodeURIComponent) {
          console.error(`${err.message} (${value})`)
          return
        }
      }
    }

    if (!this[seq]) {
      console.error('inbound IPC message with unknown sequence:', seq, value)
      return
    }

    if (status === 0) {
      await this[seq].resolve(value)
    } else {
      const err = value instanceof Error
        ? value
        : value?.err instanceof Error
        ? value.err
        : new Error(typeof value === 'string' ? value : JSON.stringify(value))

      await this[seq].reject(err);
    }

    delete this[seq];
  }

  send (name, value) {
    const seq = 'R' + this.nextSeq++
    const index = window.process.index
    let serialized = ''

    const promise = new Promise((resolve, reject) => {
      this[seq] = {
        resolve: resolve,
        reject: reject
      }
    })

    try {
      if (({}).toString.call(value) !== '[object Object]') {
        value = { value }
      }

      const params = {
        ...value,
        index,
        seq
      }

      serialized = new URLSearchParams(params).toString()
      serialized = serialized.replace(/\+/g, '%20')

    } catch (err) {
      console.error(`${err.message} (${serialized})`)
      return Promise.reject(err.message)
    }

    window.external.invoke(`ipc://${name}?${serialized}`)

    return Object.assign(promise, { index, seq })
  }

  emit (name, value, target, options) {
    let detail = value

    if (typeof value === 'string') {
      try {
        detail = decodeURIComponent(value)
        detail = JSON.parse(detail)
      } catch (err) {
        // consider okay here because if detail is defined then
        // `decodeURIComponent(value)` was successful and `JSON.parse(value)`
        // was not: there could be bad/unsupported unicode in `value`
        if (!detail) {
          console.error(`${err.message} (${value})`)
          return
        }
      }
    }

    const event = new window.CustomEvent(name, { detail, ...options })
    if (target) {
      target.dispatchEvent(event)
    } else {
      window.dispatchEvent(event)
    }
  }
}

if (process.platform !== 'linux') {
  const clog = console.log
  const cerr = console.error

  console.log = (...args) => {
    clog(...args)
    window.external.invoke(`ipc://stdout?value=${args}`)
  }

  console.error = (...args) => {
    cerr(...args)
    window.external.invoke(`ipc://stderr?value=${args}`)
  }
}

// initialize `XMLHttpRequest` IPC intercept
void (() => {
  const { send, open } = XMLHttpRequest.prototype

  const B5_PREFIX_BUFFER = new Uint8Array([0x62, 0x35]) // literally, 'b5'
  const encoder = new TextEncoder()
  Object.assign(XMLHttpRequest.prototype, {
    open (method, url, ...args) {
      this.readyState = XMLHttpRequest.OPENED
      this.method = method
      this.url = new URL(url)
      this.seq = this.url.searchParams.get('seq')

      return open.call(this, method, url, ...args)
    },

    async send (body) {
      const { method, seq, url } = this
      const index = window.process.index

      if (url?.protocol === 'ipc:') {
        if (
          /put|post/i.test(method) &&
          typeof body !== 'undefined' &&
          typeof seq !== 'undefined'
        ) {
          if (/android/i.test(window.process?.platform)) {
            await window.external.invoke(`ipc://buffer.queue?seq=${seq}`, body)
            body = null
          }

          if (/linux/i.test(window.process?.platform)) {
            if (body?.buffer instanceof ArrayBuffer) {
              const header = new Uint8Array(24)
              const buffer = new Uint8Array(
                B5_PREFIX_BUFFER.length +
                header.length +
                body.length
              )

              header.set(encoder.encode(index))
              header.set(encoder.encode(seq), 4)

              //  <type> |      <header>     | <body>
              // "b5"(2) | index(2) + seq(2) | body(n)
              buffer.set(B5_PREFIX_BUFFER)
              buffer.set(header, B5_PREFIX_BUFFER.length)
              buffer.set(body, B5_PREFIX_BUFFER.length + header.length)

              let data = []
              const quota = 64 * 1024
              for (let i = 0; i < buffer.length; i += quota) {
                data.push(String.fromCharCode(...buffer.subarray(i, i + quota)))
              }

              data = data.join('')

              try { data = decodeURIComponent(escape(data)) }
              catch (err) { void err }

              await window.external.invoke(data)
            }

            body = null
          }
        }
      }

      return send.call(this, body)
    }
  })
})();
})();
//# sourceURL=runtime.js
)JS";

// file= desktop.js
constexpr auto gPreloadDesktop = R"JS(
;(() => {
window.parent.send = o => {
  let value = ''

  try {
    value = JSON.stringify(o)
  } catch (err) {
    return Promise.reject(err.message)
  }

  return window._ipc.send('send', { value })
}

window.parent.openExternal = o => window._ipc.send('external', o)
window.parent.exit = o => window._ipc.send('exit', o)
window.parent.setTitle = o => window._ipc.send('title', o)
window.parent.inspect = o => window.external.invoke(`ipc://inspect`)

window.parent.reload = o => window.external.invoke(`ipc://reload`)

window.parent.show = (index = 0) => {
  return window._ipc.send('show', { index })
}

window.parent.hide = (index = 0) => {
  return window._ipc.send('hide', { index })
}

window.resizeTo = (width, height) => {
  const index = window.process.index
  const o = new URLSearchParams({ width, height, index }).toString()
  window.external.invoke(`ipc://size?${o}`)
}

window.parent.setBackgroundColor = opts => {
  opts.index = window.process.index
  const o = new URLSearchParams(opts).toString()
  window.external.invoke(`ipc://background?${o}`)
}

window.parent.setSystemMenuItemEnabled = value => {
  return window._ipc.send('systemMenuItemEnabled', value)
}

Object.defineProperty(window.document, 'title', {
  get () { return window.process.title },
  set (value) {
    const index = window.process.index
    const o = new URLSearchParams({ value, index }).toString()
    window.external.invoke(`ipc://title?${o}`)
  }
})

window.parent.dialog = async o => {
  const files = await window._ipc.send('dialog', o);
  return typeof files === 'string' ? files.split('\n') : [];
}

window.parent.setContextMenu = o => {
  o = Object
    .entries(o)
    .flatMap(a => a.join(':'))
    .join('_')
  return window._ipc.send('context', o)
}

window.parent.setMenu = o => {
  const menu = o.value

  // validate the menu
  if (typeof menu !== 'string' || menu.trim().length === 0) {
    throw new Error('Menu must be a non-empty string')
  }

  const menus = menu.match(/\w+:\n/g)
  if (!menus) {
    throw new Error('Menu must have a valid format')
  }
  const menuTerminals = menu.match(/;/g)
  const delta = menus.length - (menuTerminals?.length ?? 0)

  if ((delta !== 0) && (delta !== -1)) {
    throw new Error(`Expected ${menuTerminals.length} ';', found ${menus}.`)
  }

  const lines = menu.split('\n')
  const e = new Error()
  const frame = e.stack.split('\n')[2]
  const callerLineNo = frame.split(':').reverse()[1]

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i]
    const l = Number(callerLineNo) + i

    let errMsg

    if (line.trim().length === 0) continue
    if (/.*:\n/.test(line)) continue // ignore submenu labels
    if (/---/.test(line)) continue // ignore separators
    if (/\w+/.test(line) && !line.includes(':')) {
      errMsg = 'Missing label'
    } else if (/:\s*\+/.test(line)) {
      errMsg = 'Missing accelerator'
    } else if (/\+(\n|$)/.test(line)) {
      errMsg = 'Missing modifier'
    }

    if (errMsg) {
      throw new Error(`${errMsg} on line ${l}: "${line}"`)
    }
  }

  // send the request to set the menu
  return window._ipc.send('menu', o)
}

if (window?.process?.port > 0) {
  window.addEventListener('menuItemSelected', e => {
    window.location.reload()
  })
}
})();
//# sourceURL=desktop.js
)JS";

// file= mobile.js
constexpr auto gPreloadMobile = R"JS(
;(() => {
window.parent.openExternal = o => {
  window.external.invoke(`ipc://external?value=${encodeURIComponent(o)}`)
}

window.parent.exit = o => window._ipc.send('exit', o)
window.parent.setTitle = o => window._ipc.send('title', o)
})();
//# sourceURL=mobile.js
)JS";

} // namespace SSC
#endif
