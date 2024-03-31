/* global Worker */
import { sleep } from '../timers.js'
import crypto from '../crypto.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

export const workers = new Map()

export class ServiceWorkerInfo {
  id = null
  url = null
  hash = null
  scope = null
  scriptURL = null

  constructor (data) {
    for (const key in data) {
      const value = data[key]
      if (key in this) {
        this[key] = value
      }
    }

    const url = new URL(this.scriptURL)
    this.url = url.toString()
    this.hash = crypto.murmur3(url.pathname + (this.scope || ''))
  }
}

export async function onRegister (event) {
  const info = new ServiceWorkerInfo(event.detail)

  if (!info.id || workers.has(info.hash)) {
    return
  }

  const worker = new Worker('./worker.js')

  workers.set(info.hash, worker)
  worker.addEventListener('message', onHandShakeMessage)

  async function onHandShakeMessage (event) {
    if (event.data.__service_worker_ready === true) {
      worker.postMessage({ register: info })
    } else if (event.data.__service_worker_registered?.id === info.id) {
      worker.postMessage({ install: info })
      worker.removeEventListener('message', onHandShakeMessage)
    }
  }
}

export async function onUnregister (event) {
  const info = new ServiceWorkerInfo(event.detail)

  if (!workers.has(info.hash)) {
    return
  }

  const worker = workers.get(info.hash)
  workers.delete(info.hash)

  worker.postMessage({ activate: info })
}

export async function onSkipWaiting (event) {
  onActivate(event)
}

export async function onActivate (event) {
  const info = new ServiceWorkerInfo(event.detail)

  if (!workers.has(info.hash)) {
    return
  }

  const worker = workers.get(info.hash)

  worker.postMessage({ activate: info })
}

export async function onFetch (event) {
  const info = new ServiceWorkerInfo(event.detail)
  const exists = workers.has(info.hash)

  // this may be an early fetch, just try waiting at most
  // 32*16 milliseconds for the worker to be available or
  // the 'activate' event before generating a 404 response
  await Promise.race([
    (async function () {
      let retries = 16
      while (!workers.has(info.hash) && --retries > 0) {
        await sleep(32)
      }

      if (!exists) {
        await sleep(256)
      }
    }()),
    new Promise((resolve) => {
      globalThis.top.addEventListener('serviceWorker.activate', async function onActivate (event) {
        const { hash } = new ServiceWorkerInfo(event.detail)
        if (hash === info.hash) {
          await sleep(64)
          resolve()
        }
      })
    })
  ])

  if (!workers.has(info.hash)) {
    const options = {
      statusCode: 404,
      clientId: event.detail.fetch.client.id,
      headers: '',
      id: event.detail.fetch.id
    }

    await ipc.write('serviceWorker.fetch.response', options)
    return
  }

  const client = event.detail.fetch.client ?? {}
  const request = {
    id: event.detail.fetch.id,
    url: new URL(
      event.detail.fetch.pathname + '?' + event.detail.fetch.query,
      globalThis.top.origin
    ).toString(),

    headers: event.detail.fetch.headers
      .map((entry) => entry.split(':'))
      .reduce((object, entry) => Object.assign(object, { [entry[0]]: entry[1].trim() }), {})
  }

  const worker = workers.get(info.hash)

  worker.postMessage({ fetch: { ...info, client, request } })
}

export default null

globalThis.top.addEventListener('serviceWorker.register', onRegister)
globalThis.top.addEventListener('serviceWorker.unregister', onUnregister)
globalThis.top.addEventListener('serviceWorker.skipWaiting', onSkipWaiting)
globalThis.top.addEventListener('serviceWorker.activate', onActivate)
globalThis.top.addEventListener('serviceWorker.fetch', onFetch)

hooks.onReady(() => {
  // notify top frame that the service worker init module is ready
  globalThis.top.postMessage({
    __service_worker_frame_init: true
  })
})
