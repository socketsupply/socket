/* global EventTarget */
import { createServiceWorker, SHARED_WORKER_URL } from './instance.js'
import { ServiceWorkerRegistration } from './registration.js'
import { SharedWorker } from '../internal/shared-worker.js'
import { Deferred } from '../async.js'
import application from '../application.js'
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
  ready = new Deferred()
  init = new Deferred()

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

    if (!globalThis.top || !globalThis.top.document) {
      return
    }

    const frameId = `__${os.platform()}-service-worker-frame__`
    const existingFrame = globalThis.top.document.querySelector(
      `iframe[id="${frameId}"]`
    )

    if (existingFrame) {
      this.frame = existingFrame
      return
    }

    const pending = []

    this.frame = globalThis.top.document.createElement('iframe')

    pending.push(new Promise((resolve) => {
      globalThis.top.addEventListener('message', function onMessage (event) {
        if (event.data.__service_worker_frame_init === true) {
          globalThis.top.removeEventListener('message', onMessage)
          resolve()
        }
      })
    }))

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

    await Promise.all(pending)
  }
}

const internal = new ServiceWorkerContainerInternalStateMap()

async function preloadExistingRegistration (container) {
  const registration = await container.getRegistration()
  if (registration) {
    if (registration.active) {
      if (
        application.config.webview_service_worker_mode === 'hybrid' ||
        /android|ios/i.test(os.platform())
      ) {
        if (
          !internal.get(container).isRegistered &&
          !internal.get(container).isRegistering
        ) {
          container.register(registration.active.scriptURL, { scope: registration.scope })
        }
      } else {
        queueMicrotask(() => {
          container.dispatchEvent(new Event('controllerchange'))
        })

        queueMicrotask(() => {
          internal.get(container).ready.resolve(registration)
        })
      }
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

  if (
    globalThis.__RUNTIME_SERVICE_WORKER_CONTEXT__ ||
    globalThis.location.pathname === '/socket/service-worker/index.html'
  ) {
    return false
  }

  return String(config.permissions_allow_service_worker) !== 'false'
}

/**
 * A `ServiceWorkerContainer` implementation that is attached to the global
 * `globalThis.navigator.serviceWorker` object.
 */
export class ServiceWorkerContainer extends EventTarget {
  get ready () {
    return internal.get(this).ready.promise
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
      return internal.get(this).init
    }

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

    internal.get(this).ready.then(async (registration) => {
      if (registration) {
        internal.get(this).controller = registration.active
        internal.get(this).sharedWorker = new SharedWorker(SHARED_WORKER_URL)
        internal.get(this).currentWindow = await application.getCurrentWindow()
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
        String(application.config.webview_service_worker_frame) !== 'false' &&
        (
          application.config.webview_service_worker_mode === 'hybrid' ||
          /android|ios/i.test(os.platform())
        )
      ) {
        await ServiceWorkerContainerRealm.init(this)
      }

      await preloadExistingRegistration(this)
    }

    internal.get(this).init.resolve()
  }

  async getRegistration (clientURL) {
    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return await globalThis.top.navigator.serviceWorker.getRegistration(clientURL)
    }

    let scope = clientURL
    let currentScope = null

    if (scope && URL.canParse(scope, globalThis.location.href)) {
      scope = new URL(scope, globalThis.location.href).pathname
    }

    if (globalThis.isWorkerScope) {
      currentScope = new URL('.', globalThis.RUNTIME_WORKER_LOCATION).pathname
    } else if (globalThis.location.protocol === 'blob:') {
      currentScope = new URL('.', globalThis.location.pathname).pathname
    } else {
      currentScope = globalThis.location.pathname
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
    await internal.get(this).init

    if (globalThis.top && globalThis.window && globalThis.top !== globalThis.window) {
      return await globalThis.top.navigator.serviceWorker.register(scriptURL, options)
    }

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

    if (!info?.registration) {
      return // registration likely never completed
    }

    const url = 'blob:'.startsWith(globalThis.location.origin)
      ? new URL(info.registration.scope, new URL(globalThis.location.origin).pathname)
      : new URL(info.registration.scope, globalThis.location.origin)

    const container = this

    if (info?.registration && url.pathname.startsWith(options.scope)) {
      state.serviceWorker.state = info.registration.state.replace('registered', 'installing')
      state.serviceWorker.scriptURL = info.registration.scriptURL

      const serviceWorker = createServiceWorker(state.serviceWorker.state, {
        scriptURL: info.registration.scriptURL
      })

      const registration = new ServiceWorkerRegistration(info, serviceWorker)
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
