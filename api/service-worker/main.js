import hooks from '../hooks.js'
import ipc from '../ipc.js'

const frames = new Map()
const channels = new Map()
const defaultHost = globalThis.location.origin

globalThis.top.addEventListener('serviceWorker.register', onRegister)
globalThis.top.addEventListener('serviceWorker.unregister', onUnregister)
globalThis.top.addEventListener('serviceWorker.skipWaiting', onSkipWaiting)
globalThis.top.addEventListener('serviceWorker.activate', onActivate)
globalThis.top.addEventListener('serviceWorker.fetch', onFetch)

hooks.onReady(async () => {
  // notify top frame that the service worker init module is ready
  globalThis.top.postMessage({
    __service_worker_frame_init: true
  }, '*')

  const result = await ipc.request('serviceWorker.getRegistrations')
  if (Array.isArray(result.data)) {
    for (const info of result.data) {
      await navigator.serviceWorker.register(info.scriptURL, info)
    }
  }
})

async function onRegister (event) {
  const url = new URL(event.detail.scriptURL)
  const origin = url.origin || defaultHost
  const frame = (
    frames.get(origin) ||
    globalThis.document.createElement('iframe')
  )

  const channel = (
    channels.get(origin) ||
    new ipc.IPCBroadcastChannel('socket.runtime.serviceWorker', {
      origin
    })
  )

  if (!frames.has(origin)) {
    frame.src = new URL('/socket/service-worker/frame.html', origin)
    frame.setAttribute('sandbox', [
      'allow-storage-access-by-user-activation',
      'allow-same-origin',
      'allow-scripts'
    ].join(' '))

    frame.ready = new Promise((resolve) => {
      frame.addEventListener('load', () => setTimeout(resolve, 500))
    })

    globalThis.document.body.appendChild(frame)

    channels.set(origin, channel)
    frames.set(origin, frame)
  }

  await frame.ready

  channel.postMessage({
    type: 'serviceWorker.register',
    detail: event.detail
  })
}

async function onUnregister (event) {
  const url = new URL(event.detail.scriptURL)
  const origin = url.origin || defaultHost
  const frame = frames.get(origin)
  const channel = channels.get(origin)

  if (!frame || !channel) {
    return
  }

  await frame.ready

  channel.postMessage({
    type: 'serviceWorker.unregister',
    detail: event.detail
  })
}

async function onSkipWaiting (event) {
  const url = new URL(event.detail.scriptURL)
  const origin = url.origin || defaultHost
  const frame = frames.get(origin)
  const channel = channels.get(origin)

  if (!frame || !channel) {
    return
  }

  await frame.ready
  channel.postMessage({
    type: 'serviceWorker.skipWaiting',
    detail: event.detail
  })
}

async function onActivate (event) {
  const url = new URL(event.detail.scriptURL)
  const origin = url.origin || defaultHost
  const frame = frames.get(origin)
  const channel = channels.get(origin)

  if (!frame || !channel) {
    return
  }

  await frame.ready

  channel.postMessage({
    type: 'serviceWorker.activate',
    detail: event.detail
  })
}

async function onFetch (event) {
  const url = new URL(event.detail.scriptURL)
  const origin = url.origin || defaultHost
  const frame = (
    frames.get(origin) ||
    globalThis.document.createElement('iframe')
  )

  const channel = (
    channels.get(origin) ||
    new ipc.IPCBroadcastChannel('socket.runtime.serviceWorker', {
      origin
    })
  )

  if (!frames.has(origin)) {
    frames.set(origin, frame)
    channels.set(origin, channel)

    frame.src = new URL('/socket/service-worker/frame.html', origin)
    frame.setAttribute('sandbox', [
      'allow-storage-access-by-user-activation',
      'allow-same-origin',
      'allow-scripts'
    ].join(' '))

    globalThis.document.body.appendChild(frame)

    frame.ready = new Promise((resolve) => {
      frame.addEventListener('load', () => setTimeout(resolve, 500))
    })
  }

  await frame.ready

  channel.postMessage({
    type: 'serviceWorker.fetch',
    detail: event.detail
  })
}
