/* global ServiceWorker  */
import { SharedWorker } from '../internal/shared-worker.js'
import state from './state.js'

export const SHARED_WORKER_URL = new URL('./shared-worker.js', import.meta.url)

export function createServiceWorker (
  currentState = state.serviceWorker.state,
  options = null
) {
  // client message bus worker
  const sharedWorker = new SharedWorker(SHARED_WORKER_URL)

  // events
  const eventTarget = new EventTarget()
  let onstatechange = null
  let onerror = null

  // state
  let scriptURL = options?.scriptURL ?? null

  sharedWorker.port.start()
  sharedWorker.port.addEventListener('message', (event) => {
    eventTarget.dispatchEvent(new MessageEvent('message', event.data))
  })

  const serviceWorker = Object.create(ServiceWorker.prototype, {
    postMessage: {
      enumerable: false,
      configurable: false,
      value (message, ...args) {
        sharedWorker.postMessage(message, ...args)
      }
    },

    state: {
      configurable: true,
      enumerable: false,
      get: () => currentState
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
    state.channel.addEventListener('message', (event) => {
      const { data } = event
      if (data.serviceWorker) {
        if (data.serviceWorker.state && data.serviceWorker.state !== currentState) {
          const scope = new URL(globalThis.location.href).pathname
          if (scope.startsWith(data.serviceWorker.scope)) {
            currentState = data.serviceWorker.state
            scriptURL = data.serviceWorker.scriptURL
            const event = new Event('statechange')

            Object.defineProperties(event, {
              target: { value: serviceWorker }
            })

            eventTarget.dispatchEvent(event)
          }
        }
      }
    })
  }

  return serviceWorker
}

export default createServiceWorker(state.serviceWorker.state)
