import { Snapshot } from './context.js'

export const symbol = Symbol.for('socket.runtime.async')

/**
 * Returns `true` if a given function `fn` has the "async" wrapped tag,
 * meaning it was "tagged" in a `wrap(fn)` call before, otherwise this
 * function will return `false`.
 * @ignore
 * @param {function} fn
 * @param {boolean}
 */
export function isTagged (fn) {
  if (typeof fn === 'function' && fn[symbol] === true) {
    return true
  }

  return false
}

/**
 * Tags a function `fn` as being "async wrapped" so subsequent calls to
 * `wrap(fn)` do not wrap an already wrapped function.
 * @ignore
 * @param {function} fn
 * @return {function}
 */
export function tag (fn) {
  if (typeof fn !== 'function') {
    return fn
  }

  if (fn[symbol] === true) {
    return fn
  }

  Object.defineProperty(fn, symbol, {
    configurable: false,
    enumerable: false,
    writable: false,
    value: true
  })

  return fn
}

/**
 * Wraps a function `fn` that captures a snapshot of the current async
 * context. This function is idempotent and will not wrap a function more
 * than once.
 * @ignore
 * @param {function} fn
 * @return {function}
 */
export function wrap (fn) {
  if (typeof fn === 'function') {
    if (isTagged(fn)) {
      return fn
    }

    return tag(Snapshot.wrap(fn))
  }

  return fn
}

export default wrap
