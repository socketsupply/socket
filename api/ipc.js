/**
 * @module IPC
 *
 * This is a low level API which you don't need unless you are implementing
 * a library on top of Socket SDK. A Socket SDK app has two or three processes.
 *
 * - The `Render` process, the UI where the HTML, CSS and JS is run.
 * - The `Bridge` process, the thin layer of code that managers everything.
 * - The `Main` processs, for apps that need to run heavier compute jobs. And
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
 */

/* global window */
import {
  AbortError,
  InternalError,
  TimeoutError
} from './errors.js'

import {
  isBufferLike,
  isPlainObject,
  format,
  parseJSON
} from './util.js'

import * as errors from './errors.js'
import { Buffer } from './buffer.js'
import console from './console.js'

let nextSeq = 1

export async function postMessage (...args) {
  return await window?.__ipc?.postMessage(...args)
}

function initializeXHRIntercept () {
  if (typeof window === 'undefined') return
  const { send, open } = window.XMLHttpRequest.prototype

  const B5_PREFIX_BUFFER = new Uint8Array([0x62, 0x35]) // literally, 'b5'
  const encoder = new TextEncoder()
  Object.assign(window.XMLHttpRequest.prototype, {
    open (method, url, ...args) {
      try {
        this.readyState = window.XMLHttpRequest.OPENED
      } catch (_) {}
      this.method = method
      this.url = new URL(url)
      this.seq = this.url.searchParams.get('seq')

      return open.call(this, method, url, ...args)
    },

    async send (body) {
      const { method, seq, url } = this
      const index = window.__args.index

      if (url?.protocol === 'ipc:') {
        if (
          /put|post/i.test(method) &&
          typeof body !== 'undefined' &&
          typeof seq !== 'undefined'
        ) {
          if (/android/i.test(window.__args.os)) {
            await postMessage(`ipc://buffer.map?seq=${seq}`, body)
            body = null
          }

          if (/linux/i.test(window.__args.os)) {
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

if (typeof window !== 'undefined') {
  initializeXHRIntercept()

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

function getErrorClass (type, fallback) {
  if (typeof window !== 'undefined' && typeof window[type] === 'function') {
    return window[type]
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

function getRequestResponse (request) {
  if (!request) return null
  const { responseType } = request
  let response = null

  if (!responseType || responseType === 'text') {
    // `responseText` could be an accessor which could throw an
    // `InvalidStateError` error when accessed when `responseType` is anything
    // but empty or `'text'`
    // @see {@link https://developer.mozilla.org/en-US/docs/Web/API/XMLHttpRequest/responseText#exceptions}
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
      response = Buffer.from(request.response)
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
    const message = Message.from(responseURL?.replace('http:', Message.PROTOCOL))
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

function createUri (protocol, command) {
  if (typeof window === 'object' && window?.__args.os === 'win32') {
    protocol = 'http:'
  }

  return `${protocol}//${command}`
}

/**
 * Represents an OK IPC status.
 */
export const OK = 0

/**
 * Represents an ERROR IPC status.
 */
export const ERROR = 1

/**
 * Timeout in milliseconds for IPC requests.
 */
export const TIMEOUT = 32 * 1000

/**
 * Symbol for the `ipc.debug.enabled` property
 */
export const kDebugEnabled = Symbol.for('ipc.debug.enabled')

/**
 * Parses `seq` as integer value
 * @param {string|number} seq
 * @param {object=} [options]
 * @param {boolean} [options.bigint = false]
 */
export function parseSeq (seq, options) {
  const value = String(seq).replace(/^R/i, '').replace(/n$/, '')
  return options?.bigint === true ? BigInt(value) : parseInt(value)
}

/**
 * If `debug.enabled === true`, then debug output will be printed to console.
 * @param {(boolean)} [enable]
 * @return {boolean}
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
      return typeof window === 'undefined'
        ? false
        : Boolean(window?.__args?.debug)
    }

    return debug[kDebugEnabled]
  }
})

/**
 * A container for a IPC message based on a `ipc://` URI scheme.
 */
export class Message extends URL {
  /**
   * The expected protocol for an IPC message.
   */
  static get PROTOCOL () {
    return 'ipc:'
  }

  /**
   * Creates a `Message` instance from a variety of input.
   * @param {string|URL|Message|Buffer|object} input
   * @param {(object|string|URLSearchParams)=} [params]
   * @return {Message}
   */
  static from (input, params, bytes) {
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
   * `Message` class constructor.
   * @protected
   * @param {string|URL} input
   */
  constructor (input, bytes) {
    super(input)
    if (this.protocol !== this.constructor.PROTOCOL) {
      throw new TypeError(format(
        'Invalid protocol in input. Expected \'%s\' but got \'%s\'',
        this.constructor.PROTOCOL, this.protocol
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
   */
  get command () {
    // TODO(jwerle): issue deprecation notice
    return this.name
  }

  /**
   * Computed IPC message name.
   */
  get name () {
    return this.hostname || this.host || this.pathname.slice(2)
  }

  /**
   * Computed `id` value for the command.
   */
  get id () {
    return this.has('id') ? this.get('id') : null
  }

  /**
   * Computed `seq` (sequence) value for the command.
   */
  get seq () {
    return this.has('seq') ? this.get('seq') : null
  }

  /**
   * Computed message value potentially given in message parameters.
   * This value is automatically decoded, but not treated as JSON.
   */
  get value () {
    return this.get('value') ?? null
  }

  /**
   * Computed `index` value for the command potentially referring to
   * the window index the command is scoped to or originating from. If not
   * specified in the message parameters, then this value defaults to `-1`.
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
   */
  get json () {
    return parseJSON(this.value)
  }

  /**
   * Computed readonly object of message parameters.
   */
  get params () {
    return Object.fromEntries(this.entries())
  }

  /**
   * Returns computed parameters as entries
   * @return {Array<Array<string,mixed>>}
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
   */
  keys () {
    return Array.from(this.searchParams.keys())
  }

  /**
   * Computed parameter values.
   * @return {Array<mixed>}
   */
  values () {
    return Array.from(this.searchParams.values()).map(parseJSON)
  }

  /**
   * Predicate to determine if parameter `key` is present in parameters.
   * @param {string} key
   * @return {boolean}
   */
  has (key) {
    return this.searchParams.has(key)
  }

  /**
   * Converts a `Message` instance into a plain JSON object.
   */
  toJSON () {
    const { protocol, command, params } = this
    return { protocol, command, params }
  }
}

/**
 * A result type used internally for handling
 * IPC result values from the native layer that are in the form
 * of `{ err?, data? }`. The `data` and `err` properties on this
 * type of object are in tuple form and be accessed at `[data?,err?]`
 */
export class Result {
  /**
   * Creates a `Result` instance from input that may be an object
   * like `{ err?, data? }`, an `Error` instance, or just `data`.
   * @param {(object|Error|mixed)=} result
   * @param {Error=} [maybeError]
   * @param {string=} [maybeSource]
   * @return {Result}
   */
  static from (result, maybeError, maybeSource, ...args) {
    if (result instanceof Result) {
      if (!result.source && maybeSource) {
        result.source = maybeSource
      }

      if (!result.err && maybeError) {
        result.err = maybeError
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

    return new this(err, data, source, ...args)
  }

  /**
   * `Result` class constructor.
   * @private
   * @param {Error=} [err = null]
   * @param {object=} [data = null]
   * @param {string=} [source = undefined]
   */
  constructor (err, data, source) {
    this.err = typeof err !== 'undefined' ? err : null
    this.data = typeof data !== 'undefined' ? data : null
    this.source = typeof source === 'string' && source.length
      ? source
      : undefined

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
  }

  /**
   * Computed result length.
   */
  get length () {
    return [...this].filter((v) => v !== undefined).length
  }

  /**
   * Generator for an `Iterable` interface over this instance.
   * @ignore
   */
  * [Symbol.iterator] () {
    yield this.err
    yield this.data
    yield this.source
  }
}

/**
 * Waits for the native IPC layer to be ready and exposed on the
 * global window object.
 */
export async function ready () {
  return await new Promise((resolve, reject) => {
    if (typeof window === 'undefined') {
      return reject(new TypeError('Global window object is not defined.'))
    }

    return loop()

    function loop () {
      if (window.__args) {
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
 * @param {(object|string)=} params
 * @return {Result}
 */
export function sendSync (command, params) {
  if (typeof window === 'undefined') {
    if (debug.enabled) {
      debug.log('Global window object is not defined')
    }

    return {}
  }

  const protocol = Message.PROTOCOL
  const request = new window.XMLHttpRequest()
  const index = window.__args.index ?? 0
  const seq = nextSeq++
  const uri = createUri(protocol, command)

  params = new URLSearchParams(params)
  params.set('index', index)
  params.set('seq', 'R' + seq)

  const query = `?${params}`

  if (debug.enabled) {
    debug.log('ipc.sendSync: %s', uri + query)
  }

  request.open('GET', uri + query, false)
  request.setRequestHeader('x-ipc-request', command)
  request.send()

  const result = Result.from(getRequestResponse(request), null, command)

  if (debug.enabled) {
    debug.log('ipc.sendSync: (resolved)', command, result)
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

  const event = new window.CustomEvent(name, { detail, ...options })
  if (target) {
    target.dispatchEvent(event)
  } else {
    window.dispatchEvent(event)
  }
}

/**
 * Resolves a request by `seq` with possible value.
 * @param {string} seq
 * @param {Mixed} value
 */
export async function resolve (seq, value) {
  await ready()

  if (debug.enabled) {
    debug.log('ipc.resolve:', seq, value)
  }

  const index = window.__args.index
  const eventName = `resolve-${index}-${seq}`
  const event = new window.CustomEvent(eventName, { detail: value })
  window.dispatchEvent(event)
}

/**
 * Sends an async IPC command request with parameters.
 * @param {string} command
 * @param {Mixed=} value
 * @return {Promise<Result>}
 */
export async function send (command, value) {
  await ready()

  if (debug.enabled) {
    debug.log('ipc.send:', command, value)
  }

  const seq = 'R' + nextSeq++
  const index = value?.index ?? window.__args.index
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

  postMessage(`ipc://${command}?${serialized}`)

  return await new Promise((resolve) => {
    const event = `resolve-${index}-${seq}`
    window.addEventListener(event, onresolve, { once: true })
    function onresolve (event) {
      const result = Result.from(event.detail, null, command)
      if (debug.enabled) {
        debug.log('ipc.send: (resolved)', command, result)
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
 */
export async function write (command, params, buffer, options) {
  if (typeof window === 'undefined') {
    debug.log('Global window object is not defined')
    return {}
  }

  await ready()

  const protocol = Message.PROTOCOL
  const request = new window.XMLHttpRequest()
  const signal = options?.signal
  const index = window.__args.index ?? 0
  const seq = nextSeq++
  const uri = createUri(protocol, command)

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

  const query = `?${params}`

  request.open('POST', uri + query, true)
  request.setRequestHeader('x-ipc-request', command)
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

      if (request.readyState === window.XMLHttpRequest.DONE) {
        resolved = true
        clearTimeout(timeout)

        const result = Result.from(getRequestResponse(request), null, command)

        if (debug.enabled) {
          debug.log('ipc.write: (resolved)', command, result)
        }

        return resolve(result)
      }
    }

    request.onerror = () => {
      const err = new Error(getRequestResponseText(request) || '')
      resolved = true
      clearTimeout(timeout)
      resolve(Result.from(null, err, command))
    }
  })
}

/**
 * Sends an async IPC command request with parameters requesting a response
 * with buffered bytes.
 * @param {string} command
 * @param {object=} params
 * @param {object=} options
 */
export async function request (command, params, options) {
  await ready()

  const protocol = Message.PROTOCOL
  const request = new window.XMLHttpRequest()
  const signal = options?.signal
  const index = window.__args.index ?? 0
  const seq = nextSeq++
  const uri = createUri(protocol, command)

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

  const query = `?${params}`

  request.responseType = options?.responseType ?? ''
  request.open('GET', uri + query)
  request.setRequestHeader('x-ipc-request', command)
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

      if (request.readyState === window.XMLHttpRequest.DONE) {
        resolved = true
        clearTimeout(timeout)

        const result = Result.from(getRequestResponse(request), null, command)

        if (debug.enabled) {
          debug.log('ipc.request: (resolved)', command, result)
        }

        return resolve(result)
      }
    }

    request.onerror = () => {
      const err = new Error(getRequestResponseText(request))
      resolved = true
      clearTimeout(timeout)
      resolve(Result.from(null, err, command))
    }
  })
}

/**
 * Factory for creating a proxy based IPC API.
 * @param {string} domain
 * @param {(function|object)=} ctx
 * @param {string=} [ctx.default]
 * @return {Proxy}
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

export default {
  OK,
  ERROR,
  TIMEOUT,

  createBinding,
  debug,
  emit,
  Message,
  postMessage,
  ready,
  resolve,
  request,
  send,
  sendSync,
  write
}
