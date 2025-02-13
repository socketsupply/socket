/**
 * @module process
 *
 * Example usage:
 * ```js
 * import process from 'socket:process'
 * ```
 */
import { primordials, send } from './ipc.js'
import { EventEmitter } from './events.js'
import signal from './process/signal.js'
import tty from './tty.js'
import os from './os.js'

let didEmitExitEvent = false
let cwd = primordials.cwd

export class ProcessEnvironmentEvent extends Event {
  key
  value

  constructor (type, key, value) {
    super(type)
    this.key = key
    this.value = value ?? process.env[key] ?? undefined
  }
}

export class ProcessEnvironment extends EventTarget {
  get [Symbol.toStringTag] () {
    return 'ProcessEnvironment'
  }
}

export const env = Object.defineProperties(new ProcessEnvironment(), {
  proxy: {
    configurable: false,
    enumerable: false,
    writable: false,
    value: new Proxy({}, {
      get (_, property) {
        if (Reflect.has(env, property)) {
          return Reflect.get(env, property)
        }

        return Reflect.get(globalThis.__args.env, property)
      },

      set (_, property, value) {
        if (Reflect.get(env, property) !== value) {
          env.dispatchEvent(new ProcessEnvironmentEvent('set', property, value))
          env.dispatchEvent(new ProcessEnvironmentEvent('change', property, value))
        }
        return Reflect.set(env, property, value)
      },

      deleteProperty (_, property) {
        if (Reflect.has(env, property)) {
          // @ts-ignore
          env.dispatchEvent(new ProcessEnvironmentEvent('delete', property))
          env.dispatchEvent(new ProcessEnvironmentEvent('change', property))
        }
        return Reflect.deleteProperty(env, property)
      },

      getOwnPropertyDescriptor (_, property) {
        if (Reflect.has(globalThis.__args.env, property)) {
          return {
            configurable: true,
            enumerable: true,
            writable: true,
            value: globalThis.__args.env[property]
          }
        }
      },

      has (_, property) {
        return (
          Reflect.has(env, property) ||
          Reflect.has(globalThis.__args.env, property)
        )
      },

      ownKeys (_) {
        const keys = []
        keys.push(...Reflect.ownKeys(env))
        keys.push(...Reflect.ownKeys(globalThis.__args.env))
        return Array.from(new Set(keys))
      }
    })
  }
})

class Process extends EventEmitter {
  // @ts-ignore
  stdin = new tty.ReadStream(0)
  // @ts-ignore
  stdout = new tty.WriteStream(1)
  // @ts-ignore
  stderr = new tty.WriteStream(2)

  get version () {
    return primordials.version.short
  }

  get platform () {
    return primordials.platform
  }

  get env () {
    return env.proxy
  }

  get arch () {
    return primordials.arch
  }

  get argv () {
    return globalThis.__args?.argv ?? []
  }

  get argv0 () {
    return this.argv[0] ?? ''
  }

  get execArgv () {
    return []
  }

  get versions () {
    return {
      socket: this.version,
      uv: primordials.uv.version,
      llama: primordials.llama.version
    }
  }

  uptime () {
    return os.uptime()
  }

  cwd () {
    return cwd
  }

  exit (code) {
    return exit(code)
  }

  nextTick (callback, ...args) {
    return nextTick(callback, ...args)
  }

  hrtime (time = [0, 0]) {
    return hrtime(time)
  }

  memoryUsage () {
    return memoryUsage
  }

  chdir (dir) {
    cwd = dir
  }
}

const isNode = Boolean(globalThis.process?.versions?.node)
const process = isNode
  ? globalThis.process
  : new Process()

if (!isNode) {
  EventEmitter.call(process)
}

if (!isNode) {
  signal.channel.addEventListener('message', (event) => {
    if (event.data.signal) {
      const code = event.data.signal
      const name = signal.getName(code)
      const message = signal.getMessage(code)
      process.emit(name, name, code, message)
    }
  })

  globalThis.addEventListener('signal', (event) => {
    // @ts-ignore
    if (event.detail.signal) {
      // @ts-ignore
      const code = event.detail.signal
      const name = signal.getName(code)
      const message = signal.getMessage(code)
      process.emit(name, name, code, message)
    }
  })
}

export default process

/**
 * Adds callback to the 'nextTick' queue.
 * @param {Function} callback
 */
export function nextTick (callback, ...args) {
  if (isNode && typeof process.nextTick === 'function' && process.nextTick !== nextTick) {
    process.nextTick(callback, ...args)
  } else if (typeof globalThis.queueMicrotask === 'function') {
    globalThis.queueMicrotask(() => {
      try {
        // eslint-disable-next-line
        callback(...args)
      } catch (err) {
        setTimeout(() => { throw err })
      }
    })
  } else if (typeof globalThis.setImmediate === 'function') {
    globalThis.setImmediate(callback, ...args)
  } else if (typeof globalThis.setTimeout === 'function') {
    globalThis.setTimeout(callback, ...args)
  } else if (typeof globalThis.requestAnimationFrame === 'function') {
    // eslint-disable-next-line
    globalThis.requestAnimationFrame(() => callback(...args))
  } else {
    throw new TypeError('\'process.nextTick\' is not supported in environment.')
  }
}

if (typeof process.nextTick !== 'function') {
  process.nextTick = nextTick
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

process.hrtime.bigint = hrtime.bigint

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

/**
 * Returns an object describing the memory usage of the Node.js process measured in bytes.
 * @returns {Object}
 */
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

process.memoryUsage.rss = memoryUsage.rss
