/**
 * A utility to run run the protocol in a thread seperate from the UI.
 *
 * import { network } from
 *
 * Socket        Node
 * ---           ---
 * API           API
 * Proxy         Protocol
 * Protocol
 *
 */
import { Deferred } from '../async.js'
import path from '../path.js'
const { pathname } = new URL(import.meta.url)

function deepClone (object, map = new Map()) {
  if (map.has(object)) return map.get(object)

  const isNull = object === null
  const isNotObject = typeof object !== 'object'
  const isArrayBuffer = object instanceof ArrayBuffer
  const isArray = Array.isArray(object)
  const isUint8Array = object instanceof Uint8Array
  const isMessagePort = object instanceof MessagePort

  if (isMessagePort || isNotObject || isNull || isArrayBuffer) return object
  if (isUint8Array) return new Uint8Array(object)
  if (isArrayBuffer) return object.slice(0)

  if (isArray) {
    const clonedArray = []
    map.set(object, clonedArray)
    for (const item of object) {
      clonedArray.push(deepClone(item, map))
    }
    return clonedArray
  }

  const clonedObj = {}
  map.set(object, clonedObj)
  for (const key in object) {
    clonedObj[key] = deepClone(object[key], map)
  }

  return clonedObj
}

function transferOwnership (...objects) {
  const transfers = []

  function add (value) {
    if (!transfers.includes(value)) {
      transfers.push(value)
    }
  }

  objects.forEach(object => {
    if (object instanceof ArrayBuffer || ArrayBuffer.isView(object)) {
      add(object.buffer)
    } else if (Array.isArray(object) || (object && typeof object === 'object')) {
      for (const value of Object.values(object)) {
        if (value instanceof MessagePort) add(value)
      }
    }
  })

  return transfers
}

/**
 * `Proxy` class factory, returns a Proxy class that is a proxy to the Peer.
 * @param {{ createSocket: function('udp4', null, object?): object }} options
 */
class PeerWorkerProxy {
  #promises = new Map()
  #channel = null
  #worker = null
  #index = 0
  #port = null

  constructor (options, port, fn) {
    if (!options.isWorkerThread) {
      this.#channel = new MessageChannel()
      this.#worker = new window.Worker(path.join(path.dirname(pathname), 'worker.js'))

      this.#worker.addEventListener('error', err => {
        throw err
      })

      this.#worker.postMessage({
        port: this.#channel.port2
      }, [this.#channel.port2])

      // when the main thread receives a message from the worker
      this.#channel.port1.onmessage = ({ data: args }) => {
        const {
          err,
          prop,
          data,
          seq
        } = args

        if (!prop && err) {
          throw new Error(err)
        }

        if (prop && typeof this[prop] === 'function') {
          try {
            if (Array.isArray(data)) {
              this[prop](...data)
            } else {
              this[prop](data)
            }
          } catch (err) {
            throw new Error(err)
          }
          return
        }

        const p = this.#promises.get(seq)
        if (!p) return

        if (!p) {
          console.warn(`No promise was found for the sequence (${seq})`)
          return
        }

        if (err) {
          p.reject(err)
        } else {
          p.resolve(data)
        }

        this.#promises.delete(seq)
      }

      this.callWorkerThread('create', options)
      return
    }

    this.#port = port
    this.#port.onmessage = fn.bind(this)
  }

  async init () {
    return await this.callWorkerThread('init')
  }

  async reconnect () {
    return await this.callWorkerThread('reconnect')
  }

  async disconnect () {
    return await this.callWorkerThread('disconnect')
  }

  async getInfo () {
    return await this.callWorkerThread('getInfo')
  }

  async getState () {
    return await this.callWorkerThread('getState')
  }

  async open (...args) {
    return await this.callWorkerThread('open', args)
  }

  async seal (...args) {
    return await this.callWorkerThread('seal', args)
  }

  async sealUnsigned (...args) {
    return await this.callWorkerThread('sealUnsigned', args)
  }

  async openUnsigned (...args) {
    return await this.callWorkerThread('openUnsigned', args)
  }

  async addEncryptionKey (...args) {
    return await this.callWorkerThread('addEncryptionKey', args)
  }

  async send (...args) {
    return await this.callWorkerThread('send', args)
  }

  async sendUnpublished (...args) {
    return await this.callWorkerThread('sendUnpublished', args)
  }

  async cacheInsert (...args) {
    return await this.callWorkerThread('cacheInsert', args)
  }

  async mcast (...args) {
    return await this.callWorkerThread('mcast', args)
  }

  async requestReflection (...args) {
    return await this.callWorkerThread('requestReflection', args)
  }

  async join (...args) {
    return await this.callWorkerThread('join', args)
  }

  async publish (...args) {
    return await this.callWorkerThread('publish', args)
  }

  async sync (...args) {
    return await this.callWorkerThread('sync', args)
  }

  async close (...args) {
    return await this.callWorkerThread('close', args)
  }

  async query (...args) {
    return await this.callWorkerThread('query', args)
  }

  async compileCachePredicate (src) {
    return await this.callWorkerThread('compileCachePredicate', src)
  }

  callWorkerThread (prop, data) {
    let transfer = []

    if (data) {
      data = deepClone(data)
      transfer = transferOwnership(data)
    }

    const seq = ++this.#index
    const d = new Deferred()

    this.#channel.port1.postMessage(
      { prop, data, seq },
      { transfer }
    )

    this.#promises.set(seq, d)
    return d
  }

  callMainThread (prop, args) {
    for (const i in args) {
      const arg = args[i]
      if (!arg?.peerId) continue
      args[i] = { ...arg }
      delete args[i].localPeer // don't copy this over
    }

    try {
      this.#port.postMessage(
        { data: deepClone(args), prop },
        { transfer: transferOwnership(args) }
      )
    } catch (err) {
      this.#port.postMessage({ data: { err: err.message, prop } })
    }
  }

  resolveMainThread (seq, data) {
    try {
      this.#port.postMessage(
        { data: deepClone(data), seq },
        { transfer: transferOwnership(data) }
      )
    } catch (err) {
      this.#port.postMessage({ data: { err: err.message } })
    }
  }
}

export { PeerWorkerProxy }
export default PeerWorkerProxy
