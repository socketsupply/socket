/* global XMLHttpRequest */
import { Headers } from '../ipc.js'
import location from '../location.js'
import path from '../path.js'

const RUNTIME_REQUEST_SOURCE_HEADER = 'Runtime-Request-Source'

/**
 * @typedef {{
 *   extensions?: string[] | Set<string>
 *   origin?: URL | string,
 *   statuses?: Map
 *   cache?: Map
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
 *   headers?: Headers | object
 * }} RequestLoadOptions
 */

/**
 * @typedef {{
 *   request?: Request,
 *   headers?: Headers,
 *   status?: number,
 *   text?: string
 * }} ResponseOptions
 */

/**
 * A container for the status of a CommonJS resource. A `RequestStatus` object
 * represents meta data for a `Request` that comes from a preflight
 * HTTP HEAD request.
 */
export class RequestStatus {
  #status = undefined
  #request = null
  #headers = new Headers()

  /**
   * `RequestStatus` class constructor.
   * @param {Request} request
   */
  constructor (request) {
    if (!request || !(request instanceof Request)) {
      throw new TypeError(
        `Expecting 'request' to be a Request object. Received: ${request}`
      )
    }

    this.#request = request
  }

  /**
   * The unique ID of this `RequestStatus`, which is the absolute URL as a string.
   * @type {string}
   */
  get id () {
    return this.#request.id
  }

  /**
   * The origin for this `RequestStatus` object.
   * @type {string}
   */
  get origin () {
    return this.#request.origin
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

    return this.#request.url.pathname + this.#request.url.search
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

    if (options?.headers && typeof options?.headers === 'object') {
      const entries = typeof options.headers.entries === 'function'
        ? options.headers.entries()
        : Object.entries(options.headers)

      for (const entry of entries) {
        request.setRequestHeader(entry[0], entry[1])
      }
    }

    request.send(null)

    this.#headers = Headers.from(request)
    this.#status = request.status

    const contentLocation = this.#headers.get('content-location')

    // verify 'Content-Location' header if given in response
    if (contentLocation && URL.canParse(contentLocation, this.origin)) {
      const url = new URL(contentLocation, this.origin)
      const extension = path.extname(url.pathname)

      if (!this.#request.loader.extensions.has(extension)) {
        this.#status = 404
        return this
      }
    }

    return this
  }
}

/**
 * A container for a synchronous CommonJS request to local resource or
 * over the network.
 */
export class Request {
  #id = null
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
    this.#status = new RequestStatus(this)
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
      if (this.#loader.cache.has(this.id)) {
        return this.#loader.cache.get(this.id)
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

    if (options?.headers && typeof options?.headers === 'object') {
      const entries = typeof options.headers.entries === 'function'
        ? options.headers.entries()
        : Object.entries(options.headers)

      for (const entry of entries) {
        request.setRequestHeader(entry[0], entry[1])
      }
    }

    request.send(null)

    let responseText = null

    try {
      // @ts-ignore
      responseText = request.responseText // can throw `InvalidStateError` error
    } catch {
      responseText = request.response
    }

    return new Response(this, {
      headers: Headers.from(request),
      status: request.status,
      text: responseText
    })
  }
}

/**
 * A container for a synchronous CommonJS request response for a local resource
 * or over the network.
 */
export class Response {
  #request = null
  #headers = new Headers()
  #status = 404
  #text = ''

  /**
   * `Response` class constructor.
   * @param {Request|ResponseOptions} request
   * @param {ResponseOptions=} [options]
   */
  constructor (request, options = null) {
    options = { ...options }

    if (!typeof request === 'object' && !(request instanceof Request)) {
      options = request
      request = options.request
    }

    if (!request || !(request instanceof Request)) {
      throw new TypeError(
        `Expecting 'request' to be a Request object. Received: ${request}`
      )
    }

    this.#request = request
    this.#headers = options.headers
    this.#status = options.status || 404
    this.#text = options.text || ''

    if (request.loader) {
      // cache request response in the loader
      request.loader.cache.set(request.id, this)
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
    return this.#text
  }

  /**
   * `true` if the response is considered OK, otherwise `false`.
   * @type {boolean}
   */
  get ok () {
    return this.status >= 200 && this.status < 400
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
  static defaultExtensions = new Set(['.js', '.json', '.mjs', '.cjs', '.jsx', '.ts', '.tsx'])

  #cache = null
  #origin = null
  #statuses = null
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

    this.#cache = options?.cache instanceof Map ? options.cache : new Map()
    this.#origin = Loader.resolve('.', origin)
    this.#statuses = options?.statuses instanceof Map
      ? options.statuses
      : new Map()

    if (options?.extensions && typeof options.extensions === 'object') {
      if (Array.isArray(options.extensions) || options instanceof Set) {
        for (const value of options.extensions) {
          const extension = !value.startsWith('.') ? `.${value}` : value
          this.#extensions.add(extension.trim())
        }
      }
    }
  }

  /**
   * The internal cache for this `Loader` object.
   * @type {Map}
   */
  get cache () {
    return this.#cache
  }

  /**
   * The internal statuses for this `Loader` object.
   * @type {Map}
   */
  get statuses () {
    return this.#statuses
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
      origin = this.origin
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
  status (url, origin, options) {
    if (origin && typeof origin === 'object' && !(origin instanceof URL)) {
      options = origin
      origin = this.origin
    }

    if (!origin) {
      origin = this.origin
    }

    url = this.resolve(url, origin)

    if (this.#statuses.has(url)) {
      return this.statuses.get(url)
    }

    const request = new Request(url, {
      loader: this,
      origin
    })

    this.#statuses.set(url, request.status)
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
}

export default Loader
