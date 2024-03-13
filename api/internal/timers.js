import { Timeout, Interval, Immediate } from '../timers/timer.js'
import { platform } from '../timers.js'

export function setTimeout (callback, ...args) {
  return Timeout.from(callback, ...args).id
}

export function clearTimeout (timeout) {
  return platform.clearTimeout(timeout)
}

export function setInterval (callback, ...args) {
  return Interval.from(callback, ...args).id
}

export function clearInterval (interval) {
  return platform.clearInterval(interval)
}

export function setImmediate (callback, ...args) {
  return Immediate.from(callback, ...args).id
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
