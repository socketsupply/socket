/**
 * @module ipc
 *
 * This is a low-level API that you don't need unless you are implementing
 * a library on top of Socket runtime. A Socket app has one or more processes.
 *
 * When you need to send a message to another window or to the backend, you
 * should use the `application` module to get a reference to the window and
 * use the `send` method to send a message.
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

/* global webkit, chrome, external, reportError */
import {
  AbortError,
  InternalError,
  ErrnoError,
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

import { URL, protocols } from './url.js'
import * as errors from './errors.js'
import { Buffer } from './buffer.js'
import { rand64 } from './crypto.js'
import bookmarks from './fs/bookmarks.js'
import serialize from './internal/serialize.js'
import location from './location.js'
import gc from './gc.js'

let nextSeq = 1
const cache = {}

function initializeXHRIntercept () {
  if (typeof globalThis.XMLHttpRequest !== 'function') return
  const patched = Symbol.for('socket.runtime.ipc.XMLHttpRequest.patched')
  if (globalThis.XMLHttpRequest.prototype[patched]) {
    return
  }

  globalThis.XMLHttpRequest.prototype[patched] = true

  const { send, open } = globalThis.XMLHttpRequest.prototype

  const encoder = new TextEncoder()
  Object.assign(globalThis.XMLHttpRequest.prototype, {
    open (method, url, ...args) {
      try {
        this.readyState = globalThis.XMLHttpRequest.OPENED
      } catch (_) {}
      this.method = method
      this.url = new URL(url, location.origin)
      this.seq = this.url.searchParams.get('seq')

      return open.call(this, method, url, ...args)
    },

    async send (body) {
      let { method, seq, url } = this

      if (
        url?.protocol && (
          url.protocol === 'ipc:' ||
          protocols.handlers.has(url.protocol.slice(0, -1))
        )
      ) {
        if (
          /put|post|patch/i.test(method) &&
          typeof body !== 'undefined'
        ) {
          if (typeof body === 'string') {
            body = encoder.encode(body)
          }

          if (/android/i.test(primordials.platform)) {
            if (!seq) {
              seq = 'R' + Math.random().toString().slice(2, 8) + 'X'
            }

            this.setRequestHeader('runtime-xhr-seq', seq)
            await postMessage(`ipc://buffer.map?seq=${seq}`, body)
            if (!globalThis.window && globalThis.self) {
              await new Promise((resolve) => setTimeout(resolve, 200))
            }
            body = null
          }
        }
      }

      return send.call(this, body)
    }
  })
}

function getErrorClass (type, fallback = null) {
  if (typeof globalThis !== 'undefined' && typeof globalThis[type] === 'function') {
    // eslint-disable-next-line
    return new Function(`return function ${type} () {
      const object = Object.create(globalThis['${type}']?.prototype ?? {}, {
        message: { enumerable: true, configurable: true, writable: true, value: null },
        code: { value: null }
      })

      return object
    }
    `)()
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
    // @ts-ignore
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

function getFileSystemBookmarkName (options) {
  const names = [
    options.params.get('src'),
    options.params.get('path'),
    options.params.get('value')
  ]
  return names.find(Boolean) ?? null
}

function isFileSystemBookmark (options) {
  const names = [
    options.params.get('src'),
    options.params.get('path'),
    options.params.get('value')
  ].filter(Boolean)

  if (names.some((name) => bookmarks.temporary.has(name))) {
    return true
  }

  for (const [, fd] of bookmarks.temporary.entries()) {
    if (fd === options.params.get('id') || fd === options.params.get('fd')) {
      return true
    }
  }

  return false
}

export function maybeMakeError (error, caller) {
  const errors = {
    AbortError: getErrorClass('AbortError'),
    AggregateError: getErrorClass('AggregateError'),
    EncodingError: getErrorClass('EncodingError'),
    GeolocationPositionError: getErrorClass('GeolocationPositionError'),
    IndexSizeError: getErrorClass('IndexSizeError'),
    InternalError,
    DOMException: getErrorClass('DOMContentLoaded'),
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

  if (code && ErrnoError.errno?.constants && -code in ErrnoError.errno.strings) {
    err = new ErrnoError(-code)
  } else if (type in errors) {
    err = new errors[type](error.message || '', error.code)
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
    // @ts-ignore
    typeof Error.captureStackTrace === 'function' &&
    typeof caller === 'function'
  ) {
    // @ts-ignore
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
export const kDebugEnabled = Symbol.for('socket.runtime.ipc.debug.enabled')

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

/**
 * @type {boolean}
 */
debug.enabled = false
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

if (debug.enabled && globalThis.__args?.env?.SOCKET_DEBUG_IPC) {
  // eslint-disable-next-line
  debug.log = (...args) => void globalThis.console?.log?.(...args)
} else {
  debug.log = () => undefined
}

/**
 * @ignore
 */
export class Headers extends globalThis.Headers {
  /**
   * @ignore
   */
  static from (input) {
    if (input?.headers && typeof input.headers === 'object') {
      input = input.headers
    }

    if (Array.isArray(input) && !Array.isArray(input[0])) {
      input = input.join('\n')
    } else if (typeof input?.entries === 'function') {
      // @ts-ignore
      return new this(Array.from(input.entries()))
    } else if (isPlainObject(input) || isArrayLike(input)) {
      return new this(input)
    } else if (typeof input?.getAllResponseHeaders === 'function') {
      input = input.getAllResponseHeaders()
    } else if (typeof input?.headers?.entries === 'function') {
      // @ts-ignore
      return new this(Array.from(input.headers.entries()))
    }

    // @ts-ignore
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
 * Find transfers for an in worker global `postMessage`
 * that is proxied to the main thread.
 * @ignore
 */
export function findMessageTransfers (transfers, object) {
  if (ArrayBuffer.isView(object)) {
    add(object.buffer)
  } else if (object instanceof ArrayBuffer) {
    add(object)
  } else if (Array.isArray(object)) {
    for (const value of object) {
      findMessageTransfers(transfers, value)
    }
  } else if (object && typeof object === 'object') {
    for (const key in object) {
      findMessageTransfers(transfers, object[key])
    }
  }

  return transfers

  function add (value) {
    if (!transfers.includes(value)) {
      transfers.push(value)
    }
  }
}

/**
 * @ignore
 */
export function postMessage (message, ...args) {
  if (globalThis?.webkit?.messageHandlers?.external?.postMessage) {
    // @ts-ignore
    return webkit.messageHandlers.external.postMessage(message, ...args)
  } else if (globalThis?.chrome?.webview?.postMessage) {
    // @ts-ignore
    return chrome.webview.postMessage(message, ...args)
    // @ts-ignore
  } else if (globalThis?.external?.postMessage) {
    // @ts-ignore
    return external.postMessage(message, ...args)
  } else if (globalThis.postMessage) {
    const transfer = []
    findMessageTransfers(transfer, args)
    // worker
    if (globalThis.self && !globalThis.window) {
      return globalThis?.postMessage({
        __runtime_worker_ipc_request: {
          message,
          bytes: args[0] ?? null
        }
      }, { transfer })
    } else {
      return globalThis.top.postMessage(message, ...args)
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
   * @param {(object|Uint8Array)?} [bytes]
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
   * @type {string}
   * @ignore
   */
  get command () {
    // TODO(jwerle): issue deprecation notice
    return this.name
  }

  /**
   * Computed IPC message name.
   * @type {string}
   * @ignore
   */
  get name () {
    return this.hostname || this.host || this.pathname.slice(2)
  }

  /**
   * Computed `id` value for the command.
   * @type {string}
   * @ignore
   */
  get id () {
    return this.searchParams.get('id') ?? null
  }

  /**
   * Computed `seq` (sequence) value for the command.
   * @type {string}
   * @ignore
   */
  get seq () {
    return this.has('seq') ? this.get('seq') : null
  }

  /**
   * Computed message value potentially given in message parameters.
   * This value is automatically decoded, but not treated as JSON.
   * @type {string}
   * @ignore
   */
  get value () {
    return this.get('value') ?? null
  }

  /**
   * Computed `index` value for the command potentially referring to
   * the window index the command is scoped to or originating from. If not
   * specified in the message parameters, then this value defaults to `-1`.
   * @type {number}
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
   * @type {object?}
   * @ignore
   */
  get json () {
    return parseJSON(this.value)
  }

  /**
   * Computed readonly object of message parameters.
   * @type {object}
   * @ignore
   */
  get params () {
    return Object.fromEntries(this.entries())
  }

  /**
   * Gets unparsed message parameters.
   * @type {Array<Array<string>>}
   * @ignore
   */
  get rawParams () {
    return Object.fromEntries(this.searchParams.entries())
  }

  /**
   * Returns computed parameters as entries
   * @return {Array<Array<any>>}
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
   * @param {any} value
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
   * @param {any=} [defaultValue]
   * @return {any}
   * @ignore
   */
  get (key, defaultValue = undefined) {
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
   * @return {Array<any>}
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
   * The unique ID for this result.
   * @type {string}
   * @ignore
   */
  id = String(rand64())

  /**
   * An optional error in the result.
   * @type {Error?}
   * @ignore
   */
  err = null

  /**
   * Result data if given.
   * @type {(string|object|Uint8Array)?}
   * @ignore
   */
  data = null

  /**
   * The source of this result.
   * @type {string?}
   * @ignore
   */
  source = null

  /**
   * Result headers, if given.
   * @type {Headers?}
   * @ignore
   */
  headers = new Headers()

  /**
   * Creates a `Result` instance from input that may be an object
   * like `{ err?, data? }`, an `Error` instance, or just `data`.
   * @param {(object|Error|any)?} result
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

    const id = result?.id || null
    const err = maybeMakeError(result?.err || maybeError || null, Result.from)
    const data = !err && result?.data !== null && result?.data !== undefined
      ? result.data
      : (!err && !id && !result?.source ? result?.err ?? result : null)

    const source = result?.source || maybeSource || null
    const headers = result?.headers || maybeHeaders || null

    return new this(id, err, data, source, headers)
  }

  /**
   * `Result` class constructor.
   * @private
   * @param {string?} [id = null]
   * @param {Error?} [err = null]
   * @param {object?} [data = null]
   * @param {string?} [source = null]
   * @param {(object|string|Headers)?} [headers = null]
   * @ignore
   */
  constructor (id, err, data, source, headers) {
    this.id = typeof id !== 'undefined' ? id : this.id
    this.err = typeof err !== 'undefined' ? err : this.err
    this.data = typeof data !== 'undefined' ? data : this.data
    this.source = typeof source === 'string' && source.length > 0
      ? source
      : this.source

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
      headers: this.headers ? this.headers.toJSON() : null,
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
  const startReady = Date.now()
  return await new Promise((resolve, reject) => {
    return loop()

    function loop () {
      // this can hang on android. Give it some time because emulators can be slow.
      if (Date.now() - startReady > 10000) {
        reject(new Error('Failed to resolve globalThis.__args'))
      } else if (globalThis.__args) {
        // @ts-ignore
        queueMicrotask(() => resolve())
      } else {
        queueMicrotask(loop)
      }
    }
  })
}

const { toString } = Object.prototype

export class IPCSearchParams extends URLSearchParams {
  constructor (params, nonce = null) {
    let value
    if (params !== undefined && toString.call(params) !== '[object Object]') {
      value = params
      params = null
    }

    super({
      ...params,
      index: globalThis.__args?.index ?? 0,
      seq: params?.seq ?? ('R' + nextSeq++)
    })

    if (value !== undefined) {
      this.set('value', value)
    }

    if (nonce) {
      this.set('nonce', nonce)
    }

    if (globalThis.RUNTIME_WORKER_ID) {
      this.set('runtime-worker-id', globalThis.RUNTIME_WORKER_ID)
    }

    if (globalThis.RUNTIME_WORKER_LOCATION) {
      this.set('runtime-worker-location', globalThis.RUNTIME_WORKER_LOCATION)
    }

    const runtimeFrameSource = globalThis.document
      // @ts-ignore
      ? globalThis.document.querySelector('meta[name=runtime-frame-source]')?.content
      : ''

    // @ts-ignore
    if (globalThis.top && globalThis.top !== globalThis) {
      this.set('runtime-frame-type', 'nested')
    } else if (!globalThis.window && globalThis.self === globalThis) {
      this.set('runtime-frame-type', 'worker')
      if (
        globalThis.isServiceWorkerScope ||
        (globalThis.clients && globalThis.FetchEvent) ||
        globalThis.RUNTIME_WORKER_TYPE === 'serviceWorker'
      ) {
        this.set('runtime-worker-type', 'serviceworker')
      } else {
        this.set('runtime-worker-type', 'worker')
      }
    } else {
      this.set('runtime-frame-type', 'top-level')
    }

    if (runtimeFrameSource) {
      this.set('runtime-frame-source', runtimeFrameSource)
    }
  }

  toString () {
    return super.toString().replace(/\+/g, '%20')
  }
}

/**
 * Sends a synchronous IPC command over XHR returning a `Result`
 * upon success or error.
 * @param {string} command
 * @param {any?} [value]
 * @param {object?} [options]
 * @return {Result}
 * @ignore
 */
export function sendSync (command, value = '', options = null, buffer = null) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  const params = new IPCSearchParams(value, Date.now())
  params.set('__sync__', 'true')
  const uri = `ipc://${command}?${params}`

  if (
    typeof globalThis.__global_ipc_extension_handler === 'function' &&
    (options?.useExtensionIPCIfAvailable || command.startsWith('fs.'))
  ) {
    // eslint-disable-next-line
    do {
      if (command.startsWith('fs.')) {
        if (isFileSystemBookmark({ params })) {
          break
        }
      }

      let response = null
      try {
        response = globalThis.__global_ipc_extension_handler(uri)
      } catch (err) {
        return Result.from(null, err)
      }

      if (typeof response === 'string') {
        try {
          response = JSON.parse(response)
        } catch {}
      }

      return Result.from(response, null, command)
    } while (0)
  }

  const request = new globalThis.XMLHttpRequest()

  if (debug.enabled) {
    debug.log('ipc.sendSync: %s', uri)
  }

  if (options?.responseType && typeof primordials !== 'undefined') {
    if (!(/android/i.test(primordials.platform) && globalThis.document)) {
      // @ts-ignore
      request.responseType = options.responseType
    }
  }

  if (buffer) {
    request.open('POST', uri, false)
    request.send(buffer)
  } else {
    request.open('GET', uri, false)
    request.send()
  }

  const response = getRequestResponse(request, options)
  const headers = request.getAllResponseHeaders()
  const result = Result.from(response, null, command, headers)

  if (debug.enabled) {
    debug.log('ipc.sendSync: (resolved)', command, result)
  }

  if (options?.cache === true) {
    cache[command] = result
  }

  if (command.startsWith('fs.') && isFileSystemBookmark({ params })) {
    if (!result.err) {
      const id = result.data?.id ?? result.data?.fd
      const name = getFileSystemBookmarkName({ params })
      if (id && name) {
        bookmarks.temporary.set(name, id)
      }
    }
  }

  return result
}

/**
 * Emit event to be dispatched on `window` object.
 * @param {string} name
 * @param {any} value
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
 * @param {any} value
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
 * @param {any=} value
 * @param {object=} [options]
 * @param {boolean=} [options.cache=false]
 * @param {boolean=} [options.bytes=false]
 * @return {Promise<Result>}
 */
export async function send (command, value, options = null) {
  await ready()

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  if (debug.enabled) {
    debug.log('ipc.send:', command, value)
  }

  const params = new IPCSearchParams(value)
  const uri = `ipc://${command}?${params}`

  if (
    typeof globalThis.__global_ipc_extension_handler === 'function' &&
    (options?.useExtensionIPCIfAvailable || command.startsWith('fs.'))
  ) {
    // eslint-disable-next-line
    do {
      if (command.startsWith('fs.')) {
        if (isFileSystemBookmark({ params })) {
          break
        }
      }

      let response = null
      try {
        response = await globalThis.__global_ipc_extension_handler(uri)
      } catch (err) {
        return Result.from(null, err)
      }

      if (typeof response === 'string') {
        try {
          response = JSON.parse(response)
        } catch {}
      }

      return Result.from(response, null, command)
    } while (0)
  }

  if (options?.bytes) {
    postMessage(uri, options.bytes)
  } else {
    postMessage(uri)
  }

  return await new Promise((resolve) => {
    const event = `resolve-${params.get('index')}-${params.get('seq')}`
    globalThis.addEventListener(event, onresolve, { once: true })
    function onresolve (event) {
      const result = Result.from(event.detail, null, command)
      if (debug.enabled) {
        debug.log('ipc.send: (resolved)', command, result)
      }

      if (options?.cache === true) {
        cache[command] = result
      }

      if (command.startsWith('fs.') && isFileSystemBookmark({ params })) {
        if (!result.err) {
          const id = result.data?.id ?? result.data?.fd
          const name = getFileSystemBookmarkName({ params })
          if (id && name) {
            bookmarks.temporary.set(name, id)
          }
        }
      }

      resolve(result)
    }
  })
}

/**
 * Sends an async IPC command request with parameters and buffered bytes.
 * @param {string} command
 * @param {any=} value
 * @param {(Buffer|Uint8Array|ArrayBuffer|string|Array)=} buffer
 * @param {object=} options
 * @ignore
 */
export async function write (command, value, buffer, options) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  await ready()

  const params = new IPCSearchParams(value, Date.now())
  const uri = `ipc://${command}?${params}`

  if (
    typeof globalThis.__global_ipc_extension_handler === 'function' &&
    (options?.useExtensionIPCIfAvailable || command.startsWith('fs.'))
  ) {
    // eslint-disable-next-line
    do {
      if (command.startsWith('fs.')) {
        if (isFileSystemBookmark({ params })) {
          break
        }
      }

      let response = null
      try {
        response = await globalThis.__global_ipc_extension_handler(uri, buffer)
      } catch (err) {
        return Result.from(null, err)
      }

      if (typeof response === 'string') {
        try {
          response = JSON.parse(response)
        } catch {}
      }

      return Result.from(response, null, command)
    } while (0)
  }

  const signal = options?.signal
  const request = new globalThis.XMLHttpRequest()

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

  request.responseType = options?.responseType ?? ''
  request.open('POST', uri, true)
  await request.send(buffer || null)

  if (debug.enabled) {
    debug.log('ipc.write:', uri, buffer || null)
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
 * @param {any=} value
 * @param {object=} options
 * @ignore
 */
export async function request (command, value, options) {
  if (!globalThis.XMLHttpRequest) {
    const err = new Error('XMLHttpRequest is not supported in environment')
    return Result.from(err)
  }

  if (options?.cache === true && cache[command]) {
    return cache[command]
  }

  await ready()

  const params = new IPCSearchParams(value, Date.now())
  const uri = `ipc://${command}?${params}`

  if (
    typeof globalThis.__global_ipc_extension_handler === 'function' &&
    (options?.useExtensionIPCIfAvailable || command.startsWith('fs.'))
  ) {
    // eslint-disable-next-line
    do {
      if (command.startsWith('fs.')) {
        if (isFileSystemBookmark({ params })) {
          break
        }
      }

      let response = null
      try {
        response = await globalThis.__global_ipc_extension_handler(uri)
      } catch (err) {
        return Result.from(null, err)
      }

      if (typeof response === 'string') {
        try {
          response = JSON.parse(response)
        } catch {}
      }

      return Result.from(response, null, command)
    } while (0)
  }

  const signal = options?.signal
  const request = new globalThis.XMLHttpRequest()

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

  request.responseType = options?.responseType ?? ''
  request.open('GET', uri)
  request.send(null)

  if (debug.enabled) {
    debug.log('ipc.request:', uri)
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

        if (command.startsWith('fs.') && isFileSystemBookmark({ params })) {
          if (!result.err) {
            const id = result.data?.id ?? result.data?.fd
            const name = getFileSystemBookmarkName({ params })
            if (id && name) {
              bookmarks.temporary.set(name, id)
            }
          }
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

  ctx.chain = new Set()
  ctx.chain.add(domain)

  const proxy = new Proxy(ctx, {
    apply (target, _, args) {
      const chain = Array.from(ctx.chain)
      const path = chain.filter(Boolean).filter((v) => typeof v === 'string').join('.')
      const method = (ctx[path]?.method || ctx[path]) || ctx.default || 'send'
      ctx.chain = new Set()
      ctx.chain.add(domain)
      return dispatchable[method](path, ...args)
    },

    get (target, key, ...args) {
      const value = Reflect.get(target, key, ...args)
      if (value !== undefined) return value
      if (
        key === 'inspect' ||
        key === '__proto__' ||
        key === 'constructor' ||
        key in Promise.prototype ||
        key in Function.prototype
      ) {
        return
      }

      ctx.chain.add(key)
      return new Proxy(ctx, this)
    }
  })

  return proxy
}

// TODO(@chicoxyzzy): generate the primordials file during the build
// We need to set primordials here because we are using the
// `sendSync` method. This is a hack to get around the fact
// that we can't use cyclic imports with a sync call.
/**
 * @ignore
 */
export const primordials = sendSync('platform.primordials')?.data || {}

// remove trailing slash on windows
if (primordials.cwd) {
  primordials.cwd = primordials.cwd.replace(/\\$/, '')
}

if (
  globalThis.__RUNTIME_PRIMORDIAL_OVERRIDES__ &&
  typeof globalThis.__RUNTIME_PRIMORDIAL_OVERRIDES__ === 'object'
) {
  Object.assign(primordials, globalThis.__RUNTIME_PRIMORDIAL_OVERRIDES__)
}

Object.freeze(primordials)

initializeXHRIntercept()

if (typeof globalThis?.window !== 'undefined') {
  if (globalThis.document.readyState === 'complete') {
    queueMicrotask(async () => {
      try {
        await send('platform.event', 'domcontentloaded')
      } catch (err) {
        reportError(err)
      }
    })
  } else {
    globalThis.document.addEventListener('DOMContentLoaded', () => {
      queueMicrotask(async () => {
        try {
          await send('platform.event', 'domcontentloaded')
        } catch (err) {
          reportError(err)
        }
      })
    })
  }
}

export function inflateIPCMessageTransfers (object, types = new Map()) {
  if (ArrayBuffer.isView(object)) {
    return object
  } else if (Array.isArray(object)) {
    for (let i = 0; i < object.length; ++i) {
      object[i] = inflateIPCMessageTransfers(object[i], types)
    }
  } else if (object && typeof object === 'object') {
    if ('__type__' in object && types.has(object.__type__)) {
      const Type = types.get(object.__type__)
      if (typeof Type === 'function') {
        if (typeof Type.from === 'function') {
          return Type.from(object)
        } else {
          return new Type(object)
        }
      }
    }

    if (object.__type__ === 'IPCMessagePort' && object.id) {
      return IPCMessagePort.create(object)
    } else {
      for (const key in object) {
        const value = object[key]
        object[key] = inflateIPCMessageTransfers(value, types)
      }
    }
  }

  return object
}

export function findIPCMessageTransfers (transfers, object) {
  if (ArrayBuffer.isView(object)) {
    add(object.buffer)
  } else if (object instanceof ArrayBuffer) {
    add(object)
  } else if (Array.isArray(object)) {
    for (let i = 0; i < object.length; ++i) {
      object[i] = findIPCMessageTransfers(transfers, object[i])
    }
  } else if (object && typeof object === 'object') {
    if (
      object instanceof MessagePort || (
        typeof object.postMessage === 'function' &&
        Object.getPrototypeOf(object).constructor.name === 'MessagePort'
      )
    ) {
      const port = IPCMessagePort.create(object)
      object.addEventListener('message', function onMessage (event) {
        if (port.closed === true) {
          port.onmessage = null
          event.preventDefault()
          event.stopImmediatePropagation()
          object.removeEventListener('message', onMessage)
          return false
        }

        port.dispatchEvent(new MessageEvent('message', event))
      })

      port.onmessage = (event) => {
        if (port.closed === true) {
          port.onmessage = null
          event.preventDefault()
          event.stopImmediatePropagation()
          return false
        }

        const transfers = new Set()
        findIPCMessageTransfers(transfers, event.data)
        object.postMessage(event.data, {
          transfer: Array.from(transfers)
        })
      }
      add(port)
      return port
    } else {
      for (const key in object) {
        object[key] = findIPCMessageTransfers(transfers, object[key])
      }
    }
  }

  return object

  function add (value) {
    if (
      value &&
      !transfers.has(value) &&
      !(Symbol.for('socket.runtime.serialize') in value)
    ) {
      transfers.add(value)
    }
  }
}

export const ports = new Map()

export class IPCMessagePort extends MessagePort {
  static from (options = null) {
    return this.create(options)
  }

  static create (options = null) {
    const id = String(options?.id ?? rand64())
    const port = Object.create(this.prototype)
    const token = String(rand64())
    const channel = typeof options?.channel === 'string'
      ? new BroadcastChannel(options.channel)
      : (
          (options?.channel && new BroadcastChannel(options.channel.name)) ??
          new BroadcastChannel(id)
        )

    if (ports.has(id)) {
      ports.get(id).closed = true
      ports.get(id).channel.onmessage = null
      ports.get(id).onmessage = null
      ports.get(id).onmessageerror = null
    }

    port[Symbol.for('socket.runtime.IPCMessagePort.id')] = id
    ports.set(id, Object.create(null, {
      id: { writable: true, value: id },
      token: { writable: false, value: token },
      closed: { writable: true, value: false },
      started: { writable: true, value: false },
      channel: { writable: true, value: channel },
      onmessage: { writable: true, value: null },
      onmessageerror: { writable: true, value: null },
      eventTarget: { writable: true, value: ports.get(id)?.eventTarget || new EventTarget() }
    }))

    const state = ports.get(id)
    channel.onmessage = function onMessage (event) {
      if (!state || state?.closed === true) {
        event.preventDefault()
        event.stopImmediatePropagation()
        channel.onmessage = null
        return false
      }

      if (state?.started && event.data?.token !== state.token) {
        port.dispatchEvent(new MessageEvent('message', {
          ...event,
          data: event.data?.data
        }))
      }
    }

    gc.ref(port)
    return port
  }

  get id () {
    return this[Symbol.for('socket.runtime.IPCMessagePort.id')] ?? null
  }

  get started () {
    if (!ports.has(this.id)) {
      return false
    }

    return ports.get(this.id)?.started ?? false
  }

  get closed () {
    if (!ports.has(this.id)) {
      return true
    }

    return ports.get(this.id)?.closed ?? false
  }

  get onmessage () {
    return ports.get(this.id)?.onmessage ?? null
  }

  set onmessage (onmessage) {
    const port = ports.get(this.id)

    if (!port) {
      return
    }

    if (typeof port.onmessage === 'function') {
      this.removeEventListener('message', port.onmessage)
      port.onmessage = null
    }

    if (typeof onmessage === 'function') {
      this.addEventListener('message', onmessage)
      port.onmessage = onmessage
      if (!port.started) {
        this.start()
      }
    }
  }

  get onmessageerror () {
    return ports.get(this.id)?.onmessageerror ?? null
  }

  set onmessageerror (onmessageerror) {
    const port = ports.get(this.id)

    if (!port) {
      return
    }

    if (typeof this.onmessageerror === 'function') {
      this.removeEventListener('messageerror', port.onmessageerror)
      port.onmessageerror = null
    }

    if (typeof onmessageerror === 'function') {
      this.addEventListener('messageerror', onmessageerror)
      port.onmessageerror = onmessageerror
    }
  }

  start () {
    const port = ports.get(this.id)
    if (port) {
      port.started = true
    }
  }

  close (purge = true) {
    const port = ports.get(this.id)
    if (port) {
      port.closed = true
    }

    if (purge) {
      ports.delete(this.id)
    }
  }

  postMessage (message, optionsOrTransferList) {
    const port = ports.get(this.id)
    const options = { transfer: [] }

    if (!port) {
      return
    }

    if (Array.isArray(optionsOrTransferList)) {
      options.transfer.push(...optionsOrTransferList)
    } else if (Array.isArray(optionsOrTransferList?.transfer)) {
      options.transfer.push(...optionsOrTransferList.transfer)
    }

    const transfers = new Set(options.transfer)
    const handle = this[Symbol.for('socket.runtime.ipc.MessagePort.handlePostMessage')]

    const serializedMessage = serialize(findIPCMessageTransfers(transfers, message))
    options.transfer = Array.from(transfers)

    if (typeof handle === 'function') {
      if (handle.call(this, serializedMessage, options) === false) {
        return
      }
    }

    port.channel.postMessage({
      token: port.token,
      data: serializedMessage
    }, options)
  }

  addEventListener (...args) {
    const eventTarget = ports.get(this.id)?.eventTarget

    if (eventTarget) {
      return eventTarget.addEventListener(...args)
    }

    return false
  }

  removeEventListener (...args) {
    const eventTarget = ports.get(this.id)?.eventTarget

    if (eventTarget) {
      return eventTarget.removeEventListener(...args)
    }

    return false
  }

  dispatchEvent (event) {
    const eventTarget = ports.get(this.id)?.eventTarget

    if (eventTarget) {
      if (event.type === 'message') {
        return eventTarget.dispatchEvent(new MessageEvent('message', {
          ...event,
          data: inflateIPCMessageTransfers(event.data)
        }))
      } else {
        return eventTarget.dispatchEvent(event)
      }
    }

    return false
  }

  [Symbol.for('socket.runtime.ipc.MessagePort.handlePostMessage')] (
    message,
    options = null
  ) {
    return true
  }

  [Symbol.for('socket.runtime.serialize')] () {
    return {
      __type__: 'IPCMessagePort',
      channel: ports.get(this.id)?.channel?.name ?? null,
      id: this.id
    }
  }

  [Symbol.for('socket.runtime.gc.finalizer')] () {
    return {
      args: [this.id],
      handle (id) {
        if (!ports.get(id)?.channel?.onmessage) {
          ports.delete(id)
        }
      }
    }
  }
}

export class IPCMessageChannel extends MessageChannel {
  #id = null
  #port1 = null
  #port2 = null
  #channel = null

  constructor (options = null) {
    super()
    this.#id = String(options?.id ?? rand64())
    this.#channel = options?.channel ?? new BroadcastChannel(this.#id)

    this.#port1 = IPCMessagePort.create(options?.port1)
    this.#port2 = IPCMessagePort.create(options?.port2)

    this.port1[Symbol.for('socket.runtime.ipc.MessagePort.handlePostMessage')] = (message, options) => {
      this.port2.channel.postMessage(message, options)
      return false
    }

    this.port2[Symbol.for('socket.runtime.ipc.MessagePort.handlePostMessage')] = (message, options) => {
      this.port2.channel.postMessage(message, options)
      return false
    }
  }

  get id () {
    return this.#id
  }

  get port1 () {
    return this.#port1
  }

  get port2 () {
    return this.#port2
  }

  get channel () {
    return this.#channel
  }
}

// eslint-disable-next-line
import * as exports from './ipc.js'
export default exports
