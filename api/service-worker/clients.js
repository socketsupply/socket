import { SHARED_WORKER_URL } from './instance.js'
import { SharedWorker } from '../internal/shared-worker.js'
import state from './state.js'
import ipc from '../ipc.js'

export class Client {
  #id = null
  #url = null
  #type = null
  #frameType = null
  #sharedWorker = null

  constructor () {
    this.#sharedWorker = new SharedWorker(SHARED_WORKER_URL)
    this.#sharedWorker.port.start()
  }

  postMessage (message, optionsOrTransferables = null) {
    return this.#sharedWorker.port.postMessage(message, optionsOrTransferables)
  }
}

export class WindowClient extends Client {
  focus () {
  }

  navigate () {
  }
}

export class Clients {
  async get (id) {
  }

  async matchAll () {
  }

  async openWindow () {
  }

  async claim () {
  }
}

export default Clients
