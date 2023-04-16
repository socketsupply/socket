/**
 * @module Process
 *
 * Example usage:
 * ```js
 * import process from 'socket:process'
 * ```
 */
import { primordials, send } from './ipc.js'
import { EventEmitter } from './events.js'
import os from './os.js'

let didEmitExitEvent = false

class Process extends EventEmitter {
  arch = primordials.arch
  argv = globalThis.__args?.argv ?? []
  argv0 = globalThis.__args?.argv?.[0] ?? ''
  cwd = () => primordials.cwd
  env = globalThis.__args?.env ?? {}
  exit = exit
  homedir = homedir
  platform = primordials.platform
  version = primordials.version
}

const isNode = Boolean(globalThis.process?.versions?.node)
const process = isNode
  ? globalThis.process
  : new Process()

if (!isNode) {
  EventEmitter.call(process)
}

export default process

export function nextTick (callback) {
  if (typeof process.nextTick === 'function' && process.nextTick !== nextTick) {
    process.nextTick(callback)
  } else if (typeof globalThis.setImmediate === 'function') {
    globalThis.setImmediate(callback)
  } else if (typeof globalThis.queueMicrotask === 'function') {
    globalThis.queueMicrotask(() => {
      try {
        callback()
      } catch (err) {
        setTimeout(() => { throw err })
      }
    })
  } else if (typeof globalThis.setTimeout === 'function') {
    globalThis.setTimeout(callback)
  } else if (typeof globalThis.requestAnimationFrame === 'function') {
    globalThis.requestAnimationFrame(callback)
  } else {
    throw new TypeError('\'process.nextTick\' is not supported in environment.')
  }
}

if (typeof process.nextTick !== 'function') {
  process.nextTick = nextTick
}

/**
 * @returns {string} The home directory of the current user.
 */
export function homedir () {
  return globalThis.__args.env.HOME ?? ''
}

/**
 * Computed high resolution time as a `BigInt`.
 * @param {Array<number>?} [time]
 * @return {bigint}
 */
export function hrtime (time = [0, 0]) {
  if (!time) time = [0, 0]
  if (time && (!Array.isArray(time) || time.length !== 2)) {
    throw new TypeError('Expecting time to be an array of 2 numbers.')
  }

  const value = os.hrtime()
  const seconds = BigInt(1e9)
  const x = value / seconds
  const y = value - (x * seconds)
  return [Number(x) - time[0], Number(y) - time[1]]
}

hrtime.bigint = function bigint () {
  return os.hrtime()
}

if (typeof process.hrtime !== 'function') {
  process.hrtime = hrtime
}

/**
 * @param {number=} [code=0] - The exit code. Default: 0.
 */
export async function exit (code) {
  if (!didEmitExitEvent) {
    didEmitExitEvent = true
    queueMicrotask(() => process.emit('exit', code))
    await send('application.exit', { value: code ?? 0 })
  }
}

export function memoryUsage () {
  const rss = memoryUsage.rss()
  return {
    rss
  }
}

if (typeof process.memoryUsage !== 'function') {
  process.memoryUsage = memoryUsage
}

memoryUsage.rss = function rss () {
  const rusage = os.rusage()
  return rusage.ru_maxrss
}
