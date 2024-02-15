/* global EventTarget */
import { createServiceWorker, SHARED_WORKER_URL } from './instance.js'
import { ServiceWorkerRegistration } from './registration.js'
import { InvertedPromise } from '../util.js'
import { SharedWorker } from '../internal/shared-worker.js'
import application from '../application.js'
import state from './state.js'
import ipc from '../ipc.js'

class ServiceWorkerContainerInternalStateMap extends Map {
  define (container, property, descriptor) {
    Object.defineProperty(container, property, {
      configurable: false,
      enumerable: true,
      ...descriptor
    })
  }
}

class ServiceWorkerContainerInternalState {
  currentWindow = null
  sharedWorker = null
  controller = null
  channel = new BroadcastChannel('ServiceWorkerContainer')
  ready = new InvertedPromise()

  // level 1 events
  oncontrollerchange = null
  onmessageerror = null
  onmessage = null
  onerror = null
}

const internal = new ServiceWorkerContainerInternalStateMap()

async function preloadExistingRegistration (container) {
  const registration = await container.getRegistration()
  if (registration) {
    if (registration.active) {
      queueMicrotask(() => {
        container.dispatchEvent(new Event('controllerchange'))
      })

      queueMicrotask(() => {
        internal.get(container).ready.resolve(registration)
      })
    } else {
      const serviceWorker = registration.installing || registration.waiting
      serviceWorker.addEventListener('statechange', function onStateChange (event) {
        if (
          serviceWorker.state === 'activating' ||
          serviceWorker.state === 'activated'
        ) {
          serviceWorker.removeEventListener('statechange', onStateChange)

          queueMicrotask(() => {
            container.dispatchEvent(new Event('controllerchange'))
          })

          queueMicrotask(() => {
            internal.get(container).ready.resolve(registration)
          })
        }
      })
    }
  }
}

async function activateRegistrationFromClaim (container, claim) {
  await container.register(claim.scriptURL)
  await preloadExistingRegistration(container)
}

export class ServiceWorkerContainer extends EventTarget {
  /**
   * A special initialization function for augmenting the global
   * `globalThis.navigator.serviceWorker` platform `ServiceWorkerContainer`
   * instance.
   *
   * All functions MUST be sure to what a lexically bound `this` becomes as the
   * target could change with respect to the `internal` `Map` instance which
   * contains private implementation properties relevant to the runtime
   * `ServiceWorkerContainer` internal state implementations.
   * @ignore
   * @private
   */
  async init () {
    internal.set(this, new ServiceWorkerContainerInternalState())

    this.register = this.register.bind(this)
    this.getRegistration = this.getRegistration.bind(this)
    this.getRegistrations = this.getRegistrations.bind(this)

    internal.define(this, 'controller', {
      get () {
        return internal.get(this).controller
      }
    })

    internal.define(this, 'oncontrollerchange', {
      get () {
        return internal.get(this).oncontrollerchange
      },

      set (oncontrollerchange) {
        if (internal.get(this).oncontrollerchange) {
          this.removeEventListener('controllerchange', internal.get(this).oncontrollerchange)
        }

        internal.get(this).oncontrollerchange = null

        if (typeof oncontrollerchange === 'function') {
          this.addEventListener('controllerchange', oncontrollerchange)
          internal.get(this).oncontrollerchange = oncontrollerchange
        }
      }
    })

    internal.define(this, 'onmessageerror', {
      get () {
        return internal.get(this).onmessageerror
      },

      set (onmessageerror) {
        if (internal.get(this).onmessageerror) {
          this.removeEventListener('messageerror', internal.get(this).onmessageerror)
        }

        internal.get(this).onmessageerror = null

        if (typeof onmessageerror === 'function') {
          this.addEventListener('messageerror', onmessageerror)
          internal.get(this).onmessageerror = onmessageerror
        }
      }
    })

    internal.define(this, 'onmessage', {
      get () {
        return internal.get(this).onmessage
      },

      set (onmessage) {
        if (internal.get(this).onmessage) {
          this.removeEventListener('message', internal.get(this).onmessage)
        }

        internal.get(this).onmessage = null

        if (typeof onmessage === 'function') {
          this.addEventListener('message', onmessage)
          internal.get(this).onmessage = onmessage
        }
      }
    })

    internal.define(this, 'onerror', {
      get () {
        return internal.get(this).onerror
      },

      set (onerror) {
        if (internal.get(this).onerror) {
          this.removeEventListener('error', internal.get(this).onerror)
        }

        internal.get(this).onerror = null

        if (typeof onerror === 'function') {
          this.addEventListener('error', onerror)
          internal.get(this).onerror = onerror
        }
      }
    })

    preloadExistingRegistration(this)

    internal.get(this).ready.then(async (registration) => {
      if (registration) {
        internal.get(this).controller = registration.active
        internal.get(this).sharedWorker = new SharedWorker(SHARED_WORKER_URL)
        internal.get(this).currentWindow = await application.getCurrentWindow()
      }
    })

    state.channel.addEventListener('message', (event) => {
      if (event.data?.clients?.claim?.scope) {
        const { scope } = event.data.clients.claim
        if (globalThis.location.pathname.startsWith(scope)) {
          activateRegistrationFromClaim(this, event.data.clients.claim)
        }
      }
    })
  }

  get ready () {
    return internal.get(this).ready
  }

  get controller () {
    return internal.get(this).controller
  }

  async getRegistration (clientURL) {
    let scope = clientURL
    let currentScope = null

    if (scope) {
      try {
        scope = new URL(scope, globalThis.location.href).pathname
      } catch {}
    }

    if (globalThis.location.protocol === 'blob:') {
      currentScope = new URL('.', globalThis.location.pathname).pathname
    } else {
      currentScope = new URL('.', globalThis.location.href).pathname
    }

    if (!scope) {
      scope = currentScope
    }

    const result = await ipc.request('serviceWorker.getRegistration', { scope })

    if (result.err) {
      throw result.err
    }

    const info = result.data

    if (!info?.registration?.state || !info?.registration?.id) {
      return
    }

    if (scope === currentScope) {
      state.serviceWorker.state = info.registration.state.replace('registered', 'installing')
      state.serviceWorker.scope = scope
      state.serviceWorker.scriptURL = info.registration.scriptURL
    }

    const serviceWorker = createServiceWorker(state.serviceWorker.state, {
      subscribe: scope === currentScope,
      scriptURL: info.registration.scriptURL
    })

    return new ServiceWorkerRegistration(info, serviceWorker)
  }

  async getRegistrations () {
    const result = await ipc.request('serviceWorker.getRegistrations')

    if (result.err) {
      throw result.err
    }

    const registrations = []

    if (Array.isArray(result.data)) {
      for (const registration of result.data) {
        const info = { registration }
        info.registration.state = info.registration.state.replace('registered', 'installing')
        const serviceWorker = createServiceWorker(registration.state, {
          scriptURL: info.registration.scriptURL
        })
        registrations.push(new ServiceWorkerRegistration(info, serviceWorker))
      }
    }

    return registrations
  }

  async register (scriptURL, options = null) {
    scriptURL = new URL(scriptURL, globalThis.location.href).toString()

    if (!options || typeof options !== 'object') {
      options = {}
    }

    if (!options.scope || typeof options.scope !== 'string') {
      options.scope = new URL('./', scriptURL).pathname
    }

    const result = await ipc.request('serviceWorker.register', {
      ...options,
      scriptURL
    })

    if (result.err) {
      throw result.err
    }

    const info = result.data
    const url = new URL(scriptURL)

    if (url.pathname.startsWith(options.scope)) {
      state.serviceWorker.state = info.registration.state.replace('registered', 'installing')
      state.serviceWorker.scriptURL = info.registration.scriptURL

      const serviceWorker = createServiceWorker(state.serviceWorker.state, {
        scriptURL: info.registration.scriptURL
      })

      const registration = new ServiceWorkerRegistration(info, serviceWorker)

      return registration
    }
  }

  startMessages () {
    internal.get(this).ready.then(() => {
      internal.get(this).sharedWorker.port.start()
      internal.get(this).sharedWorker.port.addEventListener('message', (event) => {
        this.dispatchEvent(new MessageEvent(event.type, event))
      })
    })
  }
}

export default ServiceWorkerContainer
