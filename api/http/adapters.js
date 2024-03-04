import { Deferred } from '../async.js'
import { Buffer } from '../buffer.js'
import assert from '../assert.js'

/**
 * @typedef {{
 *   Connection: typeof import('../http.js').Connection,
 *   globalAgent: import('../http.js').Agent,
 *   IncomingMessage: typeof import('../http.js').IncomingMessage,
 *   ServerResponse: typeof import('../http.js').ServerResponse,
 *   STATUS_CODES: object,
 *   METHODS: string[]
 * }} HTTPModuleInterface
 */

/**
 * An abstract base clase for a HTTP server adapter.
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

  /**
   * Called when the adapter should destroy itself.
   * @abstract
   */
  async destroy () {}
}

/**
 * A HTTP adapter for running a HTTP server in a service worker that uses the
 * "fetch" event for the request and response lifecycle.
 */
export class ServiceWorkerServerAdapter extends ServerAdapter {
  /**
   * `ServiceWorkerServerAdapter` class constructor.
   * @ignore
   * @param {import('../http.js').Server} server
   * @param {HTTPModuleInterface} httpInterface
   */
  constructor (server, httpInterface) {
    assert(
      globalThis.isServiceWorkerScope,
      'The ServiceWorkerServerAdapter can only be used in a ServiceWorker scope'
    )

    super(server, httpInterface)

    this.onInstall = this.onInstall.bind(this)
    this.onActivate = this.onActivate.bind(this)
    this.onFetch = this.onFetch.bind(this)

    globalThis.addEventListener('install', this.onInstall, { once: true })
    globalThis.addEventListener('activate', this.onActivate, { once: true })
    globalThis.addEventListener('fetch', this.onFetch)
  }

  /**
   * Called when the adapter should destroy itself.
   */
  async destroy () {
    globalThis.removeEventListener('install', this.onInstall, { once: true })
    globalThis.removeEventListener('activate', this.onActivate, { once: true })
    globalThis.removeEventListener('fetch', this.onFetch)
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
    console.log('fetch', event)
    if (this.server.closed) {
      console.log('server closed')
      return
    }

    const url = new URL(event.request.url)

    if (this.server.port !== 0 && url.port !== this.server.port) {
      console.log('port mismatch')
      return
    }

    if (this.server.host && url.hostname !== this.server.host) {
      console.log('host mismatch', { url: url.hostname, server: this.server.host })
      return
    }

    if (this.server.connections.length >= this.server.maxConnections) {
      console.log('too many connections')
      event.respondWith(new Response())
      return
    }

    const deferred = new Deferred()

    const incomingMessage = new this.httpInterface.IncomingMessage({
      complete: !/post|put/i.test(event.request.method),
      headers: event.request.headers,
      method: event.request.method,
      url: event.request.url
    })

    const serverResponse = new this.httpInterface.ServerResponse({
      request: incomingMessage
    })

    const connection = new this.httpInterface.Connection(
      this.server,
      incomingMessage,
      serverResponse
    )

    this.server.emit('connection', connection)
    event.respondWith(deferred.promise)

    this.server.emit('request', incomingMessage, serverResponse)

    if (/post|put/i.test(event.request.method)) {
      const { highWaterMark } = incomingMessage._readableState
      const requestArrayBuffer = await event.request.arrayBuffer()
      const buffer = Buffer.from(requestArrayBuffer)
      for (let i = 0; i < buffer.byteLength; i += highWaterMark) {
        incomingMessage.push(buffer.slice(i, i + highWaterMark))
      }
    }

    serverResponse.on('finish', () => {
      const buffer = Buffer.concat(serverResponse.buffers)

      if (!serverResponse.hasHeader('content-length')) {
        serverResponse.setHeader('content-length', buffer.byteLength)
      }

      // disable cache for error responses
      if (serverResponse.statusCode >= 400) {
        serverResponse.setHeader('cache-control', 'no-cache')
      }

      const response = new Response(buffer, {
        status: serverResponse.statusCode,
        statusText: serverResponse.statusText,
        headers: serverResponse.headers
      })

      deferred.resolve(response)
    })
  }
}

export default {
  ServerAdapter,
  ServiceWorkerServerAdapter
}
