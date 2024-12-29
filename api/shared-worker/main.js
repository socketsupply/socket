import ipc from '../ipc.js'

const frames = new Map()
const channels = new Map()
const defaultHost = globalThis.location.origin

globalThis.addEventListener('connect', async (event) => {
  if (event.detail?.connect) {
    const url = new URL(event.detail.connect.scriptURL)
    const origin = url.origin || defaultHost
    const frame = (
      frames.get(origin) ||
      globalThis.document.createElement('iframe')
    )

    const channel = (
      channels.get(origin) ||
      new ipc.IPCBroadcastChannel('socket.runtime.sharedWorker', {
        origin
      })
    )

    if (!frames.has(origin)) {
      frame.src = new URL('/socket/shared-worker/frame.html', origin)
      frame.setAttribute('sandbox', [
        'allow-storage-access-by-user-activation',
        'allow-same-origin',
        'allow-scripts'
      ].join(' '))

      frame.ready = new Promise((resolve) => {
        frame.addEventListener('load', () => setTimeout(resolve, 100))
        setTimeout(resolve, 500)
      })

      globalThis.document.body.appendChild(frame)

      channels.set(origin, channel)
      frames.set(origin, frame)
    }

    await frame.ready

    channel.postMessage(event.detail)
  }
})
