import { CallSite } from './callsite.js'

function applyPlatforErrorHook (PlatformError, target, message, ...args) {
  const error = PlatformError.call(target, message, ...args)
  const stack = error.stack.split('\n').slice(2) // slice off the `Error()` + `applyPlatforErrorHook()` call frames
  const [, callsite] = stack[0].split('@')

  let stackValue = stack.join('\n')

  Object.defineProperties(error, {
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
      value: message
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
        Object.defineProperty(error, 'stack', {
          configurable: true,
          enumerable: false,
          writable: true,
          value: stackValue
        })
      },
      get: () => {
        if (Error.stackTraceLimit === 0) {
          stackValue = `${error.name}: ${error.message}`
        } else {
          if (typeof target.constructor.prepareStackTrace === 'function') {
            const callsites = []
            stackValue = target.constructor.prepareStackTrace(error, callsites)
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

function makeError (PlatformError) {
  const Error = function Error (message, ...args) {
    if (!(this instanceof Error)) {
      return new Error(message, ...args)
    }

    return applyPlatforErrorHook(PlatformError, this, message, ...args)
  }

  Error.prototype = PlatformError.prototype

  /**
   * @ignore
   */
  Error.stackTraceLimit = 32

  /**
   * @ignore
   */
  // eslint-disable-next-line
  Error.captureStackTrace = function (err, ErrorConstructor) {
    // TODO
  }

  // Install
  globalThis[PlatformError.name] = Error

  return Error
}

export const Error = makeError(globalThis.Error)
export const URIError = makeError(globalThis.URIError)
export const EvalError = makeError(globalThis.EvalError)
export const TypeError = makeError(globalThis.TypeError)
export const RangeError = makeError(globalThis.RangeError)
export const SyntaxError = makeError(globalThis.SyntaxError)
export const ReferenceError = makeError(globalThis.ReferenceError)

export default {
  Error
}
