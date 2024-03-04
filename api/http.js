import { Writable, Readable, Duplex } from './stream.js'
import { EventEmitter } from './events.js'
import { toProperCase } from './util.js'
import adapters from './http/adapters.js'

/**
 * All known possible HTTP methods.
 * @type {string[]}
 */
export const METHODS = [
  'ACL',
  'BIND',
  'CHECKOUT',
  'CONNECT',
  'COPY',
  'DELETE',
  'GET',
  'HEAD',
  'LINK',
  'LOCK',
  'M-SEARCH',
  'MERGE',
  'MKACTIVITY',
  'MKCALENDAR',
  'MKCOL',
  'MOVE',
  'NOTIFY',
  'OPTIONS',
  'PATCH',
  'POST',
  'PROPFIND',
  'PROPPATCH',
  'PURGE',
  'PUT',
  'REBIND',
  'REPORT',
  'SEARCH',
  'SOURCE',
  'SUBSCRIBE',
  'TRACE',
  'UNBIND',
  'UNLINK',
  'UNLOCK',
  'UNSUBSCRIBE'
]

/**
 * A mapping of status codes to status texts
 * @type {object}
 */
export const STATUS_CODES = {
  100: 'Continue',
  101: 'Switching Protocols',
  102: 'Processing',
  103: 'Early Hints',
  200: 'OK',
  201: 'Created',
  202: 'Accepted',
  203: 'Non-Authoritative Information',
  204: 'No Content',
  205: 'Reset Content',
  206: 'Partial Content',
  207: 'Multi-Status',
  208: 'Already Reported',
  226: 'IM Used',
  300: 'Multiple Choices',
  301: 'Moved Permanently',
  302: 'Found',
  303: 'See Other',
  304: 'Not Modified',
  305: 'Use Proxy',
  307: 'Temporary Redirect',
  308: 'Permanent Redirect',
  400: 'Bad Request',
  401: 'Unauthorized',
  402: 'Payment Required',
  403: 'Forbidden',
  404: 'Not Found',
  405: 'Method Not Allowed',
  406: 'Not Acceptable',
  407: 'Proxy Authentication Required',
  408: 'Request Timeout',
  409: 'Conflict',
  410: 'Gone',
  411: 'Length Required',
  412: 'Precondition Failed',
  413: 'Payload Too Large',
  414: 'URI Too Long',
  415: 'Unsupported Media Type',
  416: 'Range Not Satisfiable',
  417: 'Expectation Failed',
  418: "I'm a Teapot",
  421: 'Misdirected Request',
  422: 'Unprocessable Entity',
  423: 'Locked',
  424: 'Failed Dependency',
  425: 'Too Early',
  426: 'Upgrade Required',
  428: 'Precondition Required',
  429: 'Too Many Requests',
  431: 'Request Header Fields Too Large',
  451: 'Unavailable For Legal Reasons',
  500: 'Internal Server Error',
  501: 'Not Implemented',
  502: 'Bad Gateway',
  503: 'Service Unavailable',
  504: 'Gateway Timeout',
  505: 'HTTP Version Not Supported',
  506: 'Variant Also Negotiates',
  507: 'Insufficient Storage',
  508: 'Loop Detected',
  509: 'Bandwidth Limit Exceeded',
  510: 'Not Extended',
  511: 'Network Authentication Required'
}

/**
 * The parent class of `ClientRequest` and `ServerResponse`.
 * It is an abstract outgoing message from the perspective of the
 * participants of an HTTP transaction.
 * @see {@link https://nodejs.org/api/http.html#class-httpoutgoingmessage}
 */
export class OutgoingMessage extends Writable {
  #headers = new Headers()
  #timeout = null
  #finished = false

  /**
   * `true` if the headers were sent
   * @type {boolean}
   */
  headersSent = false

  /**
   * `OutgoingMessage` class constructor.
   * @ignore
   */
  constructor () {
    super()
    this.once('finish', () => {
      this.#finished = true
      if (this.#timeout) {
        clearTimeout(this.#timeout)
      }
    })
  }

  /**
   * An object of the outgoing message headers.
   * This is equivalent to `getHeaders()`
   * @type {object}
   */
  get headers () {
    return Object.fromEntries(this.#headers.entries())
  }

  /**
   * @ignore
   */
  get socket () {
    return this
  }

  /**
   * `true` if the write state is "ended"
   * @type {boolean}
   */
  get writableEnded () {
    return this._writableState?.ended === true
  }

  /**
   * `true` if the write state is "finished"
   * @type {boolean}
   */
  get writableFinished () {
    return this.#finished
  }

  /**
   * The number of buffered bytes.
   * @type {number}
   */
  get writableLength () {
    return this._writableState.buffered
  }

  /**
   * @ignore
   * @type {boolean}
   */
  get writableObjectMode () {
    return false
  }

  /**
   * @ignore
   */
  get writableCorked () {
    return 0
  }

  /**
   * The `highWaterMark` of the writable stream.
   * @type {number}
   */
  get writableHighWaterMark () {
    return this._writableState.highWaterMark
  }

  /**
   * @ignore
   * @return {OutgoingMessage}
   */
  addTrailers (headers) {
    // not supported
    return this
  }

  /**
   * @ignore
   * @return {OutgoingMessage}
   */
  cork () {
    // not supported
    return this
  }

  /**
   * @ignore
   * @return {OutgoingMessage}
   */
  uncork () {
    // not supported
    return this
  }

  /**
   * Destroys the message.
   * Once a socket is associated with the message and is connected,
   * that socket will be destroyed as well.
   * @param {Error?} [err]
   * @return {OutgoingMessage}
   */
  destroy (err = null) {
    super.destroy(err)
    return this
  }

  /**
   * Finishes the outgoing message.
   * @param {(Buffer|Uint8Array|string|function)=} [chunk]
   * @param {(string|function)=} [encoding]
   * @param {function=} [callback]
   * @return {OutgoingMessage}
   */
  end (chunk = null, encoding = null, callback = null) {
    if (typeof chunk === 'function') {
      callback = chunk
      chunk = null
      encoding = null
    } else if (typeof encoding === 'function') {
      callback = encoding
      encoding = null
    }

    if (typeof callback === 'function') {
      this.once('finish', callback)
    }

    this.emit('prefinish')
    super.end(chunk)
    return this
  }

  /**
   * Append a single header value for the header object.
   * @param {string} name
   * @param {string|string[]} value
   * @return {OutgoingMessage}
   */
  appendHeader (name, value) {
    if (name && typeof name === 'string') {
      if (Array.isArray(value)) {
        for (const v of value) {
          this.#headers.append(name.toLowerCase(), v)
        }
      } else {
        this.#headers.append(name.toLowerCase(), value)
      }
    }
    return this
  }

  /**
   * Append a single header value for the header object.
   * @param {string} name
   * @param {string} value
   * @return {OutgoingMessage}
   */
  setHeader (name, value) {
    if (name && typeof name === 'string') {
      this.#headers.set(name.toLowerCase(), value)
    }
    return this
  }

  /**
   * Flushes the message headers.
   */
  flushHeaders () {
    queueMicrotask(() => this.emit('flushheaders'))
  }

  /**
   * Gets the value of the HTTP header with the given name.
   * If that header is not set, the returned value will be `undefined`.
   * @param {string}
   * @return {string|undefined}
   */
  getHeader (name) {
    if (name && typeof name === 'string') {
      return this.#headers.get(name.toLowerCase()) ?? undefined
    }

    return undefined
  }

  /**
   * Returns an array containing the unique names of the current outgoing
   * headers. All names are lowercase.
   * @return {string[]}
   */
  getHeaderNames () {
    return Array.from(this.#headers.keys())
  }

  /**
   * @ignore
   */
  getRawHeaderNames () {
    return this.getHeaderNames()
      .map((name) => name.split('-').map(toProperCase).join('-'))
  }

  /**
   * Returns a copy of the HTTP headers as an object.
   * @return {object}
   */
  getHeaders () {
    return Object.fromEntries(this.#headers.entries())
  }

  /**
   * Returns true if the header identified by name is currently set in the
   * outgoing headers. The header name is case-insensitive.
   * @param {string} name
   * @return {boolean}
   */
  hasHeader (name) {
    if (name && typeof name === 'string') {
      return this.#headers.has(name.toLowerCase())
    }

    return false
  }

  /**
   * Removes a header that is queued for implicit sending.
   * @param {string} name
   */
  removeHeader (name) {
    if (name && typeof name === 'string') {
      this.#headers.delete(name.toLowerCase())
    }
  }

  /**
   * Sets the outgoing message timeout with an optional callback.
   * @param {number} timeout
   * @param {function=} [callback]
   * @return {OutgoingMessage}
   */
  setTimeout (timeout, callback = null) {
    if (!timeout || !Number.isFinite(timeout)) {
      throw new TypeError('Expecting a finite integer for a timeout')
    }

    if (typeof callback === 'function') {
      this.once('timeout', callback)
    }

    this.#timeout = setTimeout(() => {
      this.emit('timeout')
    }, timeout)

    return this
  }
}

/**
 * An `IncomingMessage` object is created by `Server` or `ClientRequest` and
 * passed as the first argument to the 'request' and 'response' event
 * respectively.
 * It may be used to access response status, headers, and data.
 * @see {@link https://nodejs.org/api/http.html#class-httpincomingmessage}
 */
export class IncomingMessage extends Readable {
  #httpVersionMajor = 1
  #httpVersionMinor = 1
  #statusMessage = null
  #statusCode = 0
  #complete = false
  #headers = new Headers()
  #timeout = null
  #method = 'GET'
  #url = null

  /**
   * `IncomingMessage` class constructor.
   * @ignore
   * @param {object} options
   */
  constructor (options) {
    super()
    if (options?.headers) {
      this.#headers = new Headers(options.headers)
    }

    // let construction decide this
    if (options?.complete === true) {
      this.#complete = true
    } else {
      this.once('complete', () => {
        this.complete = true
        clearTimeout(this.#timeout)
      })
    }

    if (options?.method && METHODS.includes(options.method)) {
      this.#method = options.method
    }

    if (
      options?.statusCode &&
      Number.isFinite(options.statusCode) &&
      STATUS_CODES[options.statusCode]
    ) {
      this.#statusCode = options.statusCode
      this.#statusMessage = (
        options.statusMessage ??
        STATUS_CODES[options.statusCode]
      )
    }

    if (options?.url) {
      let url = options.url
      if (typeof url === 'string') {
        if (URL.canParse(url)) {
          url = new URL(url)
        }
      }

      if (url instanceof URL) {
        const { hostname, pathname, search } = url
        this.#url = `${pathname}${search}`
        this.#headers.set('host', hostname)
      } else if (typeof url === 'string') {
        if (!url.startsWith('/')) {
          url = `/${url}`
        }

        this.#url = url
      } else {
        throw new TypeError('Invalid URL given')
      }
    }
  }

  /**
   * This property will be `true` if a complete HTTP message has been received
   * and successfully parsed.
   * @type {boolean}
   */
  get complete () {
    return this.#complete
  }

  /**
   * An object of the incoming message headers.
   * @type {object}
   */
  get headers () {
    return Object.fromEntries(this.#headers.entries())
  }

  /**
   * Similar to `message.headers`, but there is no join logic and the values
   * are always arrays of strings, even for headers received just once.
   * @type {object}
   */
  get headersDistinct () {
    const { headers } = this
    for (const key in headers) {
      headers[key] = headers[key].split(',')
    }
    return headers
  }

  /**
   * The HTTP major version of this request.
   * @type {number}
   */
  get httpVersionMajor () {
    return this.#httpVersionMajor
  }

  /**
   * The HTTP minor version of this request.
   * @type {number}
   */
  get httpVersionMinor () {
    return this.#httpVersionMinor
  }

  /**
   * The HTTP version string.
   * A concatenation of `httpVersionMajor` and `httpVersionMinor`.
   * @type {string}
   */
  get httpVersion () {
    return `${this.httpVersionMajor}.${this.httpVersionMinor}`
  }

  /**
   * The HTTP request method.
   * @type {string}
   */
  get method () {
    return this.#method
  }

  /**
   * The raw request/response headers list potentially  as they were received.
   * @type {string[]}
   */
  get rawHeaders () {
    return Array.from(this.#headers.entries()).reduce((h, e) => h.concat(e), [])
  }

  /**
   * @ignore
   */
  get rawTrailers () {
    // not supported
    return []
  }

  /**
   * @ignore
   */
  get socket () {
    return this
  }

  /**
   * The HTTP request status code.
   * Only valid for response obtained from `ClientRequest`.
   * @type {number}
   */
  get statusCode () {
    return this.#statusCode
  }

  /**
   * The HTTP response status message (reason phrase).
   * Such as "OK" or "Internal Server Error."
   * Only valid for response obtained from `ClientRequest`.
   * @type {string?}
   */
  get statusMessage () {
    return this.#statusMessage
  }

  /**
   * An alias for `statusCode`
   * @type {number}
   */
  get status () {
    return this.#statusCode
  }

  /**
   * An alias for `statusMessage`
   * @type {string?}
   */
  get statusText () {
    return this.#statusMessage
  }

  /**
   * @ignore
   */
  get trailers () {
    // not supported
    return {}
  }

  /**
   * @ignore
   */
  get trailersDistinct () {
    // not supported
    return {}
  }

  /**
   * Sets the incoming message timeout with an optional callback.
   * @param {number} timeout
   * @param {function=} [callback]
   * @return {IncomingMessage}
   */
  setTimeout (timeout, callback = null) {
    if (!timeout || !Number.isFinite(timeout)) {
      throw new TypeError('Expecting a finite integer for a timeout')
    }

    if (this.complete) {
      return this
    }

    if (typeof callback === 'function') {
      this.once('timeout', callback)
    }

    this.#timeout = setTimeout(() => {
      this.emit('timeout')
    }, timeout)

    return this
  }
}

/**
 * An object that is created internally and returned from `request()`.
 */
export class ClientRequest extends OutgoingMessage {
  #method = null
  #agent = null
  #url = null

  #maxHeadersCount = 2000

  /**
   * `ClientRequest` class constructor.
   * @ignore
   * @param {object} options
   */
  constructor (options) {
    super()

    this.#agent = options?.agent ?? null

    if (this.#agent) {
      this.#agent.requests.add(this)
      this.once('finish', () => {
        this.#agent.requests.delete(this)
      })
    }

    if (options?.method && METHODS.includes(options.method)) {
      this.#method = options.method
    }

    if (options?.url) {
      let url = options.url
      if (typeof url === 'string') {
        if (URL.canParse(url)) {
          url = new URL(url)
        } else if (URL.canParse(url, globalThis.location.origin)) {
          url = new URL(url, globalThis.location.origin)
        } else {
          url = null
        }
      }

      if (url instanceof URL) {
        const { hostname, pathname, search } = url
        this.#url = `${pathname}${search}`
        this.setHeader('host', hostname)
      } else {
        throw new TypeError('Invalid URL given')
      }
    }
  }

  /**
   * The request protocol
   * @type {string?}
   */
  get protocol () {
    return this.#url?.protoocl ?? null
  }

  /**
   * The request path.
   * @type {string}
   */
  get path () {
    if (!this.#url) {
      return null
    }

    return this.#url.pathname + this.#url.search
  }

  /**
   * The request host name (including port).
   * @type {string?}
   */
  get host () {
    return this.#url?.hostname ?? null
  }

  /**
   * @ignore
   * @type {boolean}
   */
  get finished () {
    return this.writableEnded
  }

  /**
   * @ignore
   * @type {boolean}
   */
  get reusedSocket () {
    return false
  }

  /**
   * @ignore
   * @param {boolean=} [value]
   * @return {ClientRequest}
   */
  setNoDelay (value = false) {
    // not supported
    return this
  }

  /**
   * @ignore
   * @param {boolean=} [enable]
   * @param {number=} [initialDelay]
   * @return {ClientRequest}
   */
  setSocketKeepAlive (enable = false, initialDelay = 0) {
    // not supported
    return this
  }
}

/**
 * An object that is created internally by a `Server` instance, not by the user.
 * It is passed as the second parameter to the 'request' event.
 */
export class ServerResponse extends OutgoingMessage {
  #statusMessage = STATUS_CODES[200]
  #statusCode = 200
  #request = null
  #sendDate = true

  /**
   * `ServerResponse` class constructor.
   * @param {object} options
   */
  constructor (options) {
    super()
    this.#request = options?.request ?? null
  }

  /**
   * A reference to the original HTTP request object.
   * @type {IncomingMessage}
   */
  get request () {
    return this.#request
  }

  /**
   * A reference to the original HTTP request object.
   * @type {IncomingMessage}
   */
  get req () {
    return this.request
  }

  /**
   * The HTTP request status code.
   * Only valid for response obtained from `ClientRequest`.
   * @type {number}
   */
  get statusCode () {
    return this.#statusCode
  }

  /**
   * The HTTP response status message (reason phrase).
   * Such as "OK" or "Internal Server Error."
   * Only valid for response obtained from `ClientRequest`.
   * @type {string?}
   */
  get statusMessage () {
    return this.#statusMessage
  }

  /**
   * An alias for `statusCode`
   * @type {number}
   */
  get status () {
    return this.#statusCode
  }

  /**
   * An alias for `statusMessage`
   * @type {string?}
   */
  get statusText () {
    return this.#statusMessage
  }

  /**
   * If `true`, the "Date" header will be automatically generated and sent in
   * the response if it is not already present in the headers.
   * Defaults to `true`.
   * @type {boolean}
   */
  get sendDate () { return this.#sendDate }
  set sendDate (value) {
    if (typeof value === 'boolean') {
      this.#sendDate = value
    }
  }

  /**
   * @ignore
   */
  writeContinue () {
    // not supported
    return this
  }

  /**
   * @ignore
   */
  writeEarlyHints () {
    // not supported
    return this
  }

  /**
   * @ignore
   */
  writeProcessing () {
    // not supported
    return this
  }

  /**
   * Writes the response header to the request.
   * The `statusCode` is a 3-digit HTTP status code, like 200 or 404.
   * The last argument, `headers`, are the response headers.
   * Optionally one can give a human-readable `statusMessage`
   * as the second argument.
   * @param {number|string} statusCode
   * @param {string|object|string[]} [statusMessage]
   * @param {object|string[]} [headers]
   * @return {ClientRequest}
   */
  writeHead (statusCode, statusMessage = null, headers = null) {
    if (
      statusMessage &&
      (typeof statusMessage === 'object' || Array.isArray(statusMessage))
    ) {
      headers = statusMessage
      statusMessage = null
    }

    this.#statusCode = parseInt(statusCode)
    this.#statusMessage = statusMessage ?? STATUS_CODES[statusCode]

    if (Array.isArray(headers)) {
      for (let i = 0; i < headers.length; i += 2) {
        const key = headers[i]
        const value = headers[i + 1]
        this.appendHeader(key, value)
      }
    } else if (headers && typeof headers === 'object') {
      for (const key in headers) {
        const value = headers[key]
        this.setHeader(key, value)
      }
    }

    return this
  }
}

/**
 * An options object container for an `Agent` instance.
 */
export class AgentOptions {
  keepAlive = false
  timeout = -1

  /**
   * `AgentOptions` class constructor.
   * @ignore
   * @param {{
   *   keepAlive?: boolean,
   *   timeout?: number
   * }} [options]
   */
  constructor (options) {
    this.keepAlive = options?.keepAlive === true
    this.timeout = Number.isFinite(options?.timeout) && options.timeout > 0
      ? options.timeout
      : -1
  }
}

/**
 * An Agent is responsible for managing connection persistence
 * and reuse for HTTP clients.
 */
export class Agent extends EventEmitter {
  defaultProtocol = 'http:'
  options = null
  requests = new Set()
  sockets = {}

  // unused
  maxFreeSockets = 256
  maxTotalSockets = Infinity
  maxSockets = Infinity

  /**
   * `Agent` class constructor.
   * @param {AgentOptions=} [options]
   */
  constructor (options = null) {
    super()
    this.options = new AgentOptions(options)
  }

  /**
   * @ignore
   */
  get freeSockets () {
    return {}
  }

  /**
   * @ignore
   * @param {object} options
   */
  getName (options) {
    const {
      host = '',
      port = '',
      localAddress = '',
      family = ''
    } = options ?? {}

    if (host && port && localAddress && family) {
      return [host, port, localAddress, family].join(':')
    }

    return [host, port, localAddress].join(':')
  }

  /**
   * Produces a socket/stream to be used for HTTP requests.
   * @param {object} options
   * @param {function(Duplex)=} [callback]
   * @return {Duplex}
   */
  createConnection (options, callback = null) {
    let controller = null
    let timeout = null
    let url = null

    const abortController = new AbortController()
    const readable = new ReadableStream({ start (c) { controller = c } })
    const pending = { callbacks: [], data: [] }

    const stream = new Duplex({
      signal: abortController.signal,
      write (data, cb) {
        controller.enqueue(data)
        cb(null)
      },

      read (cb) {
        if (pending.data.length) {
          const data = pending.data.shift()
          this.push(data)
          cb(null)
        } else {
          pending.callbacks.push(cb)
        }
      }
    })

    stream.on('finish', () => readable.close())

    url = `${options.protocol ?? this.defaultProtocol}//`
    url += (options.host || options.hostname)
    if (options.port) {
      url += `:${options.port}`
    }

    url += (options.path || options.pathname)

    if (options.signal) {
      options.signal.addEventListener('abort', () => {
        abortController.abort(options.signal.reason)
      })
    }

    if (options.timeout) {
      timeout = setTimeout(() => {
        abortController.abort('Connection timed out')
        stream.emit('timeout')
      }, options.timeout)
    }

    abortController.signal.addEventListener('abort', () => {
      stream.emit('aborted')
      stream.emit('error', Object.assign(new Error('aborted'), { code: 'ECONNRESET' }))
    })

    const deferredRequestPromise = options.makeRequest
      ? options.makeRequest()
      : Promise.resolve()

    deferredRequestPromise.then(makeRequest)

    function makeRequest (req) {
      const request = fetch(url, {
        headers: Object.fromEntries(
          Array.from(Object.entries(
            options.headers?.entries?.() ?? options.headers ?? {}
          )).concat(req.headers.entries())
        ),
        signal: abortController.signal,
        method: options.method ?? 'GET',
        body: /put|post/i.test(options.method ?? '')
          ? readable
          : undefined
      })

      if (options.handleResponse) {
        request.then(options.handleResponse)
      }

      request.finally(() => clearTimeout(timeout))
      request
        .then((response) => {
          if (response.body) {
            return response.body.getReader()
          }

          return response
            .blob()
            .then((blob) => blob.stream().getReader())
        })
        .then((reader) => {
          read()
          function read () {
            reader.read()
              .then(({ done, value }) => {
                if (pending.callbacks.length) {
                  const cb = pending.callbacks.shift()
                  stream.push(value)
                  cb(null)
                } else {
                  pending.data.push(value ?? null)
                }

                if (!done) {
                  read()
                }
              })
          }
        })

      if (typeof callback === 'function') {
        callback(stream)
      }
    }

    return stream
  }

  /**
   * @ignore
   */
  keepSocketAlive () {
    // not supported
  }

  /**
   * @ignore
   */
  reuseSocket () {
    // not supported
  }

  destroy () {
    for (const request of this.requests) {
      if (typeof request?.destroy === 'function') {
        request.destroy()
      }
    }
  }
}

/**
 * The global and default HTTP agent.
 * @type {Agent}
 */
export const globalAgent = new Agent()

/**
 * A nodejs compat HTTP server typically intended for running in a "worker"
 * environment.
 */
export class Server extends EventEmitter {
  #maxConnections = Infinity
  #listening = false
  #adapter = null
  #closed = false

  // unused
  headersTimeout = 60000

  /**
   * `true` if the server is listening for requests.
   * @type {boolean}
   */
  get listening () {
    return this.#listening
  }

  /**
   * The number of concurrent max connections this server should handle.
   * Default: Infinity
   * @type {number}
   */
  get maxConnections () { return this.#maxConnections }
  set maxConnections (value) {
    if (value && typeof value === 'number' && value > 0) {
      this.#maxConnections = value
    }
  }

  /**
   * TODO
   * @param {function=} [close]
   */
  close (callback = null) {
    if (typeof callback === 'function') {
      this.once('close', callback)
    }

    this.#closed = true
    // TODO(@jwerle): close server
  }

  /**
   * TODO
   */
  closeAllConnections () {
    // TODO(@jwerle): close all connections
  }

  /**
   * TODO
   */
  closeIdleConnections () {
    // TODO(@jwerle): close "idle" connections
  }

  /**
   * TODO
   */
  listen (port, host, unused, callback) {
    if (typeof unused === 'function') {
      callback = unused
    }

    if (port && typeof port === 'object') {
      const options = port

      if (typeof host === 'function') {
        callback = host
      }

      port = options.port ?? 0
      host = options.host ?? globalThis.location.hostname
    }

    if (typeof callback === 'function') {
      this.once('listening', callback)
    }

    if (globalThis.isServiceWorkerScope === true) {
      this.#adapter = new adapters.ServiceWorkerServerAdapter(this, {
        globalAgent,
        IncomingMessage,
        ServerResponse
      })

      this.#adapter.addEventListener('activate', () => {
        this.#listening = true
        this.emit('listening')
      })
    }

    return this
  }
}

/**
 * Makes a HTTP request, optionally a `socket://` for relative paths when
 * `socket:` is the origin protocol.
 * @param {string|object} optionsOrURL
 * @param {(object|function)=} [options]
 * @param {function=} [callback]
 * @return {ClientRequest}
 */
async function request (optionsOrURL, options, callback) {
  if (typeof options === 'function') {
    callback = options
  }

  if (optionsOrURL && typeof optionsOrURL === 'object') {
    options = optionsOrURL
    callback = options
  } else if (typeof optionsOrURL === 'string') {
    const url = globalThis.location.origin.startsWith('blob')
      ? new URL(optionsOrURL, new URL(globalThis.location.origin).pathname)
      : new URL(optionsOrURL, globalThis.location.origin)

    options = {
      host: url.host,
      port: url.port,
      pathname: url.pathname,
      protocol: url.protocol,
      ...options
    }
  }

  let agent = null

  if (options.agent) {
    agent = options.agent
  } else if (options.agent === false) {
    agent = new (options.Agent ?? Agent)()
  } else {
    agent = globalAgent
  }

  let url = `${options.protocol ?? agent?.defaultProtocol ?? 'http:'}//${options.host || options.hostname}`

  if (options.port) {
    url += `:${options.port}`
  }

  url += options.pathname ?? '/'

  const request = new ClientRequest({
    method: options?.method ?? 'GET',
    agent,
    url
  })

  options = {
    ...options,
    makeRequest () {
      return new Promise((resolve) => {
        if (!/post|put/i.test(options.method ?? '')) {
          resolve(request)
        } else {
          stream.on('finish', () => resolve(request))
        }

        request.headersSent = true
      })
    },

    handleResponse (response) {
      const incomingMessage = new IncomingMessage({
        statusMessage: response.statusText,
        statusCode: response.status,
        complete: true,
        headers: response.headers,
        method: request.method,
        url: response.url
      })

      stream.response = response
      stream.pipe(incomingMessage)
      request.emit('response', incomingMessage)
    }
  }

  const stream = agent.createConnection(options, callback)

  stream.on('finish', () => request.emit('finish'))
  stream.on('timeout', () => request.emit('timeout'))

  return request
}

/**
 * Makes a HTTP or `socket:` GET request. A simplified alias to `request()`.
 * @param {string|object} optionsOrURL
 * @param {(object|function)=} [options]
 * @param {function=} [callback]
 * @return {ClientRequest}
 */
export function get (optionsOrURL, options, callback) {
  return request(optionsOrURL, options, callback)
}

/**
 * Creates a HTTP server that can listen for incoming requests.
 * Requests that are dispatched to this server depend on the context
 * in which it is created, such as a service worker which will use a
 * "fetch event" adapter.
 * @param {object|function=} [options]
 * @param {function=} [callback]
 * @return {Server}
 */
export function createServer (options = null, callback = null) {
  if (typeof options === 'function') {
    callback = options
    options = null
  }

  const server = new Server(options)

  if (typeof callback === 'function') {
    server.on('request', callback)
  }

  return server
}

export default {
  Agent,
  AgentOptions,
  ClientRequest,
  createServer,
  get,
  globalAgent,
  METHODS,
  OutgoingMessage,
  request,
  Server,
  ServerResponse,
  STATUS_CODES
}
