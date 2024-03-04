import { Deferred } from '../async.js'
import assert from '../assert.js'

/**
 * @typedef {{
 * }} HTTPModuleInterface
 */

export class ServerAdapter extends EventTarget {
  #server = null
  #httpInterface = null

  /**
   * `ServerAdapter` class constructor.
   * @ignore
   * @param {import('../http.js').Server} server
   * @param {HTTPModuleInterface} httpInterface
   */
  constructor (server, httpInterface) {
    super()

    this.#server = server
    this.#httpInterface = httpInterface
  }

  /**
   * A readonly reference to the underlying HTTP(S) server
   * for this adapter.
   * @type {import('../http.js').Server}
   */
  get server () {
    return this.#server
  }

  /**
   * A readonly reference to the underlying HTTP(S) module interface
   * for creating various HTTP module class objects.
   * @type {HTTPModuleInterface}
   */
  get httpInterface () {
    return this.#httpInterface
  }
}

/**
 */
export class ServiceWorkerServerAdapter extends ServerAdapter {
  /**
   * `ServiceWorkerServerAdapter` class constructor.
   * @ignore
   * @param {import('../http.js').Server} server
   */
  constructor (server) {
    assert(
      globalThis.isServiceWorkerScope,
      'The ServiceWorkerServerAdapter can only be used in a ServiceWorker scope'
    )

    super(server)

    globalThis.addEventListener('install', (event) => {
      this.onInstall(event)
    }, { once: true })

    globalThis.addEventListener('activate', (event) => {
      this.onActivate(event)
    }, { once: true })

    globalThis.addEventListener('fetch', (event) => {
      this.onFetch(event)
    })
  }

  /**
   * @ignore
   * @param {import('../service-worker/events.js').ExtendableEvent}
   */
  async onInstall (event) {
    globalThis.skipWaiting()
    this.dispatchEvent(new Event('install'))
  }

  /**
   * @ignore
   * @param {import('../service-worker/events.js').ExtendableEvent}
   */
  async onActivate (event) {
    globalThis.clients.claim()
    this.dispatchEvent(new Event('activate'))
  }

  /**
   * @ignore
   * @param {import('../service-worker/events.js').FetchEvent}
   */
  async onFetch (event) {
    const { request, context } = event
  }
}

export default {
  ServerAdapter,
  ServiceWorkerServerAdapter
}
