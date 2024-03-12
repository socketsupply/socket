import { Snapshot } from './context.js'

export const symbol = Symbol.for('socket.runtime.async')

export function isTagged (fn) {
  if (typeof fn === 'function' && fn[symbol] === true) {
    return true
  }

  return false
}

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
