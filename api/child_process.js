import events from './events.js'
import { Worker } from './worker_threads.js'
import { rand64 } from './crypto.js'

class ChildProcess extends events.EventEmitter {
  #id = rand64()
  #worker = null
  #state = {
    killed: false,
    signalCode: null,
    exitCode: null,
    spawnfile: null,
    pid: 0
  }

  constructor (options) {
    super()

    //
    // This does not implement disconnect or message because this is not node!
    //
    const workerLocation = new URL('./child_process/worker.js', import.meta.url)

    this.#worker = new Worker(workerLocation.toString(), {
      env: options.env,
      stdin: true,
      stdout: true,
      stderr: true,
      workerData: { id: this.#id }
    })

    this.#worker.on('message', data => {
      if (data.method === 'kill' && data.args[0] === true) {
        this.#state.killed = true
      }

      if (data.method === 'state') {
        if (this.#state.pid !== data.args[0].pid) this.emit('spawn')
        Object.assign(this.#state, data.args[0])
        if (this.#state.lifecycle === 'exit') this.emit('exit', this.#state.exitCode)
        if (this.#state.lifecycle === 'close') this.emit('close', this.#state.exitCode)
      }

      if (data.method === 'exit') {
        this.emit('exit', data.args[0])
      }
    })

    this.#worker.on('error', err => {
      this.emit('error', err)
    })
  }

  get stdin () {
    return this.#worker.stdin
  }

  get stdout () {
    return this.#worker.stdout
  }

  get stderr () {
    return this.#worker.stderr
  }

  get worker () {
    return this.#worker
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
  child.worker.on('online', () => child.spawn(command, args))
  // TODO signal
  // TODO timeout
  return child
}

export { spawn }
export default { spawn }
