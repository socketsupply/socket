import events from './events.js'
import ipc from './ipc.js'
import { Worker } from './worker_threads.js'
import { rand64 } from '../crypto.js'

class ChildProcess extends events.EventEmitter {
  constructor (options) {
    super()

    // 'close'
    // 'disconnect'
    // 'error'
    // 'exit'
    // 'message'
    // 'spawn' *

    this.#id = rand64()

    this.#state = {
      killed: false,
      signalCode: null,
      exitCode: null,
      spawnfile: null,
      pid: 0
    }

    this.#worker = new Worker('./child_process/worker.js', {
      env: options.env,
      stdin: true,
      stdout: true,
      stderr: true,
      workerData: { id: this.#id }
    })

    this.#worker.on('message', data => {
      if (data.method === 'kill' && args[0] === true) {
        this.#killed = true
      }

      if (data.method === 'state') {
        if (this.#state.pid !== args[0].pid) this.emit('spawn')
        Object.assign(this.#state, args[0])
      }
    })

    this.#worker.on('error', err => {
      this.emit('error', err)
    })
  }

  get stdin () {
    this.#worker.stdin
  }

  get stdout () {
    this.#worker.stdout
  }

  get stderr () {
    this.#worker.stderr
  }

  kill (...args) {
    this.#worker.postMessage({ id: this.#id, method: 'kill', args })
  }

  spawn (...args) {
    this.#worker.postMessage({ id: this.#id, method: 'spawn', args })
  }
}

function spawn (command, args, options) {
  const child = new ChildProcess(options)
  child.spawn(command, args)
  // TODO signal
  // TODO timeout
  return child
}

export { spawn }
