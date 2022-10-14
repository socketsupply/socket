window.parent = new class Parent {
  arch = null
  argv = []
  debug = false
  env = {}
  executable = null
  index = 0
  port = 0
  title = null
  version = null

  config = new class ProcessConfig {
    get size () {
      return Object.keys(this).length
    }

    get (key) {
      if (typeof key !== 'string') {
        throw new TypeError('Expecting key to be a string.')
      }

      key = key.toLowerCase()
      return key in this ? this[key] : null
    }
  }

  // overloaded in process
  cwd () {
    return null
  }
}

document.addEventListener('DOMContentLoaded', () => {
  queueMicrotask(async () => {
    try {
      const index = window.parent?.index || 0
      const result = await window.external.invoke(`ipc://event?value=domcontentloaded&index=${index}`)
    } catch (err) {
      console.error(err)
    }
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
    const index = window.parent.index
    let serialized = ''

    const promise = new Promise((resolve, reject) => {
      this[seq] = {
        resolve: resolve,
        reject: reject
      }
    })

    try {
      if (value !== undefined && ({}).toString.call(value) !== '[object Object]') {
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

if (window.parent.platform !== 'linux') {
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
      const index = window.parent.index

      if (url?.protocol === 'ipc:') {
        if (
          /put|post/i.test(method) &&
          typeof body !== 'undefined' &&
          typeof seq !== 'undefined'
        ) {
          if (/android/i.test(window.parent?.platform)) {
            await window.external.invoke(`ipc://buffer.queue?seq=${seq}`, body)
            body = null
          }

          if (/linux/i.test(window.parent?.platform)) {
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
