import AsyncContext from '../async-context.js'

const NativePromisePrototype = {
  then: globalThis.Promise.prototype.then,
  catch: globalThis.Promise.prototype.catch,
  finally: globalThis.Promise.prototype.finally
}

export function wrap (fn) {
  if (typeof fn === 'function') {
    return AsyncContext.Snapshot.wrap(fn)
  }

  return fn
}

globalThis.Promise.prototype.then = function (onFulfilled, onRejected) {
  return NativePromisePrototype.then.call(this, wrap(onFulfilled), wrap(onRejected))
}

globalThis.Promise.prototype.catch = function (onRejected) {
  return NativePromisePrototype.catch.call(this, wrap(onRejected))
}

globalThis.Promise.prototype.finally = function (callback) {
  return NativePromisePrototype.finally.call(this, wrap(callback))
}

export default { wrap }
