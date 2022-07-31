#ifndef PRELOAD_HH
#define PRELOAD_HH
//
// This file contains JavaScipt that are injected
// into the webview before any user code is executed.
//

constexpr auto gPreload = R"JS(
  const IPC = window._ipc = { nextSeq: 1, streams: {} }

  window._ipc.resolve = async (seq, status, value) => {
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

    if (!window._ipc[seq]) {
      console.error('inbound IPC message with unknown sequence:', seq, value)
      return
    }

    if (status === 0) {
      await window._ipc[seq].resolve(value)
    } else {
      const err = value instanceof Error
        ? value
        : value?.err instanceof Error
         ? value.err
         : new Error(typeof value === 'string' ? value : JSON.stringify(value))

      await window._ipc[seq].reject(err);
    }

    delete window._ipc[seq];
  }

  window._ipc.send = (name, o) => {
    const seq = 'R' + window._ipc.nextSeq++
    const index = window.process.index
    let serialized = ''

    const promise = new Promise((resolve, reject) => {
      window._ipc[seq] = {
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

  window._ipc.emit = (name, value, target, options) => {
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

      if (detail.event && detail.data && (detail.serverId || detail.clientId)) {
        const stream = window._ipc.streams[detail.serverId || detail.clientId]
        if (!stream) {
          console.error('inbound IPC message with unknown serverId/clientId:', detail)
          return
        }

        const value = detail.event === 'data' ? atob(detail.data) : detail.data

        if (detail.event === 'data') {
          stream.__write(detail.event, value)
        } else if (detail.event) {
          stream.emit(detail.event, value)
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

  window.parent.log = s => {
    window.external.invoke(`ipc://log?value=${s}`)
  }

  window.parent.getConfig = async o => {
    const config = await window._ipc.send('getConfig', o)
    if (!config || typeof config !== 'string') return null

    return decodeURIComponent(config)
      .split('\n')
      .map(trim)
      .filter(filterOutComments)
      .filter(filterOutEmptyLine)
      .map(splitTuple)
      .reduce(reduceTuple, {})

    function trim (line) { return line.trim() }
    function filterOutComments (line) { return !/^\s*#/.test(line) }
    function filterOutEmptyLine (line) { return line && line.length }

    function splitTuple (line) {
      return line.split(/:(.*)/).filter(filterOutEmptyLine).map(trim)
    }

    function reduceTuple (object, tuple) {
      try {
        return Object.assign(object, { [tuple[0]]: JSON.parse(tuple[1]) })
      } catch {
        return Object.assign(object, { [tuple[0]]: tuple[1] })
      }
    }
  }

  // initialize `XMLHttpRequest` IPC intercept
  void (() => {
    const { send, open } = XMLHttpRequest.prototype
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
              return send.call(this, null)
            }

            if (/linux/i.test(window.process?.platform)) {
              if (body?.buffer instanceof ArrayBuffer) {
                const type = new Uint8Array([0x62, 0x34]) // 'b4'
                const header = new Uint8Array(4)
                const buffer = new Uint8Array(
                  type.length +
                  header.length +
                  body.length
                )

                header.set(new TextEncoder().encode(index))
                header.set(new TextEncoder().encode(seq), 2)

                //  <type> |      <header>     | <body>
                // "b4"(2) | index(2) + seq(2) | body(n)
                buffer.set(type)
                buffer.set(header, type.length)
                buffer.set(body, type.length + header.length)

                let data = String.fromCharCode(...buffer)

                try { data = decodeURIComponent(escape(data)) }
                catch (err) { void err }

                await window.external.invoke(data)
              }

              return send.call(this, null)
            }
          }
        }

        return send.call(this, body)
      }
    })
  })();

  window.process.openFds.delete = async (id, shouldClose) => {
    if (shouldClose !== false) {
      return window.process.openFds.close(id)
    }

    return Map.prototype.delete.call(window.process.openFds, id)
  }

  window.process.openFds.close = async (id) => {
    await window._ipc.send('fsCloseOpenDescriptor', { id })
    return Map.prototype.delete.call(window.process.openFds, id)
  }

  window.process.openFds.clear = async (shouldClose) => {
    if (shouldClose !== false) {
      await window._ipc.send('fsCloseOpenDescriptors')
    }

    return Map.prototype.clear.call(window.process.openFds)
  }

  window.process.openFds.closeAll = async () => {
    return window.process.openFds.clear()
  }
)JS";

constexpr auto gPreloadDesktop = R"JS(
  window.system.rand64 = () => {
    return window.crypto.getRandomValues(new BigUint64Array(1))[0].toString()
  }

  window.system.send = o => {
    let value = ''

    try {
      value = JSON.stringify(o)
    } catch (err) {
      return Promise.reject(err.message)
    }

    return window._ipc.send('send', { value })
  }

  window.system.openExternal = o => window._ipc.send('external', o)
  window.system.exit = o => window._ipc.send('exit', o)
  window.system.setTitle = o => window._ipc.send('title', o)
  window.system.inspect = o => window.external.invoke(`ipc://inspect`)
  window.system.bootstrap = o => window.external.invoke(`ipc://bootstrap`)
  window.system.reload = o => window.external.invoke(`ipc://reload`)

  window.system.show = (index = 0) => {
    return window._ipc.send('show', { index })
  }

  window.system.hide = (index = 0) => {
    return window._ipc.send('hide', { index })
  }

  window.resizeTo = (width, height) => {
    const index = window.process.index
    const o = new URLSearchParams({ width, height, index }).toString()
    window.external.invoke(`ipc://size?${o}`)
  }

  window.system.setBackgroundColor = opts => {
    opts.index = window.process.index
    const o = new URLSearchParams(opts).toString()
    window.external.invoke(`ipc://background?${o}`)
  }

  window.system.setSystemMenuItemEnabled = value => {
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

  window.system.dialog = async o => {
    const files = await window._ipc.send('dialog', o);
    return typeof files === 'string' ? files.split('\n') : [];
  }

  window.system.setContextMenu = o => {
    o = Object
      .entries(o)
      .flatMap(a => a.join(':'))
      .join('_')
    return window._ipc.send('context', o)
  }

  window.system.setMenu = o => {
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

  window.parent = window.system

  if (window?.process?.port > 0) {
    window.addEventListener('menuItemSelected', e => {
      window.location.reload()
    })
  }
)JS";

constexpr auto gPreloadMobile = R"JS(
  window.system.openExternal = o => {
    window.external.invoke(`ipc://external?value=${encodeURIComponent(o)}`)
  }

  window.system.exit = o => window._ipc.send('exit', o)
  window.system.setTitle = o => window._ipc.send('title', o)
)JS";

#endif
