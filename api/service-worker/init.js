/* global Worker */
import { sleep } from '../timers.js'
import globals from '../internal/globals.js'
import crypto from '../crypto.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

export const workers = new Map()

globals.register('ServiceWorkerContext.workers', workers)
globals.register('ServiceWorkerContext.info', new Map())

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

    globals.get('ServiceWorkerContext.info').set(this.hash, this)
  }

  get pathname () {
    return new URL(this.url).pathname
  }
}

export async function onRegister (event) {
  const info = new ServiceWorkerInfo(event.detail)

  if (!info.id || workers.has(info.hash)) {
    return
  }

  const worker = new Worker('./worker.js', {
    name: `ServiceWorker (${info.pathname})`,
    [Symbol.for('socket.runtime.internal.worker.type')]: 'serviceWorker'
  })

  workers.set(info.hash, worker)
  worker.addEventListener('message', onMessage)

  async function onMessage (event) {
    if (event.data.__service_worker_ready === true) {
      worker.postMessage({ register: info })
    } else if (event.data.__service_worker_registered?.id === info.id) {
      worker.postMessage({ install: info })
    } else if (Array.isArray(event.data.__service_worker_debug)) {
      const log = document.querySelector('#log')
      if (log) {
        for (const entry of event.data.__service_worker_debug) {
          const lines = entry.split('\n')
          const span = document.createElement('span')
          for (const line of lines) {
            if (!line) continue
            const item = document.createElement('code')
            item.innerHTML = line
              .replace(/\s/g, '&nbsp;')
              .replace(/(Error|TypeError|SyntaxError|ReferenceError|RangeError)/g, '<span class="red">$1</span>')
            span.appendChild(item)
          }
          log.appendChild(span)
        }

        log.scrollTop = log.scrollHeight
      }
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
    })(),
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
      (
        event.detail.fetch.pathname +
        (event.detail.fetch.query.length ? '?' + event.detail.fetch.query : '')
      ),
      `${event.detail.fetch.scheme}://${event.detail.fetch.host}`
    ).toString(),

    method: event.detail.fetch.method,
    headers: event.detail.fetch.headers
      .map((entry) => entry.split(':'))
      .reduce((object, entry) => Object.assign(object, { [entry[0]]: entry.slice(1).join(':').trim() }), {})
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

hooks.onReady(async () => {
  // notify top frame that the service worker init module is ready
  globalThis.top.postMessage({
    __service_worker_frame_init: true
  })

  const result = await ipc.request('serviceWorker.getRegistrations')
  if (Array.isArray(result.data)) {
    for (const info of result.data) {
      await navigator.serviceWorker.register(info.scriptURL, info)
    }
  }
})
