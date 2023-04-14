/**
 * @module IPC
 *
 * This is a low-level API that you don't need unless you are implementing
 * a library on top of Socket SDK. A Socket SDK app has two or three processes.
 *
 * - The `Render` process, is the UI where the HTML, CSS, and JS are run.
 * - The `Bridge` process, is the thin layer of code that manages everything.
 * - The `Main` process, is for apps that need to run heavier compute jobs. And
 *   unlike electron it's optional.
 *
 * The Bridge process manages the Render and Main process, it may also broker
 * data between them.
 *
 * The Binding process uses standard input and output as a way to communicate.
 * Data written to the write-end of the pipe is buffered by the OS until it is
 * read from the read-end of the pipe.
 *
 * The IPC protocol uses a simple URI-like scheme. Data is passed as
 * ArrayBuffers.
 *
 * ```
 * ipc://command?key1=value1&key2=value2...
 * ```
 *
 * Example usage:
 * ```js
 * import { send } from 'socket:ipc'
 * ```
 */

/* global webkit, chrome, external */
import {
  AbortError,
  InternalError,
  TimeoutError
} from './errors.js'

import {
  isArrayLike,
  isBufferLike,
  isPlainObject,
  format,
  parseHeaders,
  parseJSON
} from './util.js'

import * as errors from './errors.js'
import { Buffer } from './buffer.js'
import console from './console.js'

let nextSeq = 1
const cache = {}

function initializeXHRIntercept () {
  if (typeof globalThis.XMLHttpRequest !== 'function') return
  const { send, open } = globalThis.XMLHttpRequest.prototype

  const B5_PREFIX_BUFFER = new Uint8Array([0x62, 0x35]) // literally, 'b5'
  const encoder = new TextEncoder()
  Object.assign(globalThis.XMLHttpRequest.prototype, {
    open (method, url, ...args) {
      try {
        this.readyState = globalThis.XMLHttpRequest.OPENED
      } catch (_) {}
      this.method = method
      this.url = new URL(url)
      this.seq = this.url.searchParams.get('seq')

      return open.call(this, method, url, ...args)
    },

    async send (body) {
      const { method, seq, url } = this
      const index = globalThis.__args?.index || 0

      if (url?.protocol === 'ipc:') {
        if (
          /put|post/i.test(method) &&
          typeof body !== 'undefined' &&
          typeof seq !== 'undefined'
        ) {
          if (typeof body === 'string') {
            body = encoder.encode(body)
          }

          if (/android/i.test(primordials.platform)) {
            await postMessage(`ipc://buffer.map?seq=${seq}`, body)
            body = null
          }

          if (/win32/i.test(primordials.platform) && body) {
            // 1. send `ipc://buffer.create`
            //   - The native side should create a shared buffer for `index` and `seq` pair of `size` bytes
            //   - `index` is the target window
            //   - `seq` is the sequence is used to know how to return the value to the sender
            // 2. wait for 'sharedbufferreceived' event
            //   - The webview will wait for this event on `window`
            //   - The event should include "additional data" that is JSON and includes the `index` and `seq` values
            // 3. filter on `index` and `seq` for this request
            //   - The webview will filter on the `index` and `seq` values before calling `getBuffer()`
            // 4. write `body` to _shared_ `buffer`
            //   - The webview should write all bytes to the buffer
            // 5. resolve promise
            //   - After promise resolution, the XHR request will continue
            //   - The native side should look up the shared buffer for the `index` and `seq` values and use it
            //     as the bytes for the request when routing the IPC request through the bridge router
            //   - The native side should release the shared buffer
            // size here assumes latin1 encoding.
            await postMessage(`ipc://buffer.create?index=${index}&seq=${seq}&size=${body.length}`)
            await new Promise((resolve) => {
              globalThis.chrome.webview
                .addEventListener('sharedbufferreceived', function onSharedBufferReceived (event) {
                  const { additionalData } = event
                  if (additionalData.index === index && additionalData.seq === seq) {
                    const buffer = new Uint8Array(event.getBuffer())
                    buffer.set(body)
                    globalThis.chrome.webview.removeEventListener('sharedbufferreceived', onSharedBufferReceived)
                    resolve()
                  }
                })
            })
          }

          if (/linux/i.test(primordials.platform)) {
            if (body?.buffer instanceof ArrayBuffer) {
              const header = new Uint8Array(24)
              const buffer = new Uint8Array(
                B5_PREFIX_BUFFER.length +
                header.length +
                body.length
              )

              header.set(encoder.encode(index))
              header.set(encoder.encode(seq), 4)

              //  <type> |      <header>     | <body>
              // "b5"(2) | index(2) + seq(2) | body(n)
              buffer.set(B5_PREFIX_BUFFER)
              buffer.set(header, B5_PREFIX_BUFFER.length)
              buffer.set(body, B5_PREFIX_BUFFER.length + header.length)

              let data = []
              const quota = 64 * 1024
              for (let i = 0; i < buffer.length; i += quota) {
                data.push(String.fromCharCode(...buffer.subarray(i, i + quota)))
              }

              data = data.join('')

              try {
                data = decodeURIComponent(escape(data))
              } catch (_) {}
              await postMessage(data)
            }

            body = null
          }
        }
      }

      return send.call(this, body)
    }
  })
}

function getErrorClass (type, fallback) {
  if (typeof globalThis !== 'undefined' && typeof globalThis[type] === 'function') {
    return globalThis[type]
  }

  if (typeof errors[type] === 'function') {
    return errors[type]
  }

  return fallback || Error
}

function getRequestResponseText (request) {
  try {
    // can throw `InvalidStateError` error
    return request?.responseText
  } catch (_) {}
  return null
}

function getRequestResponse (request, options) {
  if (!request) return null
  let { responseType } = request
  const expectedResponseType = options?.responseType ?? responseType
  const desiredResponseType = options?.desiredResponseType ?? expectedResponseType
  const headers = Headers.from(request)
  let response = null

  if (expectedResponseType && responseType !== expectedResponseType) {
    return null
  }

  if (!responseType) {
    responseType = desiredResponseType
  }

  if (!responseType || responseType === 'text') {
    // The `responseText` could be an accessor which could throw an
    // `InvalidStateError` error when accessed when `responseType` is not empty
    // empty or 'text'
    // - see https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/responseText#exceptions
    const responseText = getRequestResponseText(request)
    if (responseText) {
      response = responseText
      // maybe json for unspecified response types
      if (!responseType) {
        const json = parseJSON(response)
        if (json) {
          response = json
        }
      }
    }
  }

  if (responseType === 'json') {
    response = parseJSON(request.response)
  }

  if (responseType === 'arraybuffer') {
    if (
      request.response instanceof ArrayBuffer ||
      typeof request.response === 'string' ||
      isBufferLike(request.response)
    ) {
      const contentLength = parseInt(headers.get('content-length'))
      response = Buffer.from(request.response)
      if (contentLength) {
        response = response.slice(0, contentLength)
      }
    }

    // maybe json in buffered response
    const json = parseJSON(response)
    if ((isPlainObject(json?.data) && json?.source) || isPlainObject(json?.err)) {
      response = json
    }
  }

  // try using `statusText` for stubbed response
  if (!response) {
    // defaults:
    // `400...499: errors.NotAllowedError`
    // `500...599: errors.InternalError`
    const statusCodeToErrorCode = {
      400: errors.BadRequestError,
      403: errors.InvalidAccessError,
      404: errors.NotFoundError,
      408: errors.TimeoutError,
      501: errors.NotSupportedError,
      502: errors.NetworkError,
      504: errors.TimeoutError
    }

    const { status, responseURL, statusText } = request
    const message = Message.from(responseURL)
    const source = message.command

    if (status >= 100 && status < 400) {
      const data = { status: statusText }
      return { source, data }
    } else if (status >= 400 && status < 499) {
      const ErrorType = statusCodeToErrorCode[status] || errors.NotAllowedError
      const err = new ErrorType(statusText || status)
      err.url = responseURL
      return { source, err }
    } else if (status >= 500 && status < 599) {
      const ErrorType = statusCodeToErrorCode[status] || errors.InternalError
      const err = new ErrorType(statusText || status)
      err.url = responseURL
      return { source, err }
    }
  }

  return response
}

function maybeMakeError (error, caller) {
  const errors = {
    AbortError: getErrorClass('AbortError'),
    AggregateError: getErrorClass('AggregateError'),
    EncodingError: getErrorClass('EncodingError'),
    IndexSizeError: getErrorClass('IndexSizeError'),
    InternalError,
    InvalidAccessError: getErrorClass('InvalidAccessError'),
    NetworkError: getErrorClass('NetworkError'),
    NotAllowedError: getErrorClass('NotAllowedError'),
    NotFoundError: getErrorClass('NotFoundError'),
    NotSupportedError: getErrorClass('NotSupportedError'),
    OperationError: getErrorClass('OperationError'),
    RangeError: getErrorClass('RangeError'),
    TimeoutError,
    TypeError: getErrorClass('TypeError'),
    URIError: getErrorClass('URIError')
  }

  if (!error) {
    return null
  }

  if (error instanceof Error) {
    return error
  }

  error = { ...error }
  const type = error.type || 'Error'
  const code = error.code
  let err = null

  delete error.type

  if (type in errors) {
    err = new errors[type](error.message || '')
  } else {
    for (const E of Object.values(errors)) {
      if ((E.code && type === E.code) || (code && code === E.code)) {
        err = new E(error.message || '')
        break
      }
    }
  }

  if (!err) {
    err = new Error(error.message || '')
  }

  // assign extra data to `err` like an error `code`
  for (const key in error) {
    try {
      err[key] = error[key]
    } catch (_) {}
  }

  if (
    typeof Error.captureStackTrace === 'function' &&
    typeof caller === 'function'
  ) {
    Error.captureStackTrace(err, caller)
  }

  return err
}

/**
 * Represents an OK IPC status.
 * @ignore
 */
export const OK = 0

/**
 * Represents an ERROR IPC status.
 * @ignore
 */
export const ERROR = 1

/**
 * Timeout in milliseconds for IPC requests.
 * @ignore
 */
export const TIMEOUT = 32 * 1000

/**
 * Symbol for the `ipc.debug.enabled` property
 * @ignore
 */
export const kDebugEnabled = Symbol.for('ipc.debug.enabled')

/**
 * Parses `seq` as integer value
 * @param {string|number} seq
 * @param {object=} [options]
 * @param {boolean} [options.bigint = false]
 * @ignore
 */
export function parseSeq (seq, options) {
  const value = String(seq).replace(/^R/i, '').replace(/n$/, '')
  return options?.bigint === true ? BigInt(value) : parseInt(value)
}

/**
 * If `debug.enabled === true`, then debug output will be printed to console.
 * @param {(boolean)} [enable]
 * @return {boolean}
 * @ignore
 */
export function debug (enable) {
  if (enable === true) {
    debug.enabled = true
  } else if (enable === false) {
    debug.enabled = false
  }

  return debug.enabled
}

debug.log = () => undefined

Object.defineProperty(debug, 'enabled', {
  enumerable: false,
  set (value) {
    debug[kDebugEnabled] = Boolean(value)
  },

  get () {
    if (debug[kDebugEnabled] === undefined) {
      return Boolean(globalThis?.__args?.debug)
    }

    return debug[kDebugEnabled]
  }
})

/**
 * @ignore
 */
export class Headers extends globalThis.Headers {
  /**
   * @ignore
   */
  static from (input) {
    if (input?.headers) return this.from(input.headers)

    if (typeof input?.entries === 'function') {
      return new this(input.entries())
    } else if (isPlainObject(input) || isArrayLike(input)) {
      return new this(input)
    } else if (typeof input?.getAllResponseHeaders === 'function') {
      input = input.getAllResponseHeaders()
    } else if (typeof input?.headers?.entries === 'function') {
      return new this(input.headers.entries())
    }

    return new this(parseHeaders(String(input)))
  }

  /**
   * @ignore
   */
  get length () {
    return Array.from(this.entries()).length
  }

  /**
   * @ignore
   */
  toJSON () {
    return Object.fromEntries(this.entries())
  }
}

/**
 * @ignore
 */
export async function postMessage (message, ...args) {
  if (globalThis?.webkit?.messageHandlers?.external?.postMessage) {
    return webkit.messageHandlers.external.postMessage(message, ...args)
  } else if (globalThis?.chrome?.webview?.postMessage) {
    return chrome.webview.postMessage(message, ...args)
  } else if (globalThis?.external?.postMessage) {
    return external.postMessage(message, ...args)
  } else if (globalThis.postMessage) {
    // worker
    if (globalThis.self && !globalThis.window) {
      return globalThis?.postMessage({
        __runtime_worker_ipc_request: {
          message,
          bytes: args[0] ?? null
        }
      })
    } else {
      return globalThis?.postMessage(message, args)
    }
  }

  throw new TypeError(
    'Could not determine UserMessageHandler.postMessage in globalThis'
  )
}

/**
 * A container for a IPC message based on a `ipc://` URI scheme.
 * @ignore
 */
export class Message extends URL {
  /**
   * The expected protocol for an IPC message.
   * @ignore
   */
  static get PROTOCOL () {
    return 'ipc:'
  }

  /**
   * Creates a `Message` instance from a variety of input.
   * @param {string|URL|Message|Buffer|object} input
   * @param {(object|string|URLSearchParams)=} [params]
   * @param {(ArrayBuffer|Uint8Array|string)?} [bytes]
   * @return {Message}
   * @ignore
   */
  static from (input, params, bytes = null) {
    const protocol = this.PROTOCOL

    if (isBufferLike(input)) {
      input = Buffer.from(input).toString()
    }

    if (isBufferLike(params)) {
      bytes = Buffer.from(params)
      params = null
    } else if (bytes) {
      bytes = Buffer.from(bytes)
    }

    if (input instanceof Message) {
      const message = new this(String(input))

      if (typeof params === 'object') {
        const entries = params.entries ? params.entries() : Object.entries(params)

        for (const [key, value] of entries) {
          message.set(key, value)
        }
      }

      return message
    } else if (isPlainObject(input)) {
      return new this(
        `${input.protocol || protocol}//${input.command}?${new URLSearchParams({ ...input.params, ...params })}`,
        bytes
      )
    }

    if (typeof input === 'string' && params) {
      return new this(`${protocol}//${input}?${new URLSearchParams(params)}`, bytes)
    }

    // coerce input into a string
    const string = String(input)

    if (string.startsWith(`${protocol}//`)) {
      return new this(string, bytes)
    }

    return new this(`${protocol}//${input}`, bytes)
  }

  /**
   * Predicate to determine if `input` is valid for constructing
   * a new `Message` instance.
   * @param {string|URL|Message|Buffer|object} input
   * @return {boolean}
   * @ignore
   */
  static isValidInput (input) {
    const protocol = this.PROTOCOL
    const string = String(input)

    return (
      string.startsWith(`${protocol}//`) &&
      string.length > protocol.length + 2
    )
  }

  /**
   *  @type {Uint8Array?}
   *  @ignore
   */
  bytes = null

  /**
   * `Message` class constructor.
   * @protected
   * @param {string|URL} input
   * @param {object|Uint8Array?} [bytes]
   * @ignore
   */
  constructor (input, bytes = null) {
    super(input)
    if (this.protocol !== Message.PROTOCOL) {
      throw new TypeError(format(
        'Invalid protocol in input. Expected \'%s\' but got \'%s\'',
        Message.PROTOCOL, this.protocol
      ))
    }

    this.bytes = bytes || null

    const properties = Object.getOwnPropertyDescriptors(Message.prototype)

    Object.defineProperties(this, {
      command: { ...properties.command, enumerable: true },
      seq: { ...properties.seq, enumerable: true },
      index: { ...properties.index, enumerable: true },
      id: { ...properties.id, enumerable: true },
      value: { ...properties.value, enumerable: true },
      params: { ...properties.params, enumerable: true }
    })
  }

  /**
   * Computed IPC message name.
   * @ignore
   */
  get command () {
    // TODO(jwerle): issue deprecation notice
    return this.name
  }

  /**
   * Computed IPC message name.
   * @ignore
   */
  get name () {
    return this.hostname || this.host || this.pathname.slice(2)
  }

  /**
   * Computed `id` value for the command.
   * @ignore
   */
  get id () {
    return this.searchParams.get('id') ?? null
  }

  /**
   * Computed `seq` (sequence) value for the command.
   * @ignore
   */
  get seq () {
    return this.has('seq') ? this.get('seq') : null
  }

  /**
   * Computed message value potentially given in message parameters.
   * This value is automatically decoded, but not treated as JSON.
   * @ignore
   */
  get value () {
    return this.get('value') ?? null
  }

  /**
   * Computed `index` value for the command potentially referring to
   * the window index the command is scoped to or originating from. If not
   * specified in the message parameters, then this value defaults to `-1`.
   * @ignore
   */
  get index () {
    const index = this.get('index')

    if (index !== undefined) {
      const value = parseInt(index)
      if (Number.isFinite(value)) {
        return value
      }
    }

    return -1
  }

  /**
   * Computed value parsed as JSON. This value is `null` if the value is not present
   * or it is invalid JSON.
   * @ignore
   */
  get json () {
    return parseJSON(this.value)
  }

  /**
   * Computed readonly object of message parameters.
   * @ignore
   */
  get params () {
    return Object.fromEntries(this.entries())
  }

  /**
   * Gets unparsed message parameters.
   * @ignore
   */
  get rawParams () {
    return Object.fromEntries(this.searchParams.entries())
  }

  /**
   * Returns computed parameters as entries
   * @return {Array<Array<string,mixed>>}
   * @ignore
   */
  entries () {
    return Array.from(this.searchParams.entries()).map(([key, value]) => {
      return [key, parseJSON(value) || value]
    })
  }

  /**
   * Set a parameter `value` by `key`.
   * @param {string} key
   * @param {mixed} value
   * @ignore
   */
  set (key, value) {
    if (value && typeof value === 'object') {
      value = JSON.stringify(value)
    }

    return this.searchParams.set(key, value)
  }

  /**
   * Get a parameter value by `key`.
   * @param {string} key
   * @param {mixed} defaultValue
   * @return {mixed}
   * @ignore
   */
  get (key, defaultValue) {
    if (!this.has(key)) {
      return defaultValue
    }

    const value = this.searchParams.get(key)
    const json = value && parseJSON(value)

    if (json === null || json === undefined) {
      return value
    }

    return json
  }

  /**
   * Delete a parameter by `key`.
   * @param {string} key
   * @return {boolean}
   * @ignore
   */
  delete (key) {
    if (this.has(key)) {
      return this.searchParams.delete(key)
    }

    return false
  }

  /**
   * Computed parameter keys.
   * @return {Array<string>}
   * @ignore
   */
  keys () {
    return Array.from(this.searchParams.keys())
  }

  /**
   * Computed parameter values.
   * @return {Array<mixed>}
   * @ignore
   */
  values () {
    return Array.from(this.searchParams.values()).map(parseJSON)
  }

  /**
   * Predicate to determine if parameter `key` is present in parameters.
   * @param {string} key
   * @return {boolean}
   * @ignore
   */
  has (key) {
    return this.searchParams.has(key)
  }
}

/**
 * A result type used internally for handling
 * IPC result values from the native layer that are in the form
 * of `{ err?, data? }`. The `data` and `err` properties on this
 * type of object are in tuple form and be accessed at `[data?,err?]`
 * @ignore
 */
export class Result {
  /**
   * @type {Error?}
   * @ignore
   */
  err

  /**
   * @type {string|object|Uint8Array}
   * @ignore
   */
  data

  /**
   * @type {string?}
   * @ignore
   */
  source

  /**
   * @type {Headers?}
   * @ignore
   */
  headers

  /**
   * Creates a `Result` instance from input that may be an object
   * like `{ err?, data? }`, an `Error` instance, or just `data`.
   * @param {object|Error|mixed?} result
   * @param {Error|object} [maybeError]
   * @param {string} [maybeSource]
   * @param {object|string|Headers} [maybeHeaders]
   * @return {Result}
   * @ignore
   */
  static from (result, maybeError = null, maybeSource = null, maybeHeaders = null) {
    if (result instanceof Result) {
      if (!result.source && maybeSource) {
        result.source = maybeSource
      }

      if (!result.err && maybeError) {
        result.err = maybeError
      }

      if (!result.headers && maybeHeaders) {
        result.headers = maybeHeaders
      }

      return result
    }

    if (result instanceof Error) {
      result = { err: result }
    }

    if (!maybeSource && typeof maybeError === 'string') {
      maybeSource = maybeError
      maybeError = null
    }

    const err = maybeMakeError(result?.err || maybeError || null, Result.from)
    const data = !err && result?.data !== null && result?.data !== undefined
      ? result.data
      : (!err ? result : null)

    const source = result?.source || maybeSource || null
    const headers = result?.headers || maybeHeaders || null

    return new this(err, data, source, headers)
  }

  /**
   * `Result` class constructor.
   * @private
   * @param {Error?} [err = null]
   * @param {object?} [data = null]
   * @param {string?} [source = null]
   * @param {object|string|Headers?} [headers = null]
   * @ignore
   */
  constructor (err, data, source, headers) {
    this.err = typeof err !== 'undefined' ? err : null
    this.data = typeof data !== 'undefined' ? data : null
    this.source = typeof source === 'string' && source.length > 0
      ? source
      : null

    this.headers = headers ? Headers.from(headers) : null

    Object.defineProperty(this, 0, {
      get: () => this.err,
      enumerable: false,
      configurable: false
    })

    Object.defineProperty(this, 1, {
      get: () => this.data,
      enumerable: false,
      configurable: false
    })

    Object.defineProperty(this, 2, {
      get: () => this.source,
      enumerable: false,
      configurable: false
    })

    Object.defineProperty(this, 3, {
      get: () => this.headers,
      enumerable: false,
      configurable: false
    })
  }

  /**
   * Computed result length.
   * @ignore
   */
  get length () {
    return Array.from(this).length
  }

  /**
   * Generator for an `Iterable` interface over this instance.
   * @ignore
   */
  * [Symbol.iterator] () {
    if (this.err !== undefined) yield this.err
    if (this.data !== undefined) yield this.data
    if (this.source !== undefined) yield this.source
    if (this.headers !== undefined) yield this.headers
  }

  /**
   * @ignore
   */
  toJSON () {
    return {
      headers: this.headers ?? null,
      source: this.source ?? null,
      data: this.data ?? null,
      err: this.err && {
        // @ts-ignore
        message: this.err.message ?? '',
        // @ts-ignore
        name: this.err.name ?? '',
        // @ts-ignore
        type: this.err.type ?? '',
        // @ts-ignore
        code: this.err.code ?? '',
        ...this.err
      }
    }
  }
}

/**
 * Waits for the native IPC layer to be ready and exposed on the
 * global window object.
 * @ignore
 */
export async function ready () {
  return await new Promise((resolve, reject) => {
    return loop()

    function loop () {
      if (globalThis.__args) {
        queueMicrotask(resolve)
      } else {
        queueMicrotask(loop)
      }
    }
  })
}

/**
 * Sends a synchronous IPC command over XHR returning a `Result`
 * upon success or error.
 * @param {string} command
 * @param {object|string?} [params]
 * @param {object?} [options]
 * @return {Result}
 * @ignore
 */
export function sendSync (command, params = {}, options = {}) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  const request = new globalThis.XMLHttpRequest()
  const index = globalThis.__args?.index ?? 0
  const seq = nextSeq++
  const uri = `ipc://${command}`

  params = new URLSearchParams(params)
  params.set('index', index)
  params.set('seq', 'R' + seq)
  params.set('nonce', Date.now())

  const query = `?${params}`

  if (debug.enabled) {
    debug.log('ipc.sendSync: %s', uri + query)
  }

  request.responseType = options?.responseType ?? ''
  request.open('GET', uri + query, false)
  request.send()

  const response = getRequestResponse(request, options)
  const headers = request.getAllResponseHeaders()
  const result = Result.from(response, null, command, headers)

  if (debug.enabled) {
    debug.log('ipc.sendSync: (resolved)', command, result)
  }

  if (options?.cache === true) {
    cache[command] = result
  }

  return result
}

/**
 * Emit event to be dispatched on `window` object.
 * @param {string} name
 * @param {Mixed} value
 * @param {EventTarget=} [target = window]
 * @param {Object=} options
 */
export async function emit (name, value, target, options) {
  let detail = value
  await ready()

  if (debug.enabled) {
    debug.log('ipc.emit:', name, value, target, options)
  }

  if (typeof value === 'string') {
    try {
      detail = decodeURIComponent(value)
      detail = JSON.parse(detail)
    } catch (err) {
      // consider okay here because if detail is defined then
      // `decodeURIComponent(value)` was successful and `JSON.parse(value)`
      // was not: there could be bad/unsupported unicode in `value`
      if (!detail) {
        console.error(`${err.message} (${value})`)
        return
      }
    }
  }

  const event = new globalThis.CustomEvent(name, { detail, ...options })
  if (target) {
    target.dispatchEvent(event)
  } else {
    globalThis.dispatchEvent(event)
  }
}

/**
 * Resolves a request by `seq` with possible value.
 * @param {string} seq
 * @param {Mixed} value
 * @ignore
 */
export async function resolve (seq, value) {
  await ready()

  if (debug.enabled) {
    debug.log('ipc.resolve:', seq, value)
  }

  const index = globalThis.__args?.index || 0
  const eventName = `resolve-${index}-${seq}`
  const event = new globalThis.CustomEvent(eventName, { detail: value })
  globalThis.dispatchEvent(event)
}

/**
 * Sends an async IPC command request with parameters.
 * @param {string} command
 * @param {Mixed=} value
 * @param {object=} [options]
 * @param {boolean=} [options.cache=false]
 * @param {boolean=} [options.bytes=false]
 * @return {Promise<Result>}
 */
export async function send (command, value, options) {
  await ready()

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  if (debug.enabled) {
    debug.log('ipc.send:', command, value)
  }

  const seq = 'R' + nextSeq++
  const index = value?.index ?? globalThis.__args?.index ?? 0
  let serialized = ''

  try {
    if (value !== undefined && ({}).toString.call(value) !== '[object Object]') {
      value = { value }
    }

    const params = {
      ...value,
      index,
      seq
    }

    serialized = new URLSearchParams(params).toString()
    serialized = serialized.replace(/\+/g, '%20')
  } catch (err) {
    console.error(`${err.message} (${serialized})`)
    return Promise.reject(err.message)
  }

  if (options?.bytes) {
    postMessage(`ipc://${command}?${serialized}`, options?.bytes)
  } else {
    postMessage(`ipc://${command}?${serialized}`)
  }

  return await new Promise((resolve) => {
    const event = `resolve-${index}-${seq}`
    globalThis.addEventListener(event, onresolve, { once: true })
    function onresolve (event) {
      const result = Result.from(event.detail, null, command)
      if (debug.enabled) {
        debug.log('ipc.send: (resolved)', command, result)
      }

      if (options?.cache === true) {
        cache[command] = result
      }

      resolve(result)
    }
  })
}

/**
 * Sends an async IPC command request with parameters and buffered bytes.
 * @param {string} command
 * @param {object=} params
 * @param {(Buffer|TypeArray|ArrayBuffer|string|Array)=} buffer
 * @param {object=} options
 * @ignore
 */
export async function write (command, params, buffer, options) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  await ready()

  const signal = options?.signal
  const request = new globalThis.XMLHttpRequest()
  const index = globalThis?.__args?.index ?? 0
  const seq = nextSeq++
  const uri = `ipc://${command}`

  let resolved = false
  let aborted = false
  let timeout = null

  if (signal) {
    if (signal.aborted) {
      return Result.from(null, new AbortError(signal), command)
    }

    signal.addEventListener('abort', () => {
      if (!aborted && !resolved) {
        aborted = true
        request.abort()
      }
    })
  }

  params = new URLSearchParams(params)
  params.set('index', index)
  params.set('seq', 'R' + seq)
  params.set('nonce', Date.now())

  const query = `?${params}`

  request.responseType = options?.responseType ?? ''
  request.open('POST', uri + query, true)
  await request.send(buffer || null)

  if (debug.enabled) {
    debug.log('ipc.write:', uri + query, buffer || null)
  }

  return await new Promise((resolve) => {
    if (options?.timeout) {
      timeout = setTimeout(() => {
        resolve(Result.from(null, new TimeoutError('ipc.write timedout'), command))
        request.abort()
      }, typeof options.timeout === 'number' ? options.timeout : TIMEOUT)
    }

    request.onabort = () => {
      aborted = true
      if (options?.timeout) {
        clearTimeout(timeout)
      }
      resolve(Result.from(null, new AbortError(signal), command))
    }

    request.onreadystatechange = () => {
      if (aborted) {
        return
      }

      if (request.readyState === globalThis.XMLHttpRequest.DONE) {
        resolved = true
        clearTimeout(timeout)

        const response = getRequestResponse(request, options)
        const headers = request.getAllResponseHeaders()
        const result = Result.from(response, null, command, headers)

        if (debug.enabled) {
          debug.log('ipc.write: (resolved)', command, result)
        }

        return resolve(result)
      }
    }

    request.onerror = () => {
      const headers = request.getAllResponseHeaders()
      const err = new Error(getRequestResponseText(request) || '')
      resolved = true
      clearTimeout(timeout)
      resolve(Result.from(null, err, command, headers))
    }
  })
}

/**
 * Sends an async IPC command request with parameters requesting a response
 * with buffered bytes.
 * @param {string} command
 * @param {object=} params
 * @param {object=} options
 * @ignore
 */
export async function request (command, params, options) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  await ready()

  const request = new globalThis.XMLHttpRequest()
  const signal = options?.signal
  const index = globalThis?.__args?.index ?? 0
  const seq = nextSeq++
  const uri = `ipc://${command}`

  let resolved = false
  let aborted = false
  let timeout = null

  if (signal) {
    if (signal.aborted) {
      return Result.from(null, new AbortError(signal), command)
    }

    signal.addEventListener('abort', () => {
      if (!aborted && !resolved) {
        aborted = true
        request.abort()
      }
    })
  }

  params = new URLSearchParams(params)
  params.set('index', index)
  params.set('seq', 'R' + seq)
  params.set('nonce', Date.now())

  const query = `?${params}`

  request.responseType = options?.responseType ?? ''
  request.open('GET', uri + query)
  request.send(null)

  if (debug.enabled) {
    debug.log('ipc.request:', uri + query)
  }

  return await new Promise((resolve) => {
    if (options?.timeout) {
      timeout = setTimeout(() => {
        resolve(Result.from(null, new TimeoutError('ipc.request timedout'), command))
        request.abort()
      }, typeof options.timeout === 'number' ? options.timeout : TIMEOUT)
    }

    request.onabort = () => {
      aborted = true
      if (options?.timeout) {
        clearTimeout(timeout)
      }

      resolve(Result.from(null, new AbortError(signal), command))
    }

    request.onreadystatechange = () => {
      if (aborted) {
        return
      }

      if (request.readyState === globalThis.XMLHttpRequest.DONE) {
        resolved = true
        clearTimeout(timeout)

        const response = getRequestResponse(request, options)
        const headers = request.getAllResponseHeaders()
        const result = Result.from(response, null, command, headers)

        if (debug.enabled) {
          debug.log('ipc.request: (resolved)', command, result)
        }

        if (options?.cache === true) {
          cache[command] = result
        }

        return resolve(result)
      }
    }

    request.onerror = () => {
      const headers = request.getAllResponseHeaders()
      const err = new Error(getRequestResponseText(request))
      resolved = true
      clearTimeout(timeout)
      resolve(Result.from(null, err, command, headers))
    }
  })
}

/**
 * Factory for creating a proxy based IPC API.
 * @param {string} domain
 * @param {(function|object)=} ctx
 * @param {string=} [ctx.default]
 * @return {Proxy}
 * @ignore
 */
export function createBinding (domain, ctx) {
  const dispatchable = {
    emit,
    ready,
    resolve,
    request,
    send,
    sendSync,
    write
  }

  if (domain && typeof domain === 'object') {
    ctx = domain
    domain = null
  }

  if (typeof ctx !== 'function') {
    ctx = Object.assign(function () {}, ctx)
  }

  const proxy = new Proxy(ctx, {
    apply (target, bound, args) {
      const chain = [...target.chain].slice(0, -1)
      const path = chain.join('.')
      target.chain = new Set()
      const method = (ctx[path]?.method || ctx[path]) || ctx.default || 'send'
      return dispatchable[method](path, ...args)
    },

    get (target, key, receiver) {
      if (key === '__proto__') { return null }
      (ctx.chain ||= new Set()).add(key)
      return new Proxy(ctx, this)
    }
  })

  if (typeof domain === 'string') {
    return proxy[domain]
  }

  return domain
}

// TODO(@chicoxyzzy): generate the primordials file during the build
// We need to set primordials here because we are using the
// `sendSync` method. This is a hack to get around the fact
// that we can't use cyclic imports with a sync call.
/**
 * @ignore
 */
export const primordials = sendSync('platform.primordials')?.data || {}

initializeXHRIntercept()

if (typeof globalThis?.window !== 'undefined') {
  document.addEventListener('DOMContentLoaded', () => {
    queueMicrotask(async () => {
      try {
        await send('platform.event', 'domcontentloaded')
      } catch (err) {
        console.error('ERR:', err)
      }
    })
  })
}

// eslint-disable-next-line
import * as exports from './ipc.js'
export default exports
