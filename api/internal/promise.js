// @ts-nocheck
import * as asyncHooks from './async/hooks.js'

const resourceSymbol = Symbol('PromiseResource')

export const NativePromise = globalThis.Promise
export const NativePromisePrototype = {
  then: globalThis.Promise.prototype.then,
  catch: globalThis.Promise.prototype.catch,
  finally: globalThis.Promise.prototype.finally
}

export const NativePromiseAll = globalThis.Promise.all.bind(globalThis.Promise)
export const NativePromiseAny = globalThis.Promise.any.bind(globalThis.Promise)

/**
 * @typedef {function(any): void} ResolveFunction
 */

/**
 * @typedef {function(Error|string|null): void} RejectFunction
 */

/**
 * @typedef {function(ResolveFunction, RejectFunction): void} ResolverFunction
 */

/**
 * @typedef {{
 *   promise: Promise,
 *   resolve: ResolveFunction,
 *   reject: RejectFunction
 * }} PromiseResolvers
 */

// @ts-ignore
export class Promise extends NativePromise {
  /**
   * Creates a new `Promise` with resolver functions.
   * @see {https://github.com/tc39/proposal-promise-with-resolvers}
   * @return {PromiseResolvers}
   */
  static withResolvers () {
    if (typeof super.withResolvers === 'function') {
      return super.withResolvers()
    }

    const resolvers = { promise: null, resolve: null, reject: null }
    resolvers.promise = new Promise((resolve, reject) => {
      resolvers.resolve = resolve
      resolvers.reject = reject
    })
    return resolvers
  }

  /**
   * `Promise` class constructor.
   * @ignore
   * @param {ResolverFunction} resolver
   */
  constructor (resolver) {
    super(resolver)
    // eslint-disable-next-line
    this[resourceSymbol] = new class Promise extends asyncHooks.CoreAsyncResource {
      constructor () {
        super('Promise')
      }
    }
  }
}

Promise.all = function (iterable) {
  return NativePromiseAll.call(NativePromise, Array.from(iterable).map((promise, index) => {
    if (!promise || typeof promise.catch !== 'function') {
      return promise
    }

    return promise.catch((err) => {
      return Promise.reject(Object.defineProperties(err, {
        [Symbol.for('socket.runtime.CallSite.PromiseElementIndex')]: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: index
        },

        [Symbol.for('socket.runtime.CallSite.PromiseAll')]: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: true
        }
      }))
    })
  }))
}

Promise.any = function (iterable) {
  return NativePromiseAny.call(NativePromise, Array.from(iterable).map((promise, index) => {
    if (!promise || typeof promise.catch !== 'function') {
      return promise
    }

    return promise.catch((err) => {
      return Promise.reject(Object.defineProperties(err, {
        [Symbol.for('socket.runtime.CallSite.PromiseElementIndex')]: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: index
        },

        [Symbol.for('socket.runtime.CallSite.PromiseAny')]: {
          configurable: false,
          enumerable: false,
          writable: false,
          value: true
        }
      }))
    })
  }))
}

function wrapNativePromiseFunction (name) {
  const prototype = Promise.prototype
  if (prototype[name].__async_wrapped__) {
    return
  }

  const nativeFunction = prototype[name]

  prototype[name] = function (...args) {
    if (asyncHooks.executionAsyncResource().type === 'RuntimeExecution') {
      return nativeFunction.call(this, ...args)
    }

    const resource = this[resourceSymbol]

    return nativeFunction.call(
      this,
      ...args.map((arg) => {
        if (typeof arg === 'function') {
          return asyncHooks.wrap(
            arg,
            'Promise',
            resource?.asyncId?.() ?? asyncHooks.getNextAsyncResourceId(),
            resource?.triggerAsyncId?.() ?? asyncHooks.getDefaultExecutionAsyncId(),
            resource ?? undefined
          )
        }

        return arg
      })
    )
  }

  Object.defineProperty(prototype[name], '__async_wrapped__', {
    configurable: false,
    enumerable: false,
    writable: false,
    value: true
  })
}

wrapNativePromiseFunction('then')
wrapNativePromiseFunction('catch')
wrapNativePromiseFunction('finally')

export default Promise
