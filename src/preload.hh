constexpr auto gPreload = R"JS(
  ;document.addEventListener('DOMContentLoaded', () => {
    // window.external.invoke('ipc://ready');
  })

  const IPC = window._ipc = { nextSeq: 1 }

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

  window._ipc.send = (name, value) => {
    const seq = window._ipc.nextSeq++

    const promise = new Promise((resolve, reject) => {
      window._ipc[seq] = {
        resolve: resolve,
        reject: reject
      }
    })

    try {
      let rawValue = value
      if (typeof value === 'object') {
        value = JSON.stringify(value)
      }

      value = new URLSearchParams({
        ...rawValue,
        index: window.process.index,
        seq,
        value
      }).toString()

      value = value.replace(/\+/g, '%20')

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
      console.error(`${err.message} (${value})`)
      return
    }

    const event = new window.CustomEvent(name, { detail })
    window.dispatchEvent(event)
  }

  window.system.send = value => {
    return window._ipc.send('send', value)
  }

  window.system.exit = (value) => {
    return window._ipc.send('exit', value)
  }

  window.system.openExternal = value => {
    return window._ipc.send('external', value)
  }

  window.system.setTitle = value => {
    return window._ipc.send('title', value)
  }

  window.system.hide = value => {
    return window._ipc.send('hide', value)
  }

  window.system.dialog = async (value) => {
    const files = await window._ipc.send('dialog', value);
    return files.split('\n');
  }

  window.system.setContextMenu = value => {
    value = Object
      .entries(value)
      .flatMap(o => o.join(':'))
      .join('_')
    return window._ipc.send('context', value)
  };
)JS";
