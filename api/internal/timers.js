import { platform } from '../timers.js'
import { wrap } from './async.js'

export function setTimeout (callback, ...args) {
  return platform.setTimeout(wrap(callback), 0, ...args)
}

export function clearTimeout (timeout) {
  return platform.clearTimeout(timeout)
}

export function setInterval (callback, ...args) {
  return platform.setInterval(wrap(callback), ...args)
}

export function clearInterval (interval) {
  return platform.clearInterval(interval)
}

export function setImmediate (callback, ...args) {
  return platform.setTimeout(wrap(callback), 0, ...args)
}

export function clearImmediate (immediate) {
  return platform.clearTimeout(immediate)
}

export default {
  setTimeout,
  setInterval,
  setImmediate,
  clearTimeout,
  clearInterval,
  clearImmediate
}
