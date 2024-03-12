import * as asyncHooks from './async_hooks.js'
import { platform } from '../timers.js'

export function setTimeout (callback, ...args) {
  return platform.setTimeout(asyncHooks.wrap(callback, 'Timeout'), 0, ...args)
}

export function clearTimeout (timeout) {
  return platform.clearTimeout(timeout)
}

export function setInterval (callback, ...args) {
  return platform.setInterval(asyncHooks.wrap(callback, 'Timeout'), ...args)
}

export function clearInterval (interval) {
  return platform.clearInterval(interval)
}

export function setImmediate (callback, ...args) {
  return platform.setTimeout(asyncHooks.wrap(callback, 'Timeout'), 0, ...args)
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
