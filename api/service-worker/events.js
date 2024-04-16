/* global MessagePort */
import { Deferred } from '../async.js'
import { Context } from './context.js'
import application from '../application.js'
import location from '../location.js'
import state from './state.js'
import ipc from '../ipc.js'

export const textEncoder = new TextEncoderStream()

export const FETCH_EVENT_TIMEOUT = (
  // TODO(@jwerle): document this
  parseInt(application.config.webview_service_worker_fetch_event_timeout) ||
  30000
)

export const FETCH_EVENT_MAX_RESPONSE_REDIRECTS = (
  // TODO(@jwerle): document this
  parseInt(application.config.webview_service_worker_fetch_event_max_response_redirects) ||
  16 // this aligns with WebKit
)

/**
 * The `ExtendableEvent` interface extends the lifetime of the "install" and
 * "activate" events dispatched on the global scope as part of the service
 * worker lifecycle.
 */
export class ExtendableEvent extends Event {
  #promise = new Deferred()
  #promises = []
  #pendingPromiseCount = 0
  #context = null

  /**
   * `ExtendableEvent` class constructor.
   * @ignore
   */
  constructor (...args) {
    super(...args)
    this.#context = new Context(this)
  }

  /**
   * A context for this `ExtendableEvent` instance.
   * @type {import('./context.js').Context}
   */
  get context () {
    return this.#context
  }

  /**
   * A promise that can be awaited which waits for this `ExtendableEvent`
   * instance no longer has pending promises.
   * @type {Promise}
   */
  get awaiting () {
    return this.waitsFor()
  }

  /**
   * The number of pending promises
   * @type {number}
   */
  get pendingPromises () {
    return this.#pendingPromiseCount
  }

  /**
   * `true` if the `ExtendableEvent` instance is considered "active",
   * otherwise `false`.
   * @type {boolean}
   */
  get isActive () {
    return (
      this.#pendingPromiseCount > 0 ||
      this.eventPhase === Event.AT_TARGET
    )
  }

  /**
   * Tells the event dispatcher that work is ongoing.
   * It can also be used to detect whether that work was successful.
   * @param {Promise} promise
   */
  waitUntil (promise) {
    // we ignore the isTrusted check here and just verify the event phase
    if (this.eventPhase !== Event.AT_TARGET) {
      throw new DOMException('Event is not active', 'InvalidStateError')
    }

    if (promise && promise instanceof Promise) {
      this.#pendingPromiseCount++
      this.#promises.push(promise)
      promise.then(
        () => queueMicrotask(() => {
          if (--this.#pendingPromiseCount === 0) {
            this.#promise.resolve()
          }
        }),
        () => queueMicrotask(() => {
          if (--this.#pendingPromiseCount === 0) {
            this.#promise.resolve()
          }
        })
      )

      // handle 0 pending promises
    }
  }

  /**
   * Returns a promise that this `ExtendableEvent` instance is waiting for.
   * @return {Promise}
   */
  async waitsFor () {
    if (this.#pendingPromiseCount === 0) {
      this.#promise.resolve()
    }

    return await this.#promise
  }
}

/**
 * This is the event type for "fetch" events dispatched on the service worker
 * global scope. It contains information about the fetch, including the
 * request and how the receiver will treat the response.
 */
export class FetchEvent extends ExtendableEvent {
  static defaultHeaders = new Headers()

  #handled = new Deferred()
  #request = null
  #clientId = null
  #isReload = false
  #fetchId = null
  #responded = false
  #timeout = null

  /**
   * `FetchEvent` class constructor.
   * @ignore
   * @param {string=} [type = 'fetch']
   * @param {object=} [options]
   */
  constructor (type = 'fetch', options = null) {
    super(type, options)

    this.#fetchId = options?.fetchId ?? null
    this.#request = options?.request ?? null
    this.#clientId = options?.clientId ?? ''
    this.#isReload = options?.isReload === true
    this.#timeout = setTimeout(() => {
      this.respondWith(new Response('Request Timeout', {
        status: 408,
        statusText: 'Request Timeout'
      }))
    }, FETCH_EVENT_TIMEOUT)
  }

  /**
   * The handled property of the `FetchEvent` interface returns a promise
   * indicating if the event has been handled by the fetch algorithm or not.
   * This property allows executing code after the browser has consumed a
   * response, and is usually used together with the `waitUntil()` method.
   * @type {Promise}
   */
  get handled () {
    return this.#handled.then(Promise.resolve())
  }

  /**
   * The request read-only property of the `FetchEvent` interface returns the
   * `Request` that triggered the event handler.
   * @type {Request}
   */
  get request () {
    return this.#request
  }

  /**
   * The `clientId` read-only property of the `FetchEvent` interface returns
   * the id of the Client that the current service worker is controlling.
   * @type {string}
   */
  get clientId () {
    return this.#clientId
  }

  /**
   * @ignore
   * @type {string}
   */
  get resultingClientId () {
    return ''
  }

  /**
   * @ignore
   * @type {string}
   */
  get replacesClientId () {
    return ''
  }

  /**
   * @ignore
   * @type {boolean}
   */
  get isReload () {
    return this.#isReload
  }

  /**
   * @ignore
   * @type {Promise}
   */
  get preloadResponse () {
    return Promise.resolve(null)
  }

  /**
   * The `respondWith()` method of `FetchEvent` prevents the webview's
   * default fetch handling, and allows you to provide a promise for a
   * `Response` yourself.
   * @param {Response|Promise<Response>} response
   */
  respondWith (response) {
    if (this.#responded) {
      return
    }

    this.#responded = true
    clearTimeout(this.#timeout)

    const clientId = this.#clientId
    const handled = this.#handled
    const id = this.#fetchId

    queueMicrotask(async () => {
      try {
        response = await response

        if (!response || !(response instanceof Response)) {
          // TODO(@jwerle): handle this
          return
        }

        if (response.type === 'error') {
          const statusCode = 0
          const headers = []
            .concat(Array.from(response.headers.entries()))
            .concat(Array.from(FetchEvent.defaultHeaders.entries()))
            .map((entry) => entry.join(':'))
            .concat('Runtime-Response-Source:serviceworker')
            .concat('Access-Control-Allow-Credentials:true')
            .concat('Access-Control-Allow-Origin:*')
            .concat('Access-Control-Allow-Methods:*')
            .concat('Access-Control-Allow-Headers:*')
            .join('\n')

          const params = {
            statusCode,
            clientId,
            headers,
            id
          }

          params['runtime-preload-injection'] = 'disabled'

          const result = await ipc.request('serviceWorker.fetch.response', params)

          if (result.err) {
            state.reportError(result.err)
          }

          handled.resolve()
          return
        }

        let arrayBuffer = null
        let statusCode = response.status ?? 200

        // just follow the redirect here now
        if (statusCode >= 300 && statusCode < 400 && response.headers.has('location')) {
          let previousResponse = response
          let remainingRedirects = FETCH_EVENT_MAX_RESPONSE_REDIRECTS

          while (remainingRedirects-- > 0) {
            const redirectLocation = previousResponse.headers.get('location')

            if (!redirectLocation) {
              statusCode = 404
              break
            }

            const url = new URL(redirectLocation, location.origin)
            previousResponse = await fetch(url.href)

            if (previousResponse.status >= 200 && previousResponse.status < 300) {
              arrayBuffer = await previousResponse.arrayBuffer()
              break
            } else if (previousResponse.status >= 300 && statusCode < 400) {
              continue
            } else {
              statusCode = previousResponse.statusCode
              arrayBuffer = await previousResponse.arrayBuffer()
              break
            }
          }
        } else {
          arrayBuffer = await response.arrayBuffer()
        }

        const headers = []
          .concat(Array.from(response.headers.entries()))
          .concat(Array.from(FetchEvent.defaultHeaders.entries()))
          .map((entry) => entry.join(':'))
          .concat('Runtime-Response-Source:serviceworker')
          .concat('Access-Control-Allow-Credentials:true')
          .concat('Access-Control-Allow-Origin:*')
          .concat('Access-Control-Allow-Methods:*')
          .concat('Access-Control-Allow-Headers:*')
          .concat(`Content-Length:${arrayBuffer.byteLength}`)
          .join('\n')

        const params = {
          statusCode,
          clientId,
          headers,
          id
        }

        params['runtime-preload-injection'] = (
          response.headers.get('runtime-preload-injection') ||
          'auto'
        )

        const result = await ipc.write(
          'serviceWorker.fetch.response',
          params,
          new Uint8Array(arrayBuffer)
        )

        if (result.err) {
          state.reportError(result.err)
        }

        handled.resolve()
      } catch (err) {
        state.reportError(err)
      } finally {
        handled.resolve()
      }
    })
  }
}

export class ExtendableMessageEvent extends ExtendableEvent {
  #data = null
  #ports = []
  #origin = null
  #source = null
  #lastEventId = ''

  /**
   * `ExtendableMessageEvent` class constructor.
   * @param {string=} [type = 'message']
   * @param {object=} [options]
   */
  constructor (type = 'message', options = null) {
    super(type, options)
    this.#data = options?.data ?? null

    if (Array.isArray(options?.ports)) {
      for (const port of options.ports) {
        if (port instanceof MessagePort) {
          this.#ports.push(port)
        }
      }
    }

    if (options?.source) {
      this.#source = options.source
    }
  }

  /**
   * @type {any}
   */
  get data () {
    return this.#data
  }

  /**
   * @type {MessagePort[]}
   */
  get ports () {
    return this.#ports
  }

  /**
   * @type {import('./clients.js').Client?}
   */
  get source () {
    return this.#source
  }

  /**
   * @type {string}
   */
  get lastEventId () {
    return this.#lastEventId
  }
}

export class NotificationEvent extends ExtendableEvent {
  #action = ''
  #notification = null

  constructor (type, options) {
    super(type, options)

    if (typeof options?.action === 'string') {
      this.#action = options.action
    }

    this.#notification = options.notification
  }

  get action () {
    return this.#action
  }

  get notification () {
    return this.#notification
  }
}

export default {
  ExtendableMessageEvent,
  ExtendableEvent,
  FetchEvent
}
