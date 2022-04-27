#ifndef PRELOAD_HH
#define PRELOAD_HH

constexpr auto gPreload = R"JS(
  const IPC = window._ipc = { nextSeq: 1, streams: {} }

  window._ipc.resolve = async (seq, status, value) => {
    try {
      value = decodeURIComponent(value)
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    try {
      value = JSON.parse(value)
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    if (!window._ipc[seq]) {
      console.error('inbound IPC message with unknown sequence:', seq, value)
      return
    }

    if (status === 0) {
      await window._ipc[seq].resolve(value)
    } else {
      const err = new Error(JSON.stringify(value))
      await window._ipc[seq].reject(err);
    }
    delete window._ipc[seq];
  }

  window._ipc.send = (name, o) => {
    const seq = window._ipc.nextSeq++
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
        index: window.process.index,
        seq
      }

      serialized = new URLSearchParams(params).toString()

      serialized = serialized.replace(/\+/g, '%20')

    } catch (err) {
      console.error(`${err.message} (${serialized})`)
      return Promise.reject(err.message)
    }

    window.external.invoke(`ipc://${name}?${serialized}`)
    return promise
  }

  window._ipc.emit = (name, value, target, options) => {
    let detail

    try {
      detail = JSON.parse(decodeURIComponent(value))
    } catch (err) {
      console.error(`${err.message} (${value})`)
      return
    }

    if (detail.event && detail.data && (detail.serverId || detail.clientId)) {
      const stream = window._ipc.streams[detail.serverId || detail.clientId]
      if (!stream) {
        console.error('inbound IPC message with unknown serverId/clientId:', detail)
        return
      }

      const value = detail.event === 'data' ? atob(detail.data) : detail.data

      if (details.event === 'data') {
        stream.__write(detail.event, value)
      } else {
        stream.emit(detail.event, value)
      }
    }

    const event = new window.CustomEvent(name, { detail, ...options })
    if (target) {
      target.dispatchEvent(event)
    } else {
      window.dispatchEvent(event)
    }
  }
)JS";

constexpr auto gPreloadDesktop = R"JS(
  window.system.send = o => {
    let value = ''

    try {
      value = JSON.stringify(o)
    } catch (err) {
      return Promise.reject(err.message)
    }

    return window._ipc.send('send', { value })
  }

  window.system.exit = o => window._ipc.send('exit', o)
  window.system.openExternal = o => window._ipc.send('external', o)
  window.system.setTitle = o => window._ipc.send('title', o)
  window.system.inspect = o => window.external.invoke(`ipc://inspect`)

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
  };

  window.parent = window.system

  if (window?.process?.debug === 1) {
    window.addEventListener('menuItemSelected', e => {
      window.location.reload()
    })
  }
)JS";

constexpr auto gPreloadMobile = R"JS(
  () => {
    window.system.getNetworkInterfaces = o => window._ipc.send('getNetworkInterfaces', o)
    window.system.openExternal = o => window._ipc.send('external', o)
  })();
)JS";

#endif

