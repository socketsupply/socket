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
import { Peer } from './index.js'
import dgram from '../dgram.js'

const { pathname } = new URL(import.meta.url)

class Deferred {
  constructor () {
    this._promise = new Promise((resolve, reject) => {
      this.resolve = resolve
      this.reject = reject
    })
    this.then = this._promise.then.bind(this._promise)
    this.catch = this._promise.catch.bind(this._promise)
    this.finally = this._promise.finally.bind(this._promise)
    this[Symbol.toStringTag] = 'Promise'
  }
}

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

const isWorkerThread = !globalThis.window && globalThis.self === globalThis

/**
 * `Proxy` class factory, returns a Proxy class that is a proxy to the Peer.
 * @param {{ createSocket: function('udp4', null, object?): object }} options
 */
export class PeerWorkerProxy {
  #promises = new Map()
  #channel = null
  #worker = null
  #index = 0
  #port = null

  constructor (options, port, fn) {
    if (!isWorkerThread) {
      this.#channel = new MessageChannel()
      this.#worker = new window.Worker(pathname)

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
      if (!arg.peerId) continue
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

if (isWorkerThread) {
  let proxy
  let peer

  globalThis.addEventListener('message', ({ data: source }) => {
    if (proxy) return

    proxy = new PeerWorkerProxy(null, source.port, async function ({ data: args }) {
      const {
        prop,
        data,
        seq
      } = args

      switch (prop) {
        case 'create': {
          peer = new Peer(data, dgram)

          peer.onConnecting = (...args) => this.callMainThread('onConnecting', args)
          peer.onConnection = (...args) => this.callMainThread('onConnection', args)
          peer.onDisconnection = (...args) => this.callMainThread('onDisconnection', args)
          peer.onJoin = (...args) => this.callMainThread('onJoin', args)
          peer.onPacket = (...args) => this.callMainThread('onPacket', args)
          peer.onStream = (...args) => this.callMainThread('onStream', args)
          peer.onData = (...args) => this.callMainThread('onData', args)
          peer.onSend = (...args) => this.callMainThread('onSend', args)
          peer.onFirewall = (...args) => this.callMainThread('onFirewall', args)
          peer.onMulticast = (...args) => this.callMainThread('onMulticast', args)
          peer.onJoin = (...args) => this.callMainThread('onJoin', args)
          peer.onSync = (...args) => this.callMainThread('onSync', args)
          peer.onSyncStart = (...args) => this.callMainThread('onSyncStart', args)
          peer.onSyncEnd = (...args) => this.callMainThread('onSyncEnd', args)
          peer.onQuery = (...args) => this.callMainThread('onQuery', args)
          peer.onNat = (...args) => this.callMainThread('onNat', args)
          peer.onState = (...args) => this.callMainThread('onState', args)
          peer.onWarn = (...args) => this.callMainThread('onWarn', args)
          peer.onError = (...args) => this.callMainThread('onError', args)
          peer.onReady = (...args) => this.callMainThread('onReady', args)
          break
        }

        case 'compileCachePredicate': {
          // eslint-disable-next-line
          let predicate = new Function(`return ${data.toString()}`)()
          predicate = predicate.bind(peer)
          peer.cachePredicate = packet => predicate(packet)
          break
        }

        default: {
          if (isNaN(seq) && peer[prop]) {
            peer[prop] = data
            return
          }

          let r

          try {
            if (typeof peer[prop] === 'function') {
              if (Array.isArray(data)) {
                r = await peer[prop](...data)
              } else {
                r = await peer[prop](data)
              }
            } else {
              r = peer[prop]
            }
          } catch (err) {
            console.error(err)
            return this.resolveMainThread(seq, { err })
          }

          this.resolveMainThread(seq, { data: r })
        }
      }
    })
  })
}
