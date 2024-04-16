/* global XMLHttpRequest */
/**
 * @module CommonJS.Loader
 */
import { CacheCollection, Cache } from './cache.js'
import InternalSymbols from '../internal/symbols.js'
import { Headers } from '../ipc.js'
import location from '../location.js'
import path from '../path.js'

const RUNTIME_SERVICE_WORKER_FETCH_MODE = 'Runtime-ServiceWorker-Fetch-Mode'
const RUNTIME_REQUEST_SOURCE_HEADER = 'Runtime-Request-Source'
const textDecoder = new TextDecoder()

/**
 * @typedef {{
 *   extensions?: string[] | Set<string>
 *   origin?: URL | string,
 *   statuses?: Cache
 *   cache?: { response?: Cache, status?: Cache },
 *   headers?: Headers | Map | object | string[][]
 * }} LoaderOptions
 */

/**
 * @typedef {{
 *   loader?: Loader,
 *   origin?: URL | string
 * }} RequestOptions
 */

/**
 * @typedef {{
 *   headers?: Headers | object | array[],
 *   status?: number
 * }} RequestStatusOptions
 */

/**
 * @typedef {{
 *   headers?: Headers | object
 * }} RequestLoadOptions
 */

/**
 * @typedef {{
 *   request?: Request,
 *   headers?: Headers,
 *   status?: number,
 *   buffer?: ArrayBuffer,
 *   text?: string
 * }} ResponseOptions
 */

/**
 * A container for the status of a CommonJS resource. A `RequestStatus` object
 * represents meta data for a `Request` that comes from a preflight
 * HTTP HEAD request.
 */
export class RequestStatus {
  /**
   * Creates a `RequestStatus` from JSON input.
   * @param {object} json
   * @return {RequestStatus}
   */
  static from (json, options) {
    const status = new this(null, json)
    status.request = Request.from({ url: json.id }, { ...options, status })
    return status
  }

  #status = undefined
  #request = null
  #headers = new Headers()

  /**
   * `RequestStatus` class constructor.
   * @param {Request} request
   * @param {RequestStatusOptions} [options]
   */
  constructor (request, options = null) {
    if (request && !(request instanceof Request)) {
      throw new TypeError(
        `Expecting 'request' to be a Request object. Received: ${request}`
      )
    }

    if (request) {
      this.request = request
    }

    this.#headers = options?.headers ? Headers.from(options.headers) : this.#headers
    this.#status = options?.status ? options.status : undefined
  }

  /**
   * The `Request` object associated with this `RequestStatus` object.
   * @type {Request}
   */
  get request () { return this.#request }
  set request (request) {
    this.#request = request

    if (!this.#status && request.loader?.cache?.status) {
      this.#status = request.loader.cache.status.get(request.id)?.value
    }
  }

  /**
   * The unique ID of this `RequestStatus`, which is the absolute URL as a string.
   * @type {string}
   */
  get id () {
    return this.#request?.id ?? null
  }

  /**
   * The origin for this `RequestStatus` object.
   * @type {string}
   */
  get origin () {
    return this.#request?.origin ?? null
  }

  /**
   * A HTTP status code for this `RequestStatus` object.
   * @type {number|undefined}
   */
  get status () {
    return this.#status
  }

  /**
   * An alias for `status`.
   * @type {number|undefined}
   */
  get value () {
    return this.#status
  }

  /**
   * @ignore
   */
  get valueOf () {
    return this.status
  }

  /**
   * The HTTP headers for this `RequestStatus` object.
   * @type {Headers}
   */
  get headers () {
    return this.#headers
  }

  /**
   * The resource location for this `RequestStatus` object. This value is
   * determined from the 'Content-Location' header, if available, otherwise
   * it is derived from the request URL pathname (including the query string).
   * @type {string}
   */
  get location () {
    const contentLocation = this.#headers.get('content-location')
    if (contentLocation) {
      return contentLocation
    }

    if (this.#request) {
      return this.#request.url.pathname + this.#request.url.search
    }

    return ''
  }

  /**
   * `true` if the response status is considered OK, otherwise `false`.
   * @type {boolean}
   */
  get ok () {
    return this.#status >= 200 && this.#status < 400
  }

  /**
   * Loads the internal state for this `RequestStatus` object.
   * @param {RequestLoadOptions|boolean} [options]
   * @return {RequestStatus}
   */
  load (options = null) {
    // allow `load(true)` to force a reload of the state
    if (this.#status && options !== true) {
      return this
    }

    const request = new XMLHttpRequest()

    request.open('HEAD', this.#request.id, false)
    request.setRequestHeader(RUNTIME_REQUEST_SOURCE_HEADER, 'module')
    request.withCredentials = true

    if (globalThis.isServiceWorkerScope) {
      request.setRequestHeader(RUNTIME_SERVICE_WORKER_FETCH_MODE, 'ignore')
    }

    if (this.#request?.loader) {
      const entries = this.#request.loader.headers.entries()
      for (const entry of entries) {
        request.setRequestHeader(...entry)
      }
    }

    if (options?.headers && typeof options?.headers === 'object') {
      const entries = typeof options.headers.entries === 'function'
        ? options.headers.entries()
        : Object.entries(options.headers)

      for (const entry of entries) {
        request.setRequestHeader(...entry)
      }
    }

    request.send(null)

    this.#headers = Headers.from(request)
    this.#status = request.status

    const contentLocation = this.#headers.get('content-location')

    if (this.#request) {
      this.#request.loader.cache.status.set(this.id, this)
    }

    // verify 'Content-Location' header if given in response
    // @ts-ignore
    if (this.#request && contentLocation && URL.canParse(contentLocation, this.origin)) {
      const url = new URL(contentLocation, this.origin)
      const extension = path.extname(url.pathname)

      if (!this.#request.loader.extensions.has(extension)) {
        this.#status = 404
        return this
      }
    }

    return this
  }

  /**
   * Converts this `RequestStatus` to JSON.
   * @ignore
   * @return {{
   *   id: string,
   *   origin: string | null,
   *   status: number,
   *   headers: Array<string[]>
   *   request: object | null | undefined
   * }}
   */
  toJSON (includeRequest = true) {
    if (includeRequest) {
      return {
        id: this.id,
        origin: this.origin,
        status: this.status,
        headers: Array.from(this.headers.entries()),
        request: this.#request ? this.#request.toJSON(false) : null
      }
    } else {
      return {
        id: this.id,
        origin: this.origin,
        status: this.status,
        headers: Array.from(this.headers.entries())
      }
    }
  }

  /**
   * Serializes this `Response`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'RequestStatus',
   *   id: string,
   *   origin: string | null,
   *   status: number,
   *   headers: Array<string[]>
   *   request: object | null
   * }}
   */
  [InternalSymbols.serialize] () {
    return { __type__: 'RequestStatus', ...this.toJSON() }
  }
}

/**
 * A container for a synchronous CommonJS request to local resource or
 * over the network.
 */
export class Request {
  /**
   * Creates a `Request` instance from JSON input
   * @param {object} json
   * @param {RequestOptions=} [options]
   * @return {Request}
   */
  static from (json, options) {
    return new this(json.url, {
      status: json.status && typeof json.status === 'object'
        ? RequestStatus.from(json.status)
        : options?.status,
      ...options
    })
  }

  #url = null
  #loader = null
  #status = null

  /**
   * `Request` class constructor.
   * @param {URL|string} url
   * @param {URL|string=} [origin]
   * @param {RequestOptions=} [options]
   */
  constructor (url, origin, options = null) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin ?? null
    }

    if (!origin) {
      origin = location.origin
    }

    if (String(origin).startsWith('blob:')) {
      origin = new URL(origin).pathname
    }

    this.#url = new URL(url, origin)
    this.#loader = options?.loader ?? null
    this.#status = options?.status instanceof RequestStatus
      ? options.status
      : new RequestStatus(this)

    this.#status.request = this
  }

  /**
   * The unique ID of this `Request`, which is the absolute URL as a string.
   * @type {string}
   */
  get id () {
    return this.url.href
  }

  /**
   * The absolute `URL` of this `Request` object.
   * @type {URL}
   */
  get url () {
    return this.#url
  }

  /**
   * The origin for this `Request`.
   * @type {string}
   */
  get origin () {
    return this.url.origin
  }

  /**
   * The `Loader` for this `Request` object.
   * @type {Loader?}
   */
  get loader () {
    return this.#loader
  }

  /**
   * The `RequestStatus` for this `Request`
   * @type {RequestStatus}
   */
  get status () {
    return this.#status.load()
  }

  /**
   * Loads the CommonJS source file, optionally checking the `Loader` cache
   * first, unless ignored when `options.cache` is `false`.
   * @param {RequestLoadOptions=} [options]
   * @return {Response}
   */
  load (options = null) {
    // check loader cache first
    if (options?.cache !== false && this.#loader !== null) {
      if (this.#loader.cache.response.has(this.id)) {
        return this.#loader.cache.response.get(this.id)
      }
    }

    if (this.status.value >= 400) {
      return new Response(this, {
        status: this.status.value
      })
    }

    const request = new XMLHttpRequest()
    request.open('GET', this.id, false)
    request.setRequestHeader(RUNTIME_REQUEST_SOURCE_HEADER, 'module')
    request.withCredentials = true

    if (globalThis.isServiceWorkerScope) {
      request.setRequestHeader(RUNTIME_SERVICE_WORKER_FETCH_MODE, 'ignore')
    }

    if (typeof options?.responseType === 'string') {
      request.responseType = options.responseType
    }

    if (this.#loader) {
      const entries = this.#loader.headers.entries()
      for (const entry of entries) {
        request.setRequestHeader(...entry)
      }
    }

    if (options?.headers && typeof options?.headers === 'object') {
      const entries = typeof options.headers.entries === 'function'
        ? options.headers.entries()
        : Object.entries(options.headers)

      for (const entry of entries) {
        request.setRequestHeader(...entry)
      }
    }

    request.send(null)

    let responseText = null

    try {
      // @ts-ignore
      responseText = request.responseText // can throw `InvalidStateError` error
    } catch {
      if (typeof request.response === 'string') {
        responseText = request.response
      }
    }

    return new Response(this, {
      headers: Headers.from(request),
      status: request.status,
      buffer: request.response,
      text: responseText ?? null
    })
  }

  /**
   * Converts this `Request` to JSON.
   * @ignore
   * @return {{
   *   url: string,
   *   status: object | undefined
   * }}
   */
  toJSON (includeStatus = true) {
    if (includeStatus) {
      return {
        url: this.url.href,
        status: this.status.toJSON(false)
      }
    } else {
      return {
        url: this.url.href
      }
    }
  }

  /**
   * Serializes this `Response`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'Request',
   *   url: string,
   *   status: object | undefined
   * }}
   */
  [InternalSymbols.serialize] () {
    return { __type__: 'Request', ...this.toJSON() }
  }
}

/**
 * A container for a synchronous CommonJS request response for a local resource
 * or over the network.
 */
export class Response {
  /**
   * Creates a `Response` from JSON input
   * @param {obejct} json
   * @param {ResponseOptions=} [options]
   * @return {Response}
   */
  static from (json, options) {
    return new this({
      ...json,
      request: Request.from({ url: json.id }, options)
    }, options)
  }

  #request = null
  #headers = null
  #status = 404
  #buffer = null
  #text = ''

  /**
   * `Response` class constructor.
   * @param {Request|ResponseOptions} request
   * @param {ResponseOptions=} [options]
   */
  constructor (request, options = null) {
    options = { ...options }

    if (typeof request === 'object' && !(request instanceof Request)) {
      options = request
      request = options.request
    }

    if (!request || !(request instanceof Request)) {
      throw new TypeError(
        `Expecting 'request' to be a Request object. Received: ${request}`
      )
    }

    this.#request = request
    this.#headers = Headers.from(options.headers)
    this.#status = options.status || 404
    this.#buffer = options.buffer ? new Uint8Array(options.buffer).buffer : null
    this.#text = options.text || ''

    if (request.loader) {
      // cache request response in the loader
      request.loader.cache.response.set(request.id, this)
    }
  }

  /**
   * The unique ID of this `Response`, which is the absolute
   * URL of the request as a string.
   * @type {string}
   */
  get id () {
    return this.#request.id
  }

  /**
   * The `Request` object associated with this `Response` object.
   * @type {Request}
   */
  get request () {
    return this.#request
  }

  /**
   * The response headers from the associated request.
   * @type {Headers}
   */
  get headers () {
    return this.#headers
  }

  /**
   * The `Loader` associated with this `Response` object.
   * @type {Loader?}
   */
  get loader () {
    return this.request.loader
  }

  /**
   * The `Response` status code from the associated `Request` object.
   * @type {number}
   */
  get status () {
    return this.#status
  }

  /**
   * The `Response` string from the associated `Request`
   * @type {string}
   */
  get text () {
    if (this.#text) {
      return this.#text
    } else if (this.#buffer) {
      return textDecoder.decode(this.#buffer)
    }

    return ''
  }

  /**
   * The `Response` array buffer from the associated `Request`
   * @type {ArrayBuffer?}
   */
  get buffer () {
    return this.#buffer ?? null
  }

  /**
   * `true` if the response is considered OK, otherwise `false`.
   * @type {boolean}
   */
  get ok () {
    return this.id && this.status >= 200 && this.status < 400
  }

  /**
   * Converts this `Response` to JSON.
   * @ignore
   * @return {{
   *   id: string,
   *   text: string,
   *   status: number,
   *   buffer: number[] | null,
   *   headers: Array<string[]>
   * }}
   */
  toJSON () {
    return {
      id: this.id,
      text: this.text,
      status: this.status,
      buffer: this.#buffer ? Array.from(new Uint8Array(this.#buffer)) : null,
      headers: Array.from(this.#headers.entries())
    }
  }

  /**
   * Serializes this `Response`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'Response',
   *   id: string,
   *   text: string,
   *   status: number,
   *   buffer: number[] | null,
   *   headers: Array<string[]>
   * }}
   */
  [InternalSymbols.serialize] () {
    return { __type__: 'Response', ...this.toJSON() }
  }
}

/**
 * A container for loading CommonJS module sources
 */
export class Loader {
  /**
   * A request class used by `Loader` objects.
   * @type {typeof Request}
   */
  static Request = Request

  /**
   * A response class used by `Loader` objects.
   * @type {typeof Request}
   */
  static Response = Response

  /**
   * Resolves a given module URL to an absolute URL with an optional `origin`.
   * @param {URL|string} url
   * @param {URL|string} [origin]
   * @return {string}
   */
  static resolve (url, origin = null) {
    if (!origin) {
      origin = location.origin
    }

    if (String(origin).startsWith('blob:')) {
      origin = new URL(origin).pathname
    }

    if (String(url).startsWith('blob:')) {
      url = new URL(url).pathname
    }

    return String(new URL(url, origin))
  }

  /**
   * Default extensions for a loader.
   * @type {Set<string>}
   */
  static defaultExtensions = new Set([
    '.js',
    '.json',
    '.mjs',
    '.cjs',
    '.jsx',
    '.ts',
    '.tsx',
    '.wasm'
  ])

  #cache = new CacheCollection()

  #origin = null
  #headers = new Headers()
  #extensions = Loader.defaultExtensions

  /**
   * `Loader` class constructor.
   * @param {string|URL|LoaderOptions} origin
   * @param {LoaderOptions=} [options]
   */
  constructor (origin, options = null) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin
    }

    this.#origin = Loader.resolve('.', origin)

    if (options?.headers && typeof options.headers === 'object') {
      if (Array.isArray(options.headers)) {
        for (const entry of options.headers) {
          this.#headers.set(...entry)
        }
      } else if (typeof options.headers.entries === 'function') {
        for (const entry of options.headers.entries()) {
          this.#headers.set(...entry)
        }
      } else {
        for (const key in options.headers) {
          this.#headers.set(key, options.headers[key])
        }
      }
    }

    if (options?.extensions && typeof options.extensions === 'object') {
      if (Array.isArray(options.extensions) || options instanceof Set) {
        for (const value of options.extensions) {
          const extension = (!value.startsWith('.') ? `.${value}` : value).trim()
          if (extension) {
            this.#extensions.add(extension.trim())
          }
        }
      }
    }

    this.#cache.add(
      'status',
      options?.cache?.status instanceof Cache
        ? options.cache.status
        : new Cache('loader.status', { loader: this, types: { RequestStatus } })
    )

    this.#cache.add(
      'response',
      options?.cache?.response instanceof Cache
        ? options.cache.response
        : new Cache('loader.response', { loader: this, types: { Response } })
    )

    this.#cache.restore()
  }

  /**
   * The internal caches for this `Loader` object.
   * @type {{ response: Cache, status: Cache }}
   */
  get cache () {
    return this.#cache
  }

  /**
   * Headers used in too loader requests.
   * @type {Headers}
   */
  get headers () {
    return this.#headers
  }

  /**
   * A set of supported `Loader` extensions.
   * @type {Set<string>}
   */
  get extensions () {
    return this.#extensions
  }

  /**
   * The origin of this `Loader` object.
   * @type {string}
   */
  get origin () { return this.#origin }
  set origin (origin) {
    this.#origin = Loader.resolve(origin, location.origin)
  }

  /**
   * Loads a CommonJS module source file at `url` with an optional `origin`, which
   * defaults to the application origin.
   * @param {URL|string} url
   * @param {URL|string|object} [origin]
   * @param {RequestOptions=} [options]
   * @return {Response}
   */
  load (url, origin, options) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin ?? this.origin
    }

    if (!origin) {
      origin = this.origin
    }

    const request = new Request(url, {
      loader: this,
      origin
    })

    return request.load(options)
  }

  /**
   * Queries the status of a CommonJS module source file at `url` with an
   * optional `origin`, which defaults to the application origin.
   * @param {URL|string} url
   * @param {URL|string|object} [origin]
   * @param {RequestOptions=} [options]
   * @return {RequestStatus}
   */
  status (url, origin, options = null) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = options.origin ?? this.origin
    }

    if (!origin) {
      origin = this.origin
    }

    url = this.resolve(url, origin)

    if (this.#cache.status.has(url)) {
      return this.#cache.status.get(url)
    }

    const request = new Request(url, {
      loader: this,
      origin,
      ...options
    })

    this.#cache.status.set(url, request.status)
    return request.status
  }

  /**
   * Resolves a given module URL to an absolute URL based on the loader origin.
   * @param {URL|string} url
   * @param {URL|string} [origin]
   * @return {string}
   */
  resolve (url, origin) {
    return Loader.resolve(url, origin || this.origin)
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.runtime.util.inspect.custom')] () {
    return `Loader ('${this.origin}') { }`
  }
}

export default Loader
