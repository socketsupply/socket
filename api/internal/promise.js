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

globalThis.Promise = class Promise extends NativePromise {
  constructor (...args) {
    super(...args)
    // eslint-disable-next-line
    this[resourceSymbol] = new class Promise extends asyncHooks.CoreAsyncResource {
      constructor () {
        super('Promise')
      }
    }
  }
}

globalThis.Promise.all = function (iterable) {
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

globalThis.Promise.any = function (iterable) {
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
  const prototype = globalThis.Promise.prototype
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
