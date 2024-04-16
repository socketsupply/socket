/* eslint-disable import/first */
globalThis.isServiceWorkerScope = true

import { ServiceWorkerGlobalScope } from './global.js'
import { createStorageInterface } from './storage.js'
import { Module, createRequire } from '../module.js'
import { STATUS_CODES } from '../http.js'
import { Notification } from '../notification.js'
import { Environment } from './env.js'
import { Deferred } from '../async.js'
import { Buffer } from '../buffer.js'
import { Cache } from '../commonjs/cache.js'
import globals from '../internal/globals.js'
import process from '../process.js'
import clients from './clients.js'
import debug from './debug.js'
import hooks from '../hooks.js'
import state from './state.js'
import path from '../path.js'
import util from '../util.js'
import ipc from '../ipc.js'

import {
  ExtendableMessageEvent,
  NotificationEvent,
  ExtendableEvent,
  FetchEvent
} from './events.js'

import '../console.js'

const SERVICE_WORKER_READY_TOKEN = { __service_worker_ready: true }

Object.defineProperties(
  globalThis,
  Object.getOwnPropertyDescriptors(ServiceWorkerGlobalScope.prototype)
)

const module = { exports: {} }
const events = new Set()

// event listeners
hooks.onReady(onReady)
globalThis.addEventListener('message', onMessage)

// service worker  globals
globals.register('ServiceWorker.state', state)
globals.register('ServiceWorker.events', events)
globals.register('ServiceWorker.module', module)

function onReady () {
  globalThis.postMessage(SERVICE_WORKER_READY_TOKEN)
}

async function onMessage (event) {
  if (event instanceof ExtendableMessageEvent) {
    return
  }

  const { data } = event

  if (data?.register) {
    event.stopImmediatePropagation()

    const { id, scope, scriptURL } = data.register
    const url = new URL(scriptURL)

    if (!url.pathname.startsWith('/socket/')) {
      // preload commonjs cache for user space server workers
      Cache.restore(['loader.status', 'loader.response'])
    }

    state.id = id
    state.serviceWorker.id = id
    state.serviceWorker.scope = scope
    state.serviceWorker.scriptURL = scriptURL

    Module.main.addEventListener('error', (event) => {
      if (event.error) {
        debug(event.error)
      }
    })

    Object.defineProperties(globalThis, {
      require: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: createRequire(scriptURL)
      },

      origin: {
        configurable: false,
        enumerable: true,
        writable: false,
        value: url.origin
      },

      __dirname: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: path.dirname(url.pathname)
      },

      __filename: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: url.pathname
      },

      module: {
        configurable: false,
        enumerable: false,
        writable: false,
        value: module
      },

      exports: {
        configurable: false,
        enumerable: false,
        get: () => module.exports
      },

      process: {
        configurable: false,
        enumerable: false,
        get: () => process
      },

      global: {
        configurable: false,
        enumerable: false,
        get: () => globalThis
      }
    })

    // create global registration from construct
    globalThis.registration = data.register

    try {
      // define the actual location of the worker. not `blob:...`
      globalThis.RUNTIME_WORKER_LOCATION = scriptURL

      // update and notify initial state change
      state.serviceWorker.state = 'registering'
      await state.notify('serviceWorker')

      // open envirnoment
      await Environment.open({ id, scope })

      // install storage interfaces
      Object.defineProperties(globalThis, {
        localStorage: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: await createStorageInterface('localStorage')
        },

        sessionStorage: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: await createStorageInterface('sessionStorage')
        },

        memoryStorage: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: await createStorageInterface('memoryStorage')
        }
      })

      // import module, which could be ESM, CommonJS,
      // or a simple ServiceWorker
      const result = await import(scriptURL)

      if (typeof module.exports === 'function') {
        module.exports = {
          default: module.exports
        }
      } else {
        Object.assign(module.exports, result)
      }

      state.serviceWorker.state = 'registered'
      await state.notify('serviceWorker')
    } catch (err) {
      debug(err)
      state.serviceWorker.state = 'error'
      await state.notify('serviceWorker')
      return
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.fetch === 'function') {
        state.fetch = module.exports.default.fetch.bind(module.exports.default)
      }
    } else if (typeof module.exports.default === 'function') {
      state.fetch = module.exports.default
    } else if (typeof module.exports.fetch === 'function') {
      state.fetch = module.exports.fetch.bind(module.exports)
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.install === 'function') {
        state.install = module.exports.default.install.bind(module.exports.default)
      }
    } else if (typeof module.exports.install === 'function') {
      state.install = module.exports.install.bind(module.exports)
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.activate === 'function') {
        state.activate = module.exports.default.activate.bind(module.exports.default)
      }
    } else if (typeof module.exports.activate === 'function') {
      state.activate = module.exports.activate.bind(module.exports)
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.reportError === 'function') {
        state.reportError = module.exports.default.reportError.bind(module.exports.default)
      }
    } else if (typeof module.exports.reportError === 'function') {
      state.reportError = module.reportError.bind(module.exports)
    }

    if (typeof state.activate === 'function') {
      globalThis.addEventListener('activate', async (event) => {
        try {
          await state.activate(event.context.env, event.ontext)
        } catch (err) {
          debug(err)
        }
      })
    }

    if (typeof state.install === 'function') {
      globalThis.addEventListener('install', async (event) => {
        try {
          await state.install(event.context.env, event.context)
        } catch (err) {
          debug(err)
        }
      })
    }

    if (typeof state.fetch === 'function') {
      if (!state.install) {
        globalThis.addEventListener('install', () => {
          globalThis.skipWaiting()
        })
      }

      if (!state.activate) {
        globalThis.addEventListener('activate', () => {
          clients.claim()
        })
      }

      globalThis.addEventListener('fetch', async (event) => {
        const deferred = new Deferred()
        let response = null

        event.respondWith(deferred.promise)

        try {
          response = await state.fetch(
            event.request,
            event.context.env,
            event.context
          )
        } catch (err) {
          debug(err)
          response = new Response(util.inspect(err), {
            statusText: err.message || STATUS_CODES[500],
            status: 500,
            headers: {
              'Runtime-Preload-Injection': 'disabled'
            }
          })
        }

        if (response) {
          if (!response.statusText) {
            response.statusText = STATUS_CODES[response.status]
          }

          deferred.resolve(response)
        } else {
          deferred.resolve(new Response('Not Found', {
            statusText: STATUS_CODES[404],
            status: 404,
            headers: {
              'Runtime-Preload-Injection': 'disabled'
            }
          }))
        }
      })
    }

    globalThis.postMessage({ __service_worker_registered: { id } })
    return
  }

  if (data?.unregister) {
    event.stopImmediatePropagation()
    state.serviceWorker.state = 'none'
    await state.notify('serviceWorker')
    globalThis.close()
    return
  }

  if (data?.install?.id === state.id) {
    event.stopImmediatePropagation()
    const installEvent = new ExtendableEvent('install')
    events.add(installEvent)
    state.serviceWorker.state = 'installing'
    await state.notify('serviceWorker')
    globalThis.dispatchEvent(installEvent)
    await installEvent.waitsFor()
    state.serviceWorker.state = 'installed'
    await state.notify('serviceWorker')
    events.delete(installEvent)
    return
  }

  if (data?.activate?.id === state.id) {
    event.stopImmediatePropagation()
    const activateEvent = new ExtendableEvent('activate')
    events.add(activateEvent)
    state.serviceWorker.state = 'activating'
    await state.notify('serviceWorker')
    globalThis.dispatchEvent(activateEvent)
    await activateEvent.waitsFor()
    state.serviceWorker.state = 'activated'
    await state.notify('serviceWorker')
    events.delete(activateEvent)
    return
  }

  if (data?.fetch?.request) {
    event.stopImmediatePropagation()
    if (/post|put/i.test(data.fetch.request.method)) {
      const result = await ipc.request('serviceWorker.fetch.request.body', {
        id: data.fetch.request.id
      }, { responseType: 'arraybuffer' })

      if (result.data) {
        if (result.data instanceof ArrayBuffer) {
          data.fetch.request.body = result.data
        } else if (result.data instanceof Buffer) {
          data.fetch.request.body = result.data.buffer
        } else if (result.data.buffer) {
          data.fetch.request.body = result.data.buffer
        } else if (typeof result.data === 'object') {
          data.fetch.request.body = JSON.stringify(result.data)
        } else {
          data.fetch.request.body = result.data
        }
      }
    }

    if (data.fetch.request.body) {
      data.fetch.request.body = new Uint8Array(data.fetch.request.body)
    }

    const url = new URL(data.fetch.request.url)
    const fetchEvent = new FetchEvent('fetch', {
      clientId: data.fetch.client.id,
      fetchId: data.fetch.request.id,
      request: new Request(data.fetch.request.url, {
        headers: new Headers(data.fetch.request.headers),
        method: (data.fetch.request.method ?? 'GET').toUpperCase(),
        body: data.fetch.request.body
      })
    })

    events.add(fetchEvent)
    if (url.protocol !== 'socket:') {
      const result = await ipc.request('protocol.getData', {
        scheme: url.protocol.replace(':', '')
      })

      if (result.data !== null && result.data !== undefined) {
        try {
          fetchEvent.context.data = JSON.parse(result.data)
        } catch {
          fetchEvent.context.data = result.data
        }
      }
    }

    globalThis.dispatchEvent(fetchEvent)
    await fetchEvent.waitsFor()
    events.delete(fetchEvent)
    return
  }

  if (event.data?.notificationclick) {
    event.stopImmediatePropagation()
    globalThis.dispatchEvent(new NotificationEvent('notificationclick', {
      action: event.data.notificationclick.action,
      notification: new Notification(
        event.data.notificationclick.title,
        event.data.notificationclick.options,
        event.data.notificationclick.data
      )
    }))
    return
  }

  if (event.data?.notificationclose) {
    event.stopImmediatePropagation()
    globalThis.dispatchEvent(new NotificationEvent('notificationclose', {
      action: event.data.notificationclose.action,
      notification: new Notification(
        event.data.notificationclose.title,
        event.data.notificationclose.options,
        event.data.notificationclose.data
      )
    }))
    return
  }

  if (
    typeof event.data?.from === 'string' &&
    event.data.message &&
    event.data.client
  ) {
    event.stopImmediatePropagation()
    globalThis.dispatchEvent(new ExtendableMessageEvent('message', {
      source: await clients.get(event.data.client.id),
      origin: event.data.client.origin,
      ports: event.ports,
      data: event.data.message
    }))
    // eslint-disable-next-line
    return
  }
}
