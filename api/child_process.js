/* global ErrorEvent */
import { EventEmitter } from './events.js'
import diagnostics from './diagnostics.js'
import { Worker } from './worker_threads.js'
import { Buffer } from './buffer.js'
import { rand64 } from './crypto.js'
import process from './process.js'
import ipc from './ipc.js'
import gc from './gc.js'

const dc = diagnostics.channels.group('child_process', [
  'spawn',
  'close',
  'exit',
  'kill'
])

class ChildProcess extends EventEmitter {
  #id = rand64()
  #worker = null
  #signal = null
  #timeout = null
  #env = { ...process.env }
  #state = {
    killed: false,
    signalCode: null,
    exitCode: null,
    spawnfile: null,
    spawnargs: [],
    lifecycle: 'init',
    pid: 0
  }

  constructor (options) {
    super()

    //
    // This does not implement disconnect or message because this is not node!
    //
    const workerLocation = new URL('./child_process/worker.js', import.meta.url)

    // TODO(@jwerle): support environment variable inject
    if (options?.env && typeof options?.env === 'object') {
      this.#env = options.env
    }

    this.#worker = new Worker(workerLocation.toString(), {
      env: options?.env ?? {},
      stdin: options?.stdin !== false,
      stdout: options?.stdout !== false,
      stderr: options?.stderr !== false,
      workerData: { id: this.#id }
    })

    if (options?.signal) {
      this.#signal = options.signal
      this.#signal.addEventListener('abort', () => {
        this.emit('error', new Error(this.#signal.reason))
        this.kill(options?.killSignal ?? 'SIGKILL')
      })
    }

    if (options?.timeout) {
      this.#timeout = setTimeout(() => {
        this.emit('error', new Error('Child process timed out'))
        this.kill(options?.killSignal ?? 'SIGKILL')
      }, options.timeout)

      this.once('exit', () => {
        clearTimeout(this.#timeout)
      })
    }

    this.#worker.on('message', data => {
      if (data.method === 'kill' && data.args[0] === true) {
        this.#state.killed = true
      }

      if (data.method === 'state') {
        if (this.#state.pid !== data.args[0].pid) this.emit('spawn')
        Object.assign(this.#state, data.args[0])

        switch (this.#state.lifecycle) {
          case 'spawn': {
            gc.ref(this)
            dc.channel('spawn').publish({ child_process: this })
            break
          }

          case 'exit': {
            this.emit('exit', this.#state.exitCode)
            dc.channel('exit').publish({ child_process: this })
            break
          }

          case 'close': {
            this.emit('close', this.#state.exitCode)
            dc.channel('close').publish({ child_process: this })
            break
          }

          case 'kill': {
            this.#state.killed = true
            dc.channel('kill').publish({ child_process: this })
            break
          }
        }
      }

      if (data.method === 'exit') {
        this.emit('exit', data.args[0])
      }
    })

    this.#worker.on('error', err => {
      this.emit('error', err)
    })
  }

  /**
   * `true` if the child process was killed with kill()`,
   * otherwise `false`.
   * @type {boolean}
   */
  get killed () {
    return this.#state.killed
  }

  /**
   * The process identifier for the child process. This value is
   * `> 0` if the process was spawned successfully, otherwise `0`.
   * @type {number}
   */
  get pid () {
    return this.#state.pid
  }

  /**
   * The executable file name of the child process that is launched. This
   * value is `null` until the child process has successfully been spawned.
   * @type {string?}
   */
  get spawnfile () {
    return this.#state.spawnfile ?? null
  }

  /**
   * The full list of command-line arguments the child process was spawned with.
   * This value is an empty array until the child process has successfully been
   * spawned.
   * @type {string[]}
   */
  get spawnargs () {
    return this.#state.spawnargs
  }

  /**
   * Always `false` as the IPC messaging is not supported.
   * @type {boolean}
   */
  get connected () {
    return false
  }

  /**
   * The child process exit code. This value is `null` if the child process
   * is still running, otherwise it is a positive integer.
   * @type {number?}
   */
  get exitCode () {
    return this.#state.exitCode ?? null
  }

  /**
   * If available, the underlying `stdin` writable stream for
   * the child process.
   * @type {import('./stream').Writable?}
   */
  get stdin () {
    return this.#worker.stdin ?? null
  }

  /**
   * If available, the underlying `stdout` readable stream for
   * the child process.
   * @type {import('./stream').Readable?}
   */
  get stdout () {
    return this.#worker.stdout ?? null
  }

  /**
   * If available, the underlying `stderr` readable stream for
   * the child process.
   * @type {import('./stream').Readable?}
   */
  get stderr () {
    return this.#worker.stderr ?? null
  }

  /**
   * The underlying worker thread.
   * @ignore
   * @type {import('./worker_threads').Worker}
   */
  get worker () {
    return this.#worker
  }

  /**
   * This function does nothing, but is present for nodejs compat.
   */
  disconnect () {
    return false
  }

  /**
   * This function does nothing, but is present for nodejs compat.
   * @return {boolean}
   */
  send () {
    return false
  }

  /**
   * This function does nothing, but is present for nodejs compat.
   */
  ref () {
    return false
  }

  /**
   * This function does nothing, but is present for nodejs compat.
   */
  unref () {
    return false
  }

  /**
   * Kills the child process. This function throws an error if the child
   * process has not been spawned or is already killed.
   * @param {number|string} signal
   */
  kill (...args) {
    if (!/spawn/.test(this.#state.lifecycle)) {
      throw new Error('Cannot kill a child process that has not been spawned')
    }

    if (this.killed) {
      throw new Error('Cannot kill an already killed child process')
    }

    this.#worker.postMessage({ id: this.#id, method: 'kill', args })
    return this
  }

  /**
   * Spawns the child process. This function will thrown an error if the process
   * is already spawned.
   * @param {string} command
   * @param {string[]=} [args]
   * @return {ChildProcess}
   */
  spawn (...args) {
    if (/spawning|spawn/.test(this.#state.lifecycle)) {
      throw new Error('Cannot spawn an already spawned ChildProcess')
    }

    if (!args[0] || typeof args[0] !== 'string') {
      throw new TypeError('Expecting command to be a string.')
    }

    this.#state.lifecycle = 'spawning'
    this.#worker.postMessage({
      id: this.#id,
      env: this.#env,
      method: 'spawn',
      args
    })

    return this
  }

  /**
   * `EventTarget` based `addEventListener` method.
   * @param {string} event
   * @param {function(Event)} callback
   * @param {{ once?: false }} [options]
   */
  addEventListener (event, callback, options = null) {
    callback.listener = (...args) => {
      if (event === 'error') {
        callback(new ErrorEvent('error', {
          target: this,
          error: args[0]
        }))
      } else {
        callback(new Event(event, args[0]))
      }
    }

    if (options?.once === true) {
      this.once(event, callback.listener)
    } else {
      this.on(event, callback.listener)
    }
  }

  /**
   * `EventTarget` based `removeEventListener` method.
   * @param {string} event
   * @param {function(Event)} callback
   * @param {{ once?: false }} [options]
   */
  removeEventListener (event, callback) {
    this.off(event, callback.listener ?? callback)
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.id],
      async handle (id) {
        const result = await ipc.send('child_process.kill', {
          id,
          signal: 'SIGTERM'
        })

        if (result.err) {
          console.warn(result.err)
        }
      }
    }
  }
}

/**
 * Spawns a child process exeucting `command` with `args`
 * @param {string} command
 * @param {string[]|object=} [args]
 * @param {object=} [options
 * @return {ChildProcess}
 */
export function spawn (command, args = [], options = null) {
  if (args && typeof args === 'object' && !Array.isArray(args)) {
    options = args
    args = []
  }

  if (!command || typeof command !== 'string') {
    throw new TypeError('Expecting command to be a string.')
  }

  if (args && typeof args === 'string') {
    args = args.split(' ')
  }

  const child = new ChildProcess(options)
  child.worker.on('online', () => child.spawn(command, args))
  // TODO signal
  // TODO timeout
  return child
}

export function exec (command, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  const child = spawn(command, options)
  const stdout = []
  const stderr = []
  let hasError = false

  if (child.stdout) {
    child.stdout.on('data', (data) => {
      if (hasError) {
        return
      }

      stdout.push(Buffer.from(data))
    })
  }

  if (child.stderr) {
    child.stderr.on('data', (data) => {
      if (hasError) {
        return
      }

      stderr.push(Buffer.from(data))
    })
  }

  child.once('error', (err) => {
    hasError = true
    if (typeof callback === 'function') {
      callback(err, null, null)
    }
  })

  if (typeof callback === 'function') {
    child.on('close', () => {
      if (hasError) {
        return
      }

      if (options?.encoding === 'buffer') {
        callback(
          null,
          Buffer.concat(stdout),
          Buffer.concat(stderr)
        )
      } else {
        const encoding = options?.encoding ?? 'utf8'
        callback(
          null,
          Buffer.concat(stdout).toString(encoding),
          Buffer.concat(stderr).toString(encoding)
        )
      }
    })
  }

  return Object.assign(child, {
    then (resolve, reject) {
      const promise = new Promise((resolve, reject) => {
        child.once('error', reject)
        child.once('close', () => {
          if (options?.encoding === 'buffer') {
            resolve({
              stdout: Buffer.concat(stdout),
              stderr: Buffer.concat(stderr)
            })
          } else {
            const encoding = options?.encoding ?? 'utf8'
            resolve({
              stdout: Buffer.concat(stdout).toString(encoding),
              stderr: Buffer.concat(stderr).toString(encoding)
            })
          }
        })
      })

      if (resolve && reject) {
        return promise.then(resolve, reject)
      } else if (resolve) {
        return promise.then(resolve)
      }

      return promise
    },

    catch (reject) {
      return this.then().catch(reject)
    },

    finally (next) {
      return this.then().finally(next)
    }
  })
}

export const execFile = exec

exec[Symbol.for('nodejs.util.promisify.custom')] =
exec[Symbol.for('socket.util.promisify.custom')] =
  async function execPromisify (command, options) {
    return await new Promise((resolve, reject) => {
      exec(command, options, (err, stdout, stderr) => {
        if (err) {
          reject(err)
        } else {
          resolve({ stdout, stderr })
        }
      })
    })
  }

export default {
  ChildProcess,
  spawn,
  execFile,
  exec
}
