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

window.parent.show = (index = 0) => {
  return window._ipc.send('show', { index })
}

window.parent.hide = (index = 0) => {
  return window._ipc.send('hide', { index })
}

window.resizeTo = (width, height) => {
  const index = window.parent.index
  const o = new URLSearchParams({ width, height, index }).toString()
  window.external.invoke(`ipc://size?${o}`)
}

window.parent.setBackgroundColor = opts => {
  opts.index = window.parent.index
  const o = new URLSearchParams(opts).toString()
  window.external.invoke(`ipc://background?${o}`)
}

window.parent.setSystemMenuItemEnabled = value => {
  return window._ipc.send('systemMenuItemEnabled', value)
}

Object.defineProperty(window.document, 'title', {
  get () { return window.parent.title },
  set (value) {
    const index = window.parent.index
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

if (window?.parent?.port > 0) {
  window.addEventListener('menuItemSelected', e => {
    window.location.reload()
  })
}

window._ipc.send('process.open')
