window.parent.openExternal = o => {
  window.external.invoke(`ipc://external?value=${encodeURIComponent(o)}`)
}

window.parent.exit = o => window._ipc.send('exit', o)
window.parent.setTitle = o => window._ipc.send('title', o)
