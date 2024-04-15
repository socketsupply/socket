/* global MessageChannel, MessagePort, EventTarget, Worker */
import { murmur3, randomBytes } from '../crypto.js'
import process from '../process.js'
import globals from './globals.js'
import os from '../os.js'
import gc from '../gc.js'

const workers = new Map()

export class SharedHybridWorkerProxy extends EventTarget {
  #eventListeners = []
  #started = false
  #channel = null
  #queue = []
  #port = null
  #url = null
  #id = null

  constructor (url, options) {
    super()

    this.#id = randomBytes(8).toString('base64')
    this.#url = new URL(url, globalThis.location.href)
    this.#channel = globals.get('internal.sharedWorker.channel')

    let onmessage = null

    // temporary port until one becomes acquired
    this.#port = Object.create(MessagePort.prototype, {
      onmessage: {
        configurable: true,
        enumerable: true,
        get: () => onmessage,
        set: (value) => {
          onmessage = value
          this.#port.start()
        }
      },

      onmessageerror: {
        configurable: true,
        enumerable: true
      }
    })

    this.#port.start = () => { this.#started = true }
    this.#port.close = () => {}
    this.#port.postMessage = (...args) => {
      this.#queue.push(args)
    }

    this.#port.addEventListener = (...args) => {
      this.#eventListeners.push(args)
    }

    this.#port.removeEventListener = (...args) => {
      this.#eventListeners = this.#eventListeners.filter((eventListener) => {
        if (
          eventListener[0] === args[0] &&
          eventListener[1] === args[1]
        ) {
          return false
        }

        return true
      })
    }

    if (!this.#channel) {
      throw new TypeError('Unable to acquire global SharedWorker BroadcastChannel')
    }

    this.onChannelMessage = this.onChannelMessage.bind(this)
    this.#channel.port2.addEventListener('message', this.onChannelMessage)
    this.#channel.port2.postMessage({
      __runtime_shared_worker_proxy_create: {
        id: this.#id,
        url: this.#url.toString(),
        options
      }
    })
  }

  get id () {
    return this.#id
  }

  get port () {
    return this.#port
  }

  onChannelMessage (event) {
    const eventListeners = this.#eventListeners
    const queue = this.#queue

    if (event.data?.__runtime_shared_worker_proxy_init) {
      const { id, port } = event.data.__runtime_shared_worker_proxy_init
      if (id === this.#id) {
        const { start } = port
        port.onmessage = this.#port.onmessage
        this.#port = port
        this.#port.start = () => {
          start.call(port)

          for (const entry of queue) {
            port.postMessage(...entry)
          }

          for (const entry of eventListeners) {
            port.addEventListener(...entry)
          }

          eventListeners.splice(0, eventListeners.length)
          queue.splice(0, queue.length)
        }

        if (this.#started) {
          this.#port.start()
        }

        gc.ref(this)
      }
    }
  }

  [gc.finalizer] () {
    return {
      args: [this.port, this.#channel, this.onChannelMessage],
      handler (port, channel, onChannelMessage) {
        try {
          port.close()
        } catch {}

        channel.removeEventListener('message', onChannelMessage)
      }
    }
  }
}

export class SharedHybridWorker extends EventTarget {
  #id = null
  #url = null
  #name = null
  #worker = null
  #channel = null
  #onmessage = null

  constructor (url, nameOrOptions) {
    super()
    if (typeof nameOrOptions === 'string') {
      this.#name = nameOrOptions
    }

    const options = nameOrOptions && typeof nameOrOptions === 'object'
      ? { ...nameOrOptions }
      : {}

    this.#url = new URL(url, globalThis.location.href)
    // id is based on current origin and worker path name
    this.#id = murmur3(globalThis.origin + this.#url.pathname)

    this.#worker = workers.get(this.#id) ?? new Worker(this.#url.toString(), {
      [Symbol.for('socket.runtime.internal.worker.type')]: 'sharedWorker'
    })

    this.#channel = new MessageChannel()

    workers.set(this.#id, this.#worker)

    this.#worker.addEventListener('error', (event) => {
      this.dispatchEvent(new Event(event.type, event))
    })

    // notify worker of new message channel, transfering `port2`
    // to be owned by the worker
    this.#worker.postMessage({
      __runtime_shared_worker: {
        ports: [this.#channel.port2],
        origin: globalThis.origin,
        options
      }
    }, { transfer: [this.#channel.port2] })
  }

  get port () {
    return this.#channel.port1
  }
}

const isInFrame = globalThis.window && globalThis.top !== globalThis.window

export function getSharedWorkerImplementationForPlatform () {
  if (
    os.platform() === 'android' ||
    (os.platform() === 'win32' && !process.env.COREWEBVIEW2_22_AVAILABLE)
  ) {
    if (isInFrame) {
      return SharedHybridWorkerProxy
    }

    return SharedHybridWorker
  }

  return globalThis.SharedWorker ?? SharedHybridWorker
}

export const SharedWorker = getSharedWorkerImplementationForPlatform()

if (!isInFrame) {
  const channel = new MessageChannel()
  globals.register('internal.sharedWorker.channel', channel)
  channel.port1.start()
  channel.port2.start()

  channel.port1.addEventListener('message', (event) => {
    if (event.data?.__runtime_shared_worker_proxy_create) {
      const { id, url, options } = event.data?.__runtime_shared_worker_proxy_create
      const worker = new SharedHybridWorker(url, options)
      channel.port1.postMessage({
        __runtime_shared_worker_proxy_init: {
          port: worker.port,
          id
        }
      }, { transfer: [worker.port] })
    }
  })
}

export default SharedWorker
