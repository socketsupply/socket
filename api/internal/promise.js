import * as asyncHooks from './async/hooks.js'

export const NativePromisePrototype = {
  then: globalThis.Promise.prototype.then,
  catch: globalThis.Promise.prototype.catch,
  finally: globalThis.Promise.prototype.finally
}

function wrapNativePromiseFunction (name) {
  const prototype = globalThis.Promise.prototype
  if (prototype[name].__async_wrapped__) {
    return
  }

  const nativeFunction = prototype[name]

  prototype[name] = function (...args) {
    if (name === 'then') {
      return nativeFunction.call(
        this,
        ...args.map((arg) => {
          if (typeof arg === 'function') {
            return asyncHooks.wrap(arg, 'init', 'Promise')
          }

          return arg
        })
      )
    }

    return nativeFunction.call(this, ...args)
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
