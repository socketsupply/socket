import { ExtendableEvent, FetchEvent } from './events.js'
import { ServiceWorkerGlobalScope } from './global.js'
import { InvertedPromise } from '../util.js'
import clients from './clients.js'
import hooks from '../hooks.js'
import state from './state.js'
import ipc from '../ipc.js'

const SERVICE_WORKER_READY_TOKEN = { __service_worker_ready: true }

Object.defineProperties(
  globalThis,
  Object.getOwnPropertyDescriptors(ServiceWorkerGlobalScope.prototype)
)

const events = new Set()
const env = {}

hooks.onReady(onReady)
globalThis.addEventListener('message', onMessage)

function onReady () {
  globalThis.postMessage(SERVICE_WORKER_READY_TOKEN)
}

async function onMessage (event) {
  const { data } = event

  if (data?.register) {
    const { id, scope, scriptURL } = data.register
    let module = null

    state.id = id
    state.serviceWorker.scope = scope
    state.serviceWorker.scriptURL = scriptURL

    try {
      module = await import(scriptURL)
    } catch (err) {
      state.serviceWorker.state = 'error'
      await state.notify('serviceWorker')
      return state.reportError(err)
    }

    if (module.default && typeof module.default === 'object') {
      if (typeof module.default.fetch === 'function') {
        state.fetch = module.default.fetch
      }
    } else if (typeof module.default === 'function') {
      state.fetch = module.default
    } else if (typeof module.fetch === 'function') {
      state.fetch = module.fetch
    }

    if (module.default && typeof module.default === 'object') {
      if (typeof module.default.install === 'function') {
        state.install = module.default.install
      }
    } else if (typeof module.install === 'function') {
      state.install = module.install
    }

    if (module.default && typeof module.default === 'object') {
      if (typeof module.default.activate === 'function') {
        state.activate = module.default.activate
      }
    } else if (typeof module.activate === 'function') {
      state.activate = module.activate
    }

    if (module.default && typeof module.default === 'object') {
      if (typeof module.default.reportError === 'function') {
        state.reportError = module.default.reportError
      }
    } else if (typeof module.reportError === 'function') {
      state.reportError = module.reportError
    }

    if (typeof state.activate === 'function') {
      globalThis.addEventListener('activate', async () => {
        const context = {
          passThroughOnException () {},
          async waitUntil (...args) {
            return await event.waitUntil(...args)
          }
        }

        try {
          await state.activate(env, context)
        } catch (err) {
          state.reportError(err)
        }
      })
    }

    if (typeof state.install === 'function') {
      globalThis.addEventListener('install', async () => {
        const context = {
          passThroughOnException () {},
          async waitUntil (...args) {
            return await event.waitUntil(...args)
          }
        }

        try {
          await state.install(env, context)
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
        const { request } = event
        const context = {
          passThroughOnException () {},
          async waitUntil (...args) {
            return await event.waitUntil(...args)
          },

          async handled () {
            return await event.handled
          },

          async client () {
            if (event.clientId) {
              return await clients.get(event.clientId)
            }

            return null
          }
        }

        const promise = new InvertedPromise()
        let response = null

        event.respondWith(promise)

        try {
          response = await state.fetch(request, env, context)
        } catch (err) {
          state.reportError(err)
          response = new Response(err.message, {
            status: 500
          })
        }

        if (response) {
          promise.resolve(response)
        } else {
          promise.resolve(new Response('', { status: 404 }))
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
        method: data.fetch.request.method ?? 'GET',
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
