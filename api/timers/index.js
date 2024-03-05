import { Timeout, Immediate, Interval } from './timer.js'
import scheduler from './scheduler.js'
import platform from './platform.js'
import promises from './promises.js'

export { platform }

export function setTimeout (callback, delay, ...args) {
  return Timeout.from(callback, delay, ...args)
}

export function clearTimeout (timeout) {
  if (timeout instanceof Timeout) {
    timeout.close()
  } else {
    platform.clearTimeout(timeout)
  }
}

export function setInterval (callback, delay, ...args) {
  return Interval.from(callback, delay, ...args)
}

export function clearInterval (interval) {
  if (interval instanceof Interval) {
    interval.close()
  } else {
    platform.clearInterval(interval)
  }
}

export function setImmediate (callback, ...args) {
  return Immediate.from(callback, ...args)
}

export function clearImmediate (immediate) {
  if (immediate instanceof Immediate) {
    immediate.close()
  } else {
    platform.clearImmediate(immediate)
  }
}

setTimeout[Symbol.for('nodejs.util.promisify.custom')] = promises.setTimeout
setTimeout[Symbol.for('socket.util.promisify.custom')] = promises.setTimeout

setInterval[Symbol.for('nodejs.util.promisify.custom')] = promises.setInterval
setInterval[Symbol.for('socket.util.promisify.custom')] = promises.setInterval

setImmediate[Symbol.for('nodejs.util.promisify.custom')] = promises.setImmediate
setImmediate[Symbol.for('socket.util.promisify.custom')] = promises.setImmediate

export default {
  platform,
  promises,
  scheduler,
  setTimeout,
  clearTimeout,
  setInterval,
  clearInterval,
  setImmediate,
  clearImmediate
}
