/**
 * @module Process
 */
import { pArch, pPlatform, pCwd, send } from './ipc.js'
import { EventEmitter } from './events.js'

let didEmitExitEvent = false

class Process extends EventEmitter {
  arch = pArch
  argv = globalThis.__args?.argv ?? []
  argv0 = globalThis.__args?.argv?.[0] ?? null
  config = globalThis.__args?.config ?? {}
  cwd = () => pCwd
  env = globalThis.__args?.env ?? {}
  exit = exit
  homedir = homedir
  platform = pPlatform
  version = globalThis.__args?.version ?? ''
}

const isNode = Boolean(globalThis.process?.versions?.node)
const process = isNode
  ? globalThis.process
  : new Process()

if (!isNode) {
  EventEmitter.call(process)
}

export default process

/**
 * @returns {string} The home directory of the current user.
 */
export function homedir () {
  return process.env.HOME ?? ''
}

/**
 * @param {number=} [code=0] - The exit code. Default: 0.
 */
export function exit (code) {
  if (!didEmitExitEvent) {
    didEmitExitEvent = true
    queueMicrotask(() => process.emit('exit', code))
    send('exit', { value: code ?? 0 })
  }
}
