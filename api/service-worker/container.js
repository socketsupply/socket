/* global EventTarget */
import { createServiceWorker, SHARED_WORKER_URL } from './instance.js'
import { ServiceWorkerRegistration } from './registration.js'
import { InvertedPromise } from '../util.js'
import { SharedWorker } from '../internal/shared-worker.js'
import application from '../application.js'
import hooks from '../hooks.js'
import state from './state.js'
import ipc from '../ipc.js'
import os from '../os.js'

const SERVICE_WINDOW_PATH = `${globalThis.origin}/socket/service-worker/index.html`

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
  channel = new BroadcastChannel('socket.runtime.ServiceWorkerContainer')
  ready = new InvertedPromise()
  init = new InvertedPromise()

  isRegistering = false
  isRegistered = false

  // level 1 events
  oncontrollerchange = null
  onmessageerror = null
  onmessage = null
  onerror = null
}

class ServiceWorkerContainerRealm {
  static instance = null

  frame = null
  static async init (container) {
    const realm = new ServiceWorkerContainerRealm()
    return await realm.init(container)
  }

  async init (container) {
    if (ServiceWorkerContainerRealm.instance) {
      return
    }

    ServiceWorkerContainerRealm.instance = this

    const frameId = `__${os.platform()}-service-worker-frame__`
    const existingFrame = globalThis.top.document.querySelector(
      `iframe[id="${frameId}"]`
    )

    if (existingFrame) {
      this.frame = existingFrame
      return
    }

    this.frame = globalThis.top.document.createElement('iframe')
    this.frame.setAttribute('sandbox', 'allow-same-origin allow-scripts')
    this.frame.src = SERVICE_WINDOW_PATH
    this.frame.id = frameId

    Object.assign(this.frame.style, {
      display: 'none',
      height: 0,
      width: 0
    })

    const target = (
      globalThis.top.document.head ??
      globalThis.top.document.body ??
      globalThis.top.document
    )

    target.appendChild(this.frame)

    await new Promise((resolve, reject) => {
      setTimeout(resolve, 500)
      this.frame.addEventListener('load', resolve)
      this.frame.onerror = (event) => {
        reject(new Error('Failed to load ServiceWorker context window frame', {
          cause: event.error ?? event
        }))
      }
    })

    if (
      globalThis.window &&
      globalThis.top &&
      globalThis.window === globalThis.top &&
      application.getCurrentWindowIndex() === 0
    ) {
      console.log('RESET')
      // reset serviceWorker state on bridge if we are in a
      // realm (hybrid) context setup so register already service worker
      // registrations that need to be reinitialized when the page is reloaded
      await ipc.request('serviceWorker.reset')
    await new Promise((resolve, reject) => {
      setTimeout(resolve, 500)
    })
    }
  }
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

/**
 * Predicate to determine if service workers are allowed
 * @return {boolean}
 */
export function isServiceWorkerAllowed () {
  const { config } = application

  if (globalThis.__RUNTIME_SERVICE_WORKER_CONTEXT__) {
    return false
  }

  return String(config.permissions_allow_service_worker) !== 'false'
}

/**
 * TODO
 */
export class ServiceWorkerContainer extends EventTarget {
  get ready () {
    return internal.get(this).ready
  }

  get controller () {
    return internal.get(this).controller
  }

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
    if (internal.get(this)) {
      internal.get(this).init.resolve()
      return
    }

    internal.set(this, new ServiceWorkerContainerInternalState())
    internal.get(this).currentWindow = await application.getCurrentWindow()

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

    internal.get(this).ready.then(async (registration) => {
      if (registration) {
        internal.get(this).controller = registration.active
        internal.get(this).sharedWorker = new SharedWorker(SHARED_WORKER_URL)
      }
    })

    if (isServiceWorkerAllowed()) {
      state.channel.addEventListener('message', (event) => {
        if (event.data?.clients?.claim?.scope) {
          const { scope } = event.data.clients.claim
          if (globalThis.location.pathname.startsWith(scope)) {
            activateRegistrationFromClaim(this, event.data.clients.claim)
          }
        }
      })

      if (
        application.config.webview_service_worker_mode === 'hybrid' ||
        /android|ios/i.test(os.platform())
      ) {
        console.log('before realm init')
        await ServiceWorkerContainerRealm.init(this)
        console.log('after realm init')
      }

      console.log('preloading')
      await preloadExistingRegistration(this)
      console.log('after preloadExistingRegistration')
    }

    console.log('resolve init', globalThis.location.href)
    internal.get(this).init.resolve()
  }

  async getRegistration (clientURL) {
    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return await globalThis.top.navigator.serviceWorker.getRegistration(clientURL)
    }

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
    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return await globalThis.top.navigator.serviceWorker.getRegistrations()
    }

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
    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return await globalThis.top.navigator.serviceWorker.register(scriptURL, options)
    }

    console.log('before init', globalThis.location.href, internal.get(this))
    await internal.get(this).init
    console.log('after init')

    scriptURL = new URL(scriptURL, globalThis.location.href).toString()

    if (!options || typeof options !== 'object') {
      options = {}
    }

    if (!options.scope || typeof options.scope !== 'string') {
      options.scope = new URL('./', scriptURL).pathname
    }

    internal.get(this).isRegistered = false
    internal.get(this).isRegistering = true

    const result = await ipc.request('serviceWorker.register', {
      ...options,
      scriptURL
    })

    internal.get(this).isRegistering = false

    if (result.err) {
      throw result.err
    }

    internal.get(this).isRegistered = true

    const info = result.data
    const url = new URL(scriptURL)

    console.log({ result })
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
    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return globalThis.top.navigator.serviceWorker.startMessages()
    }

    internal.get(this).ready.then(() => {
      internal.get(this).sharedWorker.port.start()
      internal.get(this).sharedWorker.port.addEventListener('message', (event) => {
        this.dispatchEvent(new MessageEvent(event.type, event))
      })
    })
  }
}

export default ServiceWorkerContainer
