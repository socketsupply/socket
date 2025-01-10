/* global Worker */
import { Notification } from '../notification.js'
import { sleep } from '../timers.js'
import globals from '../internal/globals.js'
import crypto from '../crypto.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'

export const workers = new Map()
export const channel = new BroadcastChannel('socket.runtime.serviceWorker')

globals.register('ServiceWorkerContext.workers', workers)
globals.register('ServiceWorkerContext.info', new Map())

channel.onmessage = (event) => {
  if (event.data?.from === 'instance' && event.data.registration?.id) {
    for (const worker of workers.values()) {
      if (worker.info.id === event.data.registration.id) {
        worker.postMessage(event.data)
        break
      }
    }
  } else if (event.data?.showNotification && event.data.registration?.id) {
    onNotificationShow(event)
  } else if (event.data?.getNotifications && event.data.registration?.id) {
    onGetNotifications(event)
  }
}

export class ServiceWorkerInstance extends Worker {
  #info = null
  #notifications = []

  constructor (filename, options) {
    options = { ...options }
    const info = options.info ?? null
    if (info.serializedWorkerArgs) {
      try {
        options.args = JSON.parse(info.serializedWorkerArgs)
      } catch {
        try {
          options.args = JSON.parse(decodeURIComponent(info.serializedWorkerArgs))
        } catch {
          options.args = JSON.parse(decodeURIComponent(decodeURIComponent(info.serializedWorkerArgs)))
        }
      }

      if (options?.args?.index) {
        options.args.index = globalThis.__args.index
      }
    }

    super(filename, {
      name: `ServiceWorker (${options?.info?.pathname ?? filename})`,
      ...options,
      [Symbol.for('socket.runtime.internal.worker.type')]: 'serviceWorker'
    })

    this.#info = info
    this.addEventListener('message', this.onMessage.bind(this))
  }

  get info () {
    return this.#info
  }

  get notifications () {
    return this.#notifications
  }

  async onMessage (event) {
    const { info } = this
    if (event.data.__service_worker_ready === true) {
      this.postMessage({ register: info })
    } else if (event.data.__service_worker_registered?.id === info.id) {
      this.postMessage({ install: info })
      info.promise.resolve()
    } else if (event.data?.message && event?.data.client?.id) {
      channel.postMessage({
        ...event.data,
        from: 'realm'
      })
    } else if (Array.isArray(event.data.__service_worker_debug)) {
      const log = document.querySelector('#log')
      if (log) {
        for (const entry of event.data.__service_worker_debug) {
          const lines = entry.split('\n')
          const span = document.createElement('span')
          let target = span

          for (const line of lines) {
            if (!line) continue
            const item = document.createElement('code')
            item.innerHTML = line
              .replace(/\s/g, '&nbsp;')
              .replace(/\\s/g, ' ')
              .replace(/<anonymous>/g, '&lt;anonymous&gt;')
              .replace(/([a-z|A-Z|_|0-9]+(Error|Exception)):/g, '<span class="red"><b>$1</b>:</span>')

            if (target === span && lines.length > 1) {
              target = document.createElement('details')
              const summary = document.createElement('summary')
              summary.appendChild(item)
              target.appendChild(summary)
              span.appendChild(target)
            } else {
              target.appendChild(item)
            }
          }

          log.appendChild(span)
        }

        log.scrollTop = log.scrollHeight
      }
    } else if (event.data?.showNotification && event.data.registration?.id) {
      onNotificationShow(event, this)
    } else if (event.data?.getNotifications && event.data.registration?.id) {
      onGetNotifications(event, this)
    } else if (event.data?.notificationclose?.id) {
      onNotificationClose(event, this)
    }
  }
}

export class ServiceWorkerInfo {
  id = null
  url = null
  hash = null
  scope = null
  scriptURL = null
  serializedWorkerArgs = null
  priority = 'default'
  // @ts-ignore
  #promise = Promise.withResolvers()

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

  get pathname () {
    return new URL(this.url).pathname
  }

  get promise () {
    return this.#promise
  }
}

export async function onRegister (event) {
  const info = new ServiceWorkerInfo(event.detail)

  if (!info.id || workers.has(info.hash)) {
    return
  }

  for (const worker of workers.values()) {
    if (worker.priority === 'high' && /high|default|low/i.test(info.priority)) {
      await worker.ready
    } else if (worker.priority === 'default' && /default|low/i.test(info.priority)) {
      await worker.ready
    } else if (worker.priority === 'low' && /low/i.test(info.priority)) {
      await worker.ready
    }
  }

  const worker = new ServiceWorkerInstance('./worker.js', {
    info
  })

  workers.set(info.hash, worker)
  globals.get('ServiceWorkerContext.info').set(info.hash, info)
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
      globalThis.top.addEventListener(
        'serviceWorker.activate',
        async function (event) {
          // @ts-ignore
          const { hash } = new ServiceWorkerInfo(event.detail)
          if (hash === info.hash) {
            await sleep(64)
            resolve(null)
          }
        }
      )
    })
  ])

  if (!workers.has(info.hash)) {
    const options = {
      statusCode: 404,
      clientId: event.detail.fetch.client.id,
      headers: '',
      id: event.detail.fetch.id
    }

    return await ipc.write('serviceWorker.fetch.response', options)
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
  }

  const worker = workers.get(info.hash)

  worker.postMessage({ fetch: { ...info, client, request } })
}

export function onNotificationShow (event, target) {
  for (const worker of workers.values()) {
    if (worker.info.id === event.data.registration.id) {
      let notification = null

      try {
        notification = new Notification(
          event.data.showNotification.title,
          event.data.showNotification
        )
      } catch (error) {
        return target.postMessage({
          nonce: event.data.nonce,
          notification: {
            error: { message: error.message }
          }
        })
      }

      worker.notifications.push(notification)
      notification.onshow = () => {
        notification.onshow = null
        return target.postMessage({
          nonce: event.data.nonce,
          notification: {
            id: notification.id
          }
        })
      }

      notification.onclick = (event) => {
        worker.postMessage({
          notificationclick: {
            title: notification.title,
            options: notification.options.toJSON(),
            data: {
              id: notification.id,
              timestamp: notification.timestamp
            }
          }
        })
      }

      notification.onclose = (event) => {
        notification.onclose = null
        notification.onclick = null
        worker.postMessage({
          notificationclose: {
            title: notification.title,
            options: notification.options.toJSON(),
            data: {
              id: notification.id,
              timestamp: notification.timestamp
            }
          }
        })

        const index = worker.notifications.indexOf(notification)
        if (index >= 0) {
          worker.notifications.splice(index, 1)
        }
      }
      break
    }
  }
}

export function onNotificationClose (event) {
  for (const worker of workers.values()) {
    for (const notification of worker.notifications) {
      if (event.data.notificationclose.id === notification.id) {
        notification.close()
        return
      }
    }
  }
}

export function onGetNotifications (event) {
  for (const worker of workers.values()) {
    if (worker.info.id === event.data.registration.id) {
      return channel.postMessage({
        nonce: event.data.nonce,
        notifications: worker.notifications
          .filter((notification) =>
            !event.data.getNotifications?.tag ||
            event.data.getNotifications.tag === notification.tag
          )
          .map((notification) => ({
            title: notification.title,
            options: notification.options.toJSON(),
            data: {
              id: notification.id,
              timestamp: notification.timestamp
            }
          }))
      })
    }
  }
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

  if (workers.size === 0) {
    const result = await ipc.request('serviceWorker.getRegistrations')
    if (Array.isArray(result.data)) {
      for (const info of result.data) {
        await navigator.serviceWorker.register(info.scriptURL, info)
      }
    }
  }
})
