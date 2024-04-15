import { SharedWorker } from '../internal/shared-worker.js'
import location from '../location.js'
import state from './state.js'

const serviceWorkers = new Map()
let sharedWorker = null

export const SHARED_WORKER_URL = `${globalThis.origin}/socket/service-worker/shared-worker.js`

export const ServiceWorker = globalThis.ServiceWorker ?? class ServiceWorker extends EventTarget {
  get onmessage () { return null }
  set onmessage (_) {}
  get onerror () { return null }
  set onerror (_) {}
  get onstatechange () { return null }
  set onstatechange (_) {}
  get state () { return null }
  get scriptURL () { return null }
  postMessage () {}
}

export function createServiceWorker (
  currentState = state.serviceWorker.state,
  options = null
) {
  const id = options?.id ?? state.id ?? null

  if (!globalThis.isServiceWorkerScope) {
    if (id && serviceWorkers.has(id)) {
      return serviceWorkers.get(id)
    }
  }

  const channel = new BroadcastChannel('socket.runtime.serviceWorker.state')

  // events
  const eventTarget = new EventTarget()
  let onstatechange = null
  let onerror = null

  // state
  let serviceWorker = null
  let scriptURL = options?.scriptURL ?? null

  if (
    globalThis.RUNTIME_WORKER_LOCATION !== SHARED_WORKER_URL &&
    globalThis.location.pathname !== '/socket/service-worker/index.html'
  ) {
    sharedWorker = new SharedWorker(SHARED_WORKER_URL)
    sharedWorker.port.start()
  }

  serviceWorker = Object.create(ServiceWorker.prototype, {
    postMessage: {
      enumerable: false,
      configurable: false,
      value (message, ...args) {
        if (sharedWorker && globalThis.__args?.client) {
          sharedWorker.port.postMessage({
            message,
            from: 'instance',
            registration: { id },
            client: {
              id: globalThis.__args.client.id,
              url: location.pathname + location.search,
              type: globalThis.__args.client.type,
              index: globalThis.__args.index,
              origin: location.origin,
              frameType: globalThis.__args.client.frameType
            }
          }, ...args)
        }
      }
    },

    state: {
      configurable: true,
      enumerable: false,
      get: () => currentState === null ? state.serviceWorker.state : currentState
    },

    scriptURL: {
      configurable: true,
      enumerable: false,
      get: () => scriptURL
    },

    onerror: {
      enumerable: false,
      get: () => onerror,
      set: (listener) => {
        if (onerror) {
          eventTarget.removeEventListener('error', onerror)
        }

        onerror = null

        if (typeof listener === 'function') {
          eventTarget.addEventListener('error', listener)
          onerror = listener
        }
      }
    },

    onstatechange: {
      enumerable: false,
      get: () => onstatechange,
      set: (listener) => {
        if (onstatechange) {
          eventTarget.removeEventListener('statechange', onstatechange)
        }

        onstatechange = null

        if (typeof listener === 'function') {
          eventTarget.addEventListener('statechange', listener)
          onstatechange = listener
        }
      }
    },

    dispatchEvent: {
      configurable: false,
      enumerable: false,
      value: eventTarget.dispatchEvent.bind(eventTarget)
    },

    addEventListener: {
      configurable: false,
      enumerable: false,
      value: eventTarget.addEventListener.bind(eventTarget)
    },

    removeEventListener: {
      configurable: false,
      enumerable: false,
      value: eventTarget.removeEventListener.bind(eventTarget)
    }
  })

  if (options?.subscribe !== false) {
    channel.addEventListener('message', (event) => {
      const { data } = event
      if (data?.serviceWorker?.id === id) {
        if (data.serviceWorker.state && data.serviceWorker.state !== currentState) {
          const scope = new URL(location.href).pathname
          if (scope.startsWith(data.serviceWorker.scope)) {
            if (data.serviceWorker.scriptURL) {
              scriptURL = data.serviceWorker.scriptURL
            }

            if (data.serviceWorker.state !== currentState) {
              currentState = data.serviceWorker.state
              const event = new Event('statechange')

              Object.defineProperties(event, {
                target: { value: serviceWorker }
              })

              eventTarget.dispatchEvent(event)
            }
          }
        }
      }
    })
  }

  if (!globalThis.isServiceWorkerScope && id) {
    serviceWorkers.set(id, serviceWorker)
  }

  return serviceWorker
}

export default createServiceWorker
