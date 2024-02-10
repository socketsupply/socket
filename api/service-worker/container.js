/* global EventTarget */
import serviceWorker, { createServiceWorker, SHARED_WORKER_URL } from './instance.js'
import { ServiceWorkerRegistration } from './registration.js'
import { InvertedPromise } from '../util.js'
import { SharedWorker } from '../internal/shared-worker.js'
import ipc from '../ipc.js'

class State {
  sharedWorker = null
  controller = null
  channel = new BroadcastChannel('ServiceWorkerContainer')
  ready = new InvertedPromise()

  // level 1 events
  #oncontrollerchange = null
  #onmessageerror = null
  #onmessage = null
  #onerror = null
}

const state = new Map()

export class ServiceWorkerContainer extends EventTarget {
  init () {
    state.set(this, new State())

    this.register = this.register.bind(this)
    this.getRegistration = this.getRegistration.bind(this)
    this.getRegistrations = this.getRegistrations.bind(this)

    Object.defineProperty(this, 'controller', {
      configurable: false,
      enumerable: true,
      get: () => state.get(this).controller
    })

    Object.defineProperty(this, 'oncontrollerchange', {
      configurable: false,
      enumerable: true,
      get: () => state.get(this).oncontrollerchange,
      set: (oncontrollerchange) => {
        if (state.get(this).oncontrollerchange) {
          this.removeEventListener('controllerchange', state.get(this).oncontrollerchange)
        }

        state.get(this).oncontrollerchange = null

        if (typeof oncontrollerchange === 'function') {
          this.addEventListener('controllerchange', oncontrollerchange)
          state.get(this).oncontrollerchange = oncontrollerchange
        }
      }
    })

    Object.defineProperty(this, 'onmessageerror', {
      configurable: false,
      enumerable: true,
      get: () => state.get(this).onmessageerror,
      set: (onmessageerror) => {
        if (state.get(this).onmessageerror) {
          this.removeEventListener('messageerror', state.get(this).onmessageerror)
        }

        state.get(this).onmessageerror = null

        if (typeof onmessageerror === 'function') {
          this.addEventListener('messageerror', onmessageerror)
          state.get(this).onmessageerror = onmessageerror
        }
      }
    })

    Object.defineProperty(this, 'onmessage', {
      configurable: false,
      enumerable: true,
      get: () => state.get(this).onmessage,
      set: (onmessage) => {
        if (state.get(this).onmessage) {
          this.removeEventListener('message', state.get(this).onmessage)
        }

        state.get(this).onmessage = null

        if (typeof onmessage === 'function') {
          this.addEventListener('message', onmessage)
          state.get(this).onmessage = onmessage
        }
      }
    })

    Object.defineProperty(this, 'onerror', {
      configurable: false,
      enumerable: true,
      get: () => state.get(this).onerror,
      set: (onerror) => {
        if (state.get(this).onerror) {
          this.removeEventListener('error', state.get(this).onerror)
        }

        state.get(this).onerror = null

        if (typeof onerror === 'function') {
          this.addEventListener('error', onerror)
          state.get(this).onerror = onerror
        }
      }
    })

    this.getRegistration().then((registration) => {
      if (registration) {
        if (registration.active) {
          queueMicrotask(() => this.dispatchEvent(new Event('controllerchange')))
          queueMicrotask(() => state.get(this).ready.resolve(registration))
        } else {
          serviceWorker.addEventListener('statechange', () => {
            if (serviceWorker.state === 'activating' || serviceWorker.state === 'activated') {
              queueMicrotask(() => this.dispatchEvent(new Event('controllerchange')))
              queueMicrotask(() => state.get(this).ready.resolve(registration))
            }
          })
        }
      }
    })

    state.get(this).ready.then((registration) => {
      if (registration) {
        state.get(this).controller = registration.active
        state.get(this).sharedWorker = new SharedWorker(SHARED_WORKER_URL)
      }
    })
  }

  get ready () {
    return state.get(this).ready
  }

  get controller () {
    return state.get(this).controller
  }

  async getRegistration (clientURL) {
    const scope = clientURL || new URL(globalThis.location.href).pathname
    const result = await ipc.request('serviceWorker.getRegistration', { scope })

    if (result.err) {
      throw result.err
    }

    if (result.data?.registration?.id) {
      return new ServiceWorkerRegistration(result.data, serviceWorker)
    }
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
        const serviceWorker = createServiceWorker(registration.state)
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

    const url = new URL(scriptURL)

    if (url.pathname.startsWith(options.scope)) {
      const registration = new ServiceWorkerRegistration(result.data, serviceWorker)
      serviceWorker.addEventListener('statechange', () => {
        if (serviceWorker.state === 'activating' || serviceWorker.state === 'activated') {
          queueMicrotask(() => this.dispatchEvent(new Event('controllerchange')))
          queueMicrotask(() => state.get(this).ready.resolve(registration))
        }
      })
      return registration
    }
  }

  startMessages () {
    state.get(this).ready.then(() => {
      state.get(this).sharedWorker.port.start()
      state.get(this).sharedWorker.port.addEventListener('message', (event) => {
        this.dispatchEvent(new MessageEvent(event.type, event))
      })
    })
  }
}

export default ServiceWorkerContainer
