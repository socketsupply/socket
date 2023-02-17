/**
 * @module Process
 */
import { primordials, send } from './ipc.js'
import { EventEmitter } from './events.js'

let didEmitExitEvent = false

class Process extends EventEmitter {
  arch = primordials.arch
  argv = globalThis.__args?.argv ?? []
  argv0 = globalThis.__args?.argv?.[0] ?? null
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
    globalThis.queueMicrotask(callback)
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
  return window.__args.env.HOME ?? ''
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
