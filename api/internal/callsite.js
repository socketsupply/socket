import InternalSymbols from './symbols.js'
import { createHook } from '../async/hooks.js'
import { murmur3 } from '../crypto.js'
import { Buffer } from '../buffer.js'
import path from '../path.js'

let isAsyncContext = false
const asyncContexts = new Set([
  'Promise',
  'Timeout',
  'Interval',
  'Immediate',
  'Microtask'
])

const hook = createHook({
  before (asyncId, type) {
    if (asyncContexts.has(type)) {
      isAsyncContext = true
    }
  },

  after (asyncId, type) {
    if (asyncContexts.has(type)) {
      isAsyncContext = false
    }
  }
})

hook.enable()

/**
 * @typedef {{
 *   sourceURL: string | null,
 *   symbol: string,
 *   column: number | undefined,
 *   line: number | undefined,
 *   native: boolean
 * }} ParsedStackFrame
 */

/**
 * A container for location data related to a `StackFrame`
 */
export class StackFrameLocation {
  /**
   * Creates a `StackFrameLocation` from JSON input.
   * @param {object=} json
   * @return {StackFrameLocation}
   */
  static from (json) {
    const location = new this()

    if (Number.isFinite(json?.lineNumber)) {
      location.lineNumber = json.lineNumber
    }

    if (Number.isFinite(json?.columnNumber)) {
      location.columnNumber = json.columnNumber
    }

    if (json?.sourceURL && URL.canParse(json.sourceURL)) {
      location.sourceURL = new URL(json.sourceURL).href
    }

    if (json?.isNative === true) {
      location.isNative = true
    }

    return location
  }

  /**
   * The line number of the location of the stack frame, if available.
   * @type {number | undefined}
   */
  lineNumber

  /**
   * The column number of the location of the stack frame, if available.
   * @type {number | undefined}
   */
  columnNumber

  /**
   * The source URL of the location of the stack frame, if available. This value
   * may be `null`.
   * @type {string?}
   */
  sourceURL = null

  /**
   * `true` if the stack frame location is in native location, otherwise
   * this value `false` (default).
   * @type
   */
  isNative = false

  /**
   * Converts this `StackFrameLocation` to a JSON object.
   * @ignore
   * @return {{
   *   lineNumber: number | undefined,
   *   columnNumber: number | undefined,
   *   sourceURL: string | null,
   *   isNative: boolean
   * }}
   */
  toJSON () {
    return {
      lineNumber: this.lineNumber,
      columnNumber: this.columnNumber,
      sourceURL: this.sourceURL,
      isNative: this.isNative
    }
  }

  /**
   * Serializes this `StackFrameLocation`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'StackFrameLocation',
   *   lineNumber: number | undefined,
   *   columnNumber: number | undefined,
   *   sourceURL: string | null,
   *   isNative: boolean
   * }}
   */
  [InternalSymbols.serialize] () {
    return { __type__: 'StackFrameLocation', ...this.toJSON() }
  }
}

/**
 * A stack frame container related to a `CallSite`.
 */
export class StackFrame {
  /**
   * Parses a raw stack frame string into structured data.
   * @param {string} rawStackFrame
   * @return {ParsedStackFrame}
   */
  static parse (rawStackFrame) {
    const parsed = {
      sourceURL: null,
      symbol: '<anonymous>',
      column: undefined,
      line: undefined
    }

    const parts = rawStackFrame.split('@')
    const symbol = parts.shift()
    const location = parts.shift()

    if (symbol) {
      parsed.symbol = symbol
    }

    if (location === '[native code]') {
      parsed.native = true
    } else if (location && URL.canParse(location)) {
      const url = new URL(location)
      const [pathname, lineno, columnno] = url.pathname.split(':')
      const line = parseInt(lineno)
      const column = parseInt(columnno)

      if (Number.isFinite(line)) {
        parsed.line = line
      }

      if (Number.isFinite(column)) {
        parsed.column = column
      }

      parsed.sourceURL = new URL(pathname + url.search, url.origin).href
    }

    return parsed
  }

  /**
   * Creates a new `StackFrame` from an `Error` and raw stack frame
   * source `rawStackFrame`.
   * @param {Error} error
   * @param {string} rawStackFrame
   * @return {StackFrame}
   */
  static from (error, rawStackFrame) {
    const parsed = this.parse(rawStackFrame)
    return new this(error, parsed, rawStackFrame)
  }

  /**
   * The stack frame location data.
   * @type {StackFrameLocation}
   */
  location = new StackFrameLocation()

  /**
   * The `Error` associated with this `StackFrame` instance.
   * @type {Error?}
   */
  error = null

  /**
   * The name of the function where the stack frame is located.
   * @type {string?}
   */
  symbol = null

  /**
   * The raw stack frame source string.
   * @type {string?}
   */
  source = null

  /**
   * `StackFrame` class constructor.
   * @param {Error} error
   * @param {ParsedStackFrame=} [frame]
   * @param {string=} [source]
   */
  constructor (error, frame = null, source = null) {
    if (error instanceof Error) {
      this.error = error
    }

    if (typeof source === 'string') {
      this.source = source
    }

    if (Number.isFinite(frame?.line)) {
      this.location.lineNumber = frame.line
    }

    if (Number.isFinite(frame?.column)) {
      this.location.columnNumber = frame.column
    }

    if (typeof frame?.sourceURL === 'string' && URL.canParse(frame.sourceURL)) {
      this.location.sourceURL = frame.sourceURL
    }

    if (typeof frame?.symbol === 'string') {
      this.symbol = frame.symbol
    }

    if (frame?.native === true) {
      this.location.isNative = true
    }
  }

  /**
   * Converts this `StackFrameLocation` to a JSON object.
   * @ignore
   * @return {{
   *   location: {
   *     lineNumber: number | undefined,
   *     columnNumber: number | undefined,
   *     sourceURL: string | null,
   *     isNative: boolean
   *   },
   *   isNative: boolean,
   *   symbol: string | null,
   *   source: string | null,
   *   error: { message: string, name: string, stack: string } | null
   * }}
   */
  toJSON () {
    return {
      location: this.location.toJSON(),
      isNative: this.isNative,
      symbol: this.symbol,
      source: this.source,
      error: this.error === null
        ? null
        : {
            message: this.error.message ?? '',
            name: this.error.name ?? '',
            stack: String(this.error[CallSite.StackSourceSymbol] ?? this.error.stack ?? '')
          }
    }
  }

  /**
   * Serializes this `StackFrame`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'StackFrame',
   *   location: {
   *     __type__: 'StackFrameLocation',
   *     lineNumber: number | undefined,
   *     columnNumber: number | undefined,
   *     sourceURL: string | null,
   *     isNative: boolean
   *   },
   *   isNative: boolean,
   *   symbol: string | null,
   *   source: string | null,
   *   error: { message: string, name: string, stack: string } | null
   * }}
   */
  [InternalSymbols.serialize] () {
    return {
      __type__: 'StackFrame',
      ...this.toJSON(),
      location: this.location[InternalSymbols.serialize]()
    }
  }
}

// private symbol for `CallSiteList` previous reference
const $previous = Symbol('previous')

/**
 * A v8 compatible interface and container for call site information.
 */
export class CallSite {
  /**
   * An internal symbol used to refer to the index of a promise in
   * `Promise.all` or `Promise.any` function call site.
   * @ignore
   * @type {symbol}
   */
  static PromiseElementIndexSymbol = Symbol.for('socket.runtime.CallSite.PromiseElementIndex')

  /**
   * An internal symbol used to indicate that a call site is in a `Promise.all`
   * function call.
   * @ignore
   * @type {symbol}
   */
  static PromiseAllSymbol = Symbol.for('socket.runtime.CallSite.PromiseAll')

  /**
   * An internal symbol used to indicate that a call site is in a `Promise.any`
   * function call.
   * @ignore
   * @type {symbol}
   */
  static PromiseAnySymbol = Symbol.for('socket.runtime.CallSite.PromiseAny')

  /**
   * An internal source symbol used to store the original `Error` stack source.
   * @ignore
   * @type {symbol}
   */
  static StackSourceSymbol = Symbol.for('socket.runtime.CallSite.StackSource')

  #error = null
  #frame = null
  #previous = null

  /**
   * `CallSite` class constructor
   * @param {Error} error
   * @param {string} rawStackFrame
   * @param {CallSite=} previous
   */
  constructor (error, rawStackFrame, previous = null) {
    this.#error = error
    this.#frame = StackFrame.from(error, rawStackFrame)
    if (previous !== null && previous instanceof CallSite) {
      this.#previous = previous
    }
  }

  /**
   * Private accessor to "friend class" `CallSiteList`.
   * @ignore
   */
  get [$previous] () { return this.#previous }
  set [$previous] (previous) {
    if (previous === null || previous instanceof CallSite) {
      this.#previous = previous
    }
  }

  /**
   * The `Error` associated with the call site.
   * @type {Error}
   */
  get error () {
    return this.#error
  }

  /**
   * The previous `CallSite` instance, if available.
   * @type {CallSite?}
   */
  get previous () {
    return this.#previous
  }

  /**
   * A reference to the `StackFrame` data.
   * @type {StackFrame}
   */
  get frame () {
    return this.#frame
  }

  /**
   * This function _ALWAYS__ returns `globalThis` as `this` cannot be determined.
   * @return {object}
   */
  getThis () {
    // not supported
    return globalThis
  }

  /**
   * This function _ALWAYS__ returns `null` as the type name of `this`
   * cannot be determined.
   * @return {null}
   */
  getTypeName () {
    // not supported
    return null
  }

  /**
   * This function _ALWAYS__ returns `undefined` as the current function
   * reference cannot be determined.
   * @return {undefined}
   */
  getFunction () {
    // not supported
    return undefined
  }

  /**
   * Returns the name of the function in at the call site, if available.
   * @return {string|undefined}
   */
  getFunctionName () {
    const symbol = this.#frame.symbol

    if (symbol === 'global code' || symbol === 'module code' || symbol === 'eval code') {
      return undefined
    }

    return symbol
  }

  /**
   * An alias to `getFunctionName()
   * @return {string}
   */
  getMethodName () {
    return this.getFunctionName()
  }

  /**
   * Get the filename of the call site location, if available, otherwise this
   * function returns 'unknown location'.
   * @return {string}
   */
  getFileName () {
    if (this.#frame.location.sourceURL) {
      const root = new URL('../../', import.meta.url || globalThis.location.href).pathname

      let filename = new URL(this.#frame.location.sourceURL).pathname.replace(root, '')

      if (/\/socket\//.test(filename)) {
        filename = filename.replace('socket/', 'socket:').replace(/.js$/, '')
        return filename
      }

      return path.basename(new URL(this.#frame.location.sourceURL).pathname)
    }

    return 'unknown location'
  }

  /**
   * Returns the location source URL defaulting to the global location.
   * @return {string}
   */
  getScriptNameOrSourceURL () {
    const url = new URL(this.#frame.location.sourceURL ?? globalThis.location.href)
    let filename = url.pathname.replace(url.pathname, '')

    if (/\/socket\//.test(filename)) {
      filename = filename.replace('socket/', 'socket:').replace(/.js$/, '')
      return filename
    }

    return url.href
  }

  /**
   * Returns a hash value of the source URL return by `getScriptNameOrSourceURL()`
   * @return {string}
   */
  getScriptHash () {
    return Buffer.from(String(murmur3(this.getScriptNameOrSourceURL()))).toString('hex')
  }

  /**
   * Returns the line number of the call site location.
   * This value may be `undefined`.
   * @return {number|undefined}
   */
  getLineNumber () {
    return this.#frame.lineNumber
  }

  /**
   * @ignore
   * @return {number}
   */
  getPosition () {
    return 0
  }

  /**
   * Attempts to get an "enclosing" line number, potentially the previous
   * line number of the call site
   * @param {number|undefined}
   */
  getEnclosingLineNumber () {
    if (this.#previous) {
      const previousSourceURL = this.#previous.getScriptNameOrSourceURL()
      if (previousSourceURL && previousSourceURL === this.getScriptNameOrSourceURL()) {
        return this.#previous.getLineNumber()
      }
    }
  }

  /**
   * Returns the column number of the call site location.
   * This value may be `undefined`.
   * @return {number|undefined}
   */
  getColumnNumber () {
    return this.#frame.columnNumber
  }

  /**
   * Attempts to get an "enclosing" column number, potentially the previous
   * line number of the call site
   * @param {number|undefined}
   */
  getEnclosingColumnNumber () {
    if (this.#previous) {
      const previousSourceURL = this.#previous.getScriptNameOrSourceURL()
      if (previousSourceURL && previousSourceURL === this.getScriptNameOrSourceURL()) {
        return this.#previous.getColumnNumber()
      }
    }
  }

  /**
   * Gets the origin of where `eval()` was called if this call site function
   * originated from a call to `eval()`. This function may return `undefined`.
   * @return {string|undefined}
   */
  getEvalOrigin () {
    let current = this

    while (current) {
      if (current.frame.symbol === 'eval' && current.frame.location.isNative) {
        const previous = current.previous
        if (previous) {
          return previous.location.sourceURL
        }
      }

      current = this.previous
    }
  }

  /**
   * This function _ALWAYS__ returns `false` as `this` cannot be determined so
   * "top level" detection is not possible.
   * @return {boolean}
   */
  isTopLevel () {
    return false
  }

  /**
   * Returns `true` if this call site originated from a call to `eval()`.
   * @return {boolean}
   */
  isEval () {
    let current = this

    while (current) {
      if (current.frame.symbol === 'eval' || current.frame.symbol === 'eval code') {
        return true
      }

      current = this.previous
    }

    return false
  }

  /**
   * Returns `true` if the call site is in a native location, otherwise `false`.
   * @return {boolean}
   */
  isNative () {
    return this.#frame.location.isNative
  }

  /**
   * This function _ALWAYS_ returns `false` as constructor detection
   * is not possible.
   * @return {boolean}
   */
  isConstructor () {
    // not supported
    return false
  }

  /**
   * Returns `true` if the call site is in async context, otherwise `false`.
   * @return {boolean}
   */
  isAsync () {
    return isAsyncContext
  }

  /**
   * Returns `true` if the call site is in a `Promise.all()` function call,
   * otherwise `false.
   * @return {boolean}
   */
  isPromiseAll () {
    return this.#error[CallSite.PromiseAllSymbol] === true
  }

  /**
   * Gets the index of the promise element that was followed in a
   * `Promise.all()` or `Promise.any()` function call. If not available, then
   * this function returns `null`.
   * @return {number|null}
   */
  getPromiseIndex () {
    return this.#error[CallSite.PromiseElementIndexSymbol] ?? null
  }

  /**
   * Converts this call site to a string.
   * @return {string}
   */
  toString () {
    const { symbol, location } = this.#frame
    const output = [symbol]

    if (location.sourceURL) {
      const pathname = new URL(location.sourceURL).pathname
      const root = new URL('../../', import.meta.url || globalThis.location.href).pathname

      let filename = pathname.replace(root, '')

      if (/\/?socket\//.test(filename)) {
        filename = filename.replace('socket/', 'socket:').replace(/.js$/, '')
      }

      if (location.lineNumber && location.columnNumber) {
        output.push(`(${filename}:${location.lineNumber}:${location.columnNumber})`)
      } else if (location.lineNumber) {
        output.push(`(${filename}:${location.lineNumber})`)
      } else {
        output.push(`${filename}`)
      }
    }

    return output.filter(Boolean).join(' ')
  }

  /**
   * Converts this `CallSite` to a JSON object.
   * @ignore
   * @return {{
   *   frame: {
   *     location: {
   *       lineNumber: number | undefined,
   *       columnNumber: number | undefined,
   *       sourceURL: string | null,
   *       isNative: boolean
   *     },
   *     isNative: boolean,
   *     symbol: string | null,
   *     source: string | null,
   *     error: { message: string, name: string, stack: string } | null
   *   }
   * }}
   */
  toJSON () {
    return {
      frame: this.#frame.toJSON()
    }
  }

  /**
   * Serializes this `CallSite`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {{
   *   __type__: 'CallSite',
   *   frame: {
   *     __type__: 'StackFrame',
   *     location: {
   *       __type__: 'StackFrameLocation',
   *       lineNumber: number | undefined,
   *       columnNumber: number | undefined,
   *       sourceURL: string | null,
   *       isNative: boolean
   *     },
   *     isNative: boolean,
   *     symbol: string | null,
   *     source: string | null,
   *     error: { message: string, name: string, stack: string } | null
   *   }
   * }}
   */
  [InternalSymbols.serialize] () {
    return {
      __type__: 'CallSite',
      frame: this.#frame[InternalSymbols.serialize]()
    }
  }
}

/**
 * An array based list container for `CallSite` instances.
 */
export class CallSiteList extends Array {
  /**
   * Creates a `CallSiteList` instance from `Error` input.
   * @param {Error} error
   * @param {string} source
   * @return {CallSiteList}
   */
  static from (error, source) {
    const callsites = new this(
      error,
      source.split('\n').slice(0, Error.stackTraceLimit)
    )

    for (const source of callsites.sources.reverse()) {
      callsites.unshift(new CallSite(error, source, callsites[0]))
    }

    return callsites
  }

  #error = null
  #sources = []

  /**
   * `CallSiteList` class constructor.
   * @param {Error} error
   * @param {string[]=} [sources]
   */
  constructor (error, sources = null) {
    super()
    this.#error = error

    if (Array.isArray(sources)) {
      this.#sources = Array.from(sources) // copy
    } else if (typeof error.stack === 'string') {
      this.#sources = error.stack.split('\n')
    } else if (typeof error[CallSiteList.StackSourceSymbol] === 'string') {
      this.#sources = error[CallSiteList.StackSourceSymbol].split('\n')
    }
  }

  /**
   * A reference to the `Error` for this `CallSiteList` instance.
   * @type {Error}
   */
  get error () {
    return this.#error
  }

  /**
   * An array of stack frame source strings.
   * @type {string[]}
   */
  get sources () {
    return this.#sources
  }

  /**
   * The original stack string derived from the sources.
   * @type {string}
   */
  get stack () {
    return this.#sources.join('\n')
  }

  /**
   * Adds `CallSite` instances to the top of the list, linking previous
   * instances to the next one.
   * @param {...CallSite} callsites
   * @return {number}
   */
  unshift (...callsites) {
    for (const callsite of callsites) {
      if (callsite instanceof CallSite) {
        callsite[$previous] = this[0]
        super.unshift(callsite)
      }
    }

    return this.length
  }

  /**
   * A no-op function as `CallSite` instances cannot be added to the end
   * of the list.
   * @return {number}
   */
  push () {
    // no-op
    return this.length
  }

  /**
   * Pops a `CallSite` off the end of the list.
   * @return {CallSite|undefined}
   */
  pop () {
    const value = super.pop()

    if (this.length >= 1) {
      this[this.length - 1][$previous] = null
    }

    return value
  }

  /**
   * Converts the `CallSiteList` to a string combining the error name, message,
   * and callsite stack information into a friendly string.
   * @return {string}
   */
  toString () {
    const stack = this.map((callsite) => `  at ${callsite.toString()}`)
    if (this.#error.name && this.#error.message) {
      return [`${this.#error.name}: ${this.#error.message}`].concat(stack).join('\n')
    } else if (this.#error.name) {
      return [this.#error.name].concat(stack).join('\n')
    } else {
      return stack.join('\n')
    }
  }

  /**
   * Converts this `CallSiteList` to a JSON object.
   * @return {{
   *   frame: {
   *     location: {
   *       lineNumber: number | undefined,
   *       columnNumber: number | undefined,
   *       sourceURL: string | null,
   *       isNative: boolean
   *     },
   *     isNative: boolean,
   *     symbol: string | null,
   *     source: string | null,
   *     error: { message: string, name: string, stack: string } | null
   *   }
   * }[]}
   */
  toJSON () {
    return Array.from(this.map((callsite) => callsite.toJSON()))
  }

  /**
   * Serializes this `CallSiteList`, suitable for `postMessage()` transfers.
   * @ignore
   * @return {Array<{
   *   __type__: 'CallSite',
   *   frame: {
   *     __type__: 'StackFrame',
   *     location: {
   *       __type__: 'StackFrameLocation',
   *       lineNumber: number | undefined,
   *       columnNumber: number | undefined,
   *       sourceURL: string | null,
   *       isNative: boolean
   *     },
   *     isNative: boolean,
   *     symbol: string | null,
   *     source: string | null,
   *     error: { message: string, name: string, stack: string } | null
   *   }
   * }>}
   */
  [InternalSymbols.serialize] () {
    return this.toJSON()
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.runtime.util.inspect.custom')] () {
    return this.toString()
  }
}

/**
 * Creates an ordered and link array of `CallSite` instances from a
 * given `Error`.
 * @param {Error} error
 * @param {string} source
 * @return {CallSite[]}
 */
export function createCallSites (error, source) {
  return CallSiteList.from(error, source)
}

export default CallSite
