import { CallSite, createCallSites } from './callsite.js'

/**
 * The default `Error` class stack trace limit.
 * @type {number}
 */
export const DEFAULT_ERROR_STACK_TRACE_LIMIT = 10

export const DefaultPlatformError = globalThis.Error

/**
 * @ignore
 * @param {typeof globalThis.Error} PlatformError
 * @param {Error} target
 * @param {...any} args
 * @return {Error}
 */
function applyPlatforErrorHook (PlatformError, Constructor, target, ...args) {
  const error = PlatformError.call(target, ...args)
  const stack = error.stack.split('\n').slice(2) // slice off the `Error()` + `applyPlatforErrorHook()` call frames
  const [, callsite] = stack[0].split('@')

  let stackValue = stack.join('\n')
  let sourceStack = stackValue

  Object.defineProperties(error, {
    [CallSite.StackSourceSymbol]: {
      configurable: false,
      enumerable: false,
      get: () => sourceStack
    },

    column: {
      configurable: true,
      enumerable: false,
      writable: true
    },

    line: {
      configurable: true,
      enumerable: false,
      writable: true
    },

    message: {
      configurable: true,
      enumerable: false,
      writable: true,
      value: error.message
    },

    name: {
      configurable: true,
      enumerable: false,
      writable: true,
      value: target.constructor.name
    },

    sourceURL: {
      configurable: true,
      enumerable: false,
      writable: true
    },

    stack: {
      configurable: true,
      enumerable: false,
      set: (stack) => {
        stackValue = stack
        if (stackValue && typeof stackValue === 'string') {
          sourceStack = stackValue
        }
      },
      get: () => {
        Object.defineProperty(error, 'stack', {
          configurable: true,
          enumerable: false,
          writable: true,
          value: stackValue
        })

        if (Error.stackTraceLimit === 0) {
          stackValue = `${error.name}: ${error.message}`
        } else {
          const prepareStackTrace = Constructor.prepareStackTrace || globalThis.Error.prepareStackTrace
          if (typeof prepareStackTrace === 'function') {
            const callsites = createCallSites(error, stackValue)
            stackValue = prepareStackTrace(error, callsites)
          }
        }

        Object.defineProperty(error, 'stack', {
          configurable: true,
          enumerable: false,
          writable: true,
          value: stackValue
        })

        return stackValue
      }
    }
  })

  if (URL.canParse(callsite)) {
    const url = new URL(callsite)
    const parts = url.pathname.split(':')
    const line = parseInt(parts[1])
    const column = parseInt(parts[2])

    error.sourceURL = new URL(parts[0] + url.search, url.origin).href

    if (Number.isFinite(line)) {
      error.line = line
    }

    if (Number.isFinite(column)) {
      error.column = column
    }
  } else {
    delete error.sourceURL
    delete error.column
    delete error.line
  }

  Object.setPrototypeOf(error, target)

  return error
}

/**
 * Creates and install a new runtime `Error` class
 * @param {typeof globalThis.Error} PlatformError
 * @param {boolean=} [isBaseError]
 * @return {typeof globalThis.Error}
 */
function installRuntimeError (PlatformError, isBaseError = false) {
  if (!PlatformError) {
    PlatformError = DefaultPlatformError
  }

  function Error (...args) {
    if (!(this instanceof Error)) {
      return new Error(...args)
    }

    return applyPlatforErrorHook(PlatformError, Error, this, ...args)
  }

  // directly inherit platform `Error` prototype
  Error.prototype = PlatformError.prototype

  /**
   * @ignore
   */
  Error.stackTraceLimit = DEFAULT_ERROR_STACK_TRACE_LIMIT

  Object.defineProperty(Error, 'captureStackTrace', {
    configurable: true,
    enumerable: false,
    // eslint-disable-next-line
    value (target) {
      if (!target || typeof target !== 'object') {
        throw new TypeError(
          `Invalid target given to ${PlatformError.name}.captureStackTrace. ` +
          `Expecting 'target' to be an object. Received: ${target}`
        )
      }

      // prepareStackTrace is already called there
      if (target instanceof Error) {
        target.stack = new PlatformError().stack.split('\n').slice(2).join('\n')
      } else {
        const stack = new PlatformError().stack.split('\n').slice(2).join('\n')
        const prepareStackTrace = Error.prepareStackTrace || globalThis.Error.prepareStackTrace
        if (typeof prepareStackTrace === 'function') {
          const callsites = createCallSites(target, stack)
          target.stack = prepareStackTrace(target, callsites)
        } else {
          target.stack = stack
        }
      }
    }
  })

  // Install
  globalThis[PlatformError.name] = Error

  return Error
}

export const Error = installRuntimeError(globalThis.Error, true)
export const URIError = installRuntimeError(globalThis.URIError)
export const EvalError = installRuntimeError(globalThis.EvalError)
export const TypeError = installRuntimeError(globalThis.TypeError)
export const RangeError = installRuntimeError(globalThis.RangeError)
export const MediaError = installRuntimeError(globalThis.MediaError)
export const SyntaxError = installRuntimeError(globalThis.SyntaxError)
export const ReferenceError = installRuntimeError(globalThis.ReferenceError)
export const AggregateError = installRuntimeError(globalThis.AggregateError)

// web
export const RTCError = installRuntimeError(globalThis.RTCError)
export const OverconstrainedError = installRuntimeError(globalThis.OverconstrainedError)
export const GeolocationPositionError = installRuntimeError(globalThis.GeolocationPositionError)

// not-standard
export const ApplePayError = installRuntimeError(globalThis.ApplePayError)

export default {
  Error,
  URIError,
  EvalError,
  TypeError,
  RangeError,
  MediaError,
  SyntaxError,
  ReferenceError,
  AggregateError,

  // web
  RTCError,
  OverconstrainedError,
  GeolocationPositionError,

  // non-standard
  ApplePayError
}
