const IPC = window._ipc = { nextSeq: 1, streams: {} }

document.addEventListener('DOMContentLoaded', () => {
  queueMicrotask(async () => {
    try {
      const index = window.process?.index || 0
      const result = window.external.invoke(`ipc://event?value=domcontentloaded&index=${index}`)
      if (result.catch) result.catch(console.error)
    } catch (err) { console.error(err) }
  })
})


IPC.resolve = async (seq, status, value) => {
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

  if (!IPC[seq]) {
    console.error('inbound IPC message with unknown sequence:', seq, value)
    return
  }

  if (status === 0) {
    await IPC[seq].resolve(value)
  } else {
    const err = value instanceof Error
      ? value
      : value?.err instanceof Error
      ? value.err
      : new Error(typeof value === 'string' ? value : JSON.stringify(value))

    await IPC[seq].reject(err);
  }

  delete IPC[seq];
}

IPC.send = (name, o) => {
  const seq = 'R' + IPC.nextSeq++
  const index = window.process.index
  let serialized = ''

  const promise = new Promise((resolve, reject) => {
    IPC[seq] = {
      resolve: resolve,
      reject: reject
    }
  })

  try {
    if (({}).toString.call(o) !== '[object Object]') {
      o = { value: o }
    }

    const params = {
      ...o,
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

IPC.emit = (name, value, target, options) => {
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
