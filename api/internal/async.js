const NativePromisePrototype = {
  then: globalThis.Promise.prototype.then,
  catch: globalThis.Promise.prototype.catch,
  finally: globalThis.Promise.prototype.finally
}

let currentAsyncResourceId = 0

export class ExecutionAsyncResource {
  id = getNextAsyncResourceId()
}

export const topLevelExecutionAsyncResource = new ExecutionAsyncResource()
export const asyncResourceStack = [topLevelExecutionAsyncResource]

export function getNextAsyncResourceId () {
  return ++currentAsyncResourceId
}

export function getCurrentExecutionAsyncResource () {
  return asyncResourceStack[asyncResourceStack.length - 1]
}

export function wrap (callback) {
  if (typeof callback !== 'function') {
    return callback
  }

  return function (...args) {
    // snapshot
    try {
      asyncResourceStack.push(new ExecutionAsyncResource())
      // eslint-disable-next-line
      return callback(...args)
    } finally {
      // restore
      asyncResourceStack.pop()
    }
  }
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

export default null
