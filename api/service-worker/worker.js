import { ExtendableEvent, FetchEvent } from './events.js'
import { ServiceWorkerGlobalScope } from './global.js'
import { Module, createRequire } from '../module.js'
import { STATUS_CODES } from '../http.js'
import { Environment } from './env.js'
import { Deferred } from '../async.js'
import clients from './clients.js'
import hooks from '../hooks.js'
import state from './state.js'
import path from '../path.js'
import util from '../util.js'
import ipc from '../ipc.js'

const SERVICE_WORKER_READY_TOKEN = { __service_worker_ready: true }

Object.defineProperties(
  globalThis,
  Object.getOwnPropertyDescriptors(ServiceWorkerGlobalScope.prototype)
)

const events = new Set()

hooks.onReady(onReady)
globalThis.addEventListener('message', onMessage)

function onReady () {
  globalThis.postMessage(SERVICE_WORKER_READY_TOKEN)
}

async function onMessage (event) {
  const { data } = event

  if (data?.register) {
    const { id, scope, scriptURL } = data.register
    const url = new URL(scriptURL)
    const module = { exports: {} }

    state.id = id
    state.serviceWorker.scope = scope
    state.serviceWorker.scriptURL = scriptURL

    Module.main.addEventListener('error', (event) => {
      if (event.error) {
        globalThis.reportError(event.error)
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
      }
    })

    try {
      const result = await import(scriptURL)
      if (typeof module.exports === 'function') {
        module.exports = {
          default: module.exports
        }
      } else {
        Object.assign(module.exports, result)
      }
    } catch (err) {
      state.serviceWorker.state = 'error'
      await state.notify('serviceWorker')
      return state.reportError(err)
    }

    await Environment.open({ id, scope })

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.fetch === 'function') {
        state.fetch = module.exports.default.fetch
      }
    } else if (typeof module.exports.default === 'function') {
      state.fetch = module.exports.default
    } else if (typeof module.exports.fetch === 'function') {
      state.fetch = module.exports.fetch
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.install === 'function') {
        state.install = module.exports.default.install
      }
    } else if (typeof module.exports.install === 'function') {
      state.install = module.exports.install
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.activate === 'function') {
        state.activate = module.exports.default.activate
      }
    } else if (typeof module.exports.activate === 'function') {
      state.activate = module.exports.activate
    }

    if (module.exports.default && typeof module.exports.default === 'object') {
      if (typeof module.exports.default.reportError === 'function') {
        state.reportError = module.exports.default.reportError
      }
    } else if (typeof module.exports.reportError === 'function') {
      state.reportError = module.reportError
    }

    if (typeof state.activate === 'function') {
      globalThis.addEventListener('activate', async (event) => {
        try {
          await state.activate(event.context.env, event.ontext)
        } catch (err) {
          state.reportError(err)
        }
      })
    }

    if (typeof state.install === 'function') {
      globalThis.addEventListener('install', async (event) => {
        try {
          await state.install(event.context.env, event.context)
        } catch (err) {
          state.reportError(err)
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
        const promise = new Deferred()
        let response = null

        event.respondWith(promise)

        try {
          response = await state.fetch(
            event.request,
            event.context.env,
            event.context
          )
        } catch (err) {
          state.reportError(err)
          response = new Response(util.inspect(err), {
            statusText: STATUS_CODES[500],
            status: 500
          })
        }

        if (response) {
          if (!response.statusText) {
            response.statusText = STATUS_CODES[response.status]
          }

          promise.resolve(response)
        } else {
          promise.resolve(new Response('', {
            statusText: STATUS_CODES[404],
            status: 404
          }))
        }
      })
    }

    globalThis.registration = data.register

    globalThis.postMessage({ __service_worker_registered: { id } })
    return
  }

  if (data?.install?.id === state.id) {
    const event = new ExtendableEvent('install')
    state.serviceWorker.state = 'installing'
    await state.notify('serviceWorker')
    globalThis.dispatchEvent(event)
    await event.waitsFor()
    state.serviceWorker.state = 'installed'
    await state.notify('serviceWorker')
    events.delete(event)
  }

  if (data?.activate?.id === state.id) {
    const event = new ExtendableEvent('activate')
    state.serviceWorker.state = 'activating'
    await state.notify('serviceWorker')
    events.add(event)
    globalThis.dispatchEvent(event)
    await event.waitsFor()
    state.serviceWorker.state = 'activated'
    await state.notify('serviceWorker')
    events.delete(event)
  }

  if (data?.fetch?.request) {
    if (/post|put/i.test(data.fetch.request.method)) {
      const result = await ipc.request('serviceWorker.fetch.request.body', {
        id: data.fetch.request.id
      })

      if (result.data) {
        data.fetch.request.body = result.data
      }
    }

    const event = new FetchEvent('fetch', {
      clientId: data.fetch.client.id,
      fetchId: data.fetch.request.id,
      request: new Request(data.fetch.request.url, {
        method: (data.fetch.request.method ?? 'GET').toUpperCase(),
        body: data.fetch.request.body
      })
    })

    globalThis.dispatchEvent(event)
  }

  if (event.data?.notificationclick) {
    // TODO(@jwerle)
  }

  if (event.data?.notificationshow) {
    // TODO(@jwerle)
  }
}
