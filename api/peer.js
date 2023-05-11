/**
 * @module Peer
 *
 * Provides a higher level API over the stream-relay protocol.
 *
 * Example usage:
 * ```js
 * import { Peer } from 'socket:peer'
 * ```
 *
 * @see {@link https://socketsupply.co/guides/#p2p-guide}
 *
 */
import { createPeer, Encryption, sha256 } from './stream-relay/index.js'
import { sodium, randomBytes } from './crypto.js'
import { EventEmitter } from './events.js'
import { isBufferLike } from './util.js'

import dgram from './dgram.js'

const PeerFactory = createPeer(dgram)
const textDecoder = new TextDecoder()

/**
 *
 * @class
 * @public
 *
 *The Peer class is an EventEmitter. It emits events when new network events
 *are received (.on), it can also emit new events to the network (.emit).
 *
 *```js
 *import { Peer } from 'socket:peer'
 *
 *const pair = await Peer.createKeys()
 *const clusterId = await Peer.createClusterId()
 *
 *const peer = new Peer({ ...pair, clusterId })
 *
 *peer.on('greeting', (value, peer, address, port) => {
 *  console.log(value)
 *})
 *
 *window.onload = () => {
 *  const value = { english: 'hello, world' }
 *  const packet = await peer.emit('greeting', value)
 *}
 *```
 *
 */

export class Peer extends EventEmitter {
  /**
   * `Peer` class constructor.
   * @param {Object} options - All options for the Peer constructor
   * @param {string} options.publicKey - The public key required to sign and read
   * @param {string} options.privateKey - The private key required to sign and read
   * @param {string} options.clusterId - A unique appliction identity
   * @param {string} options.scheme - Specify which encryption scheme to use (ie, 'PTP')
   * @param {Array} options.peers - An array of RemotePeer
   */
  constructor (opts = {}) {
    super(opts)

    if (!opts.clusterId) {
      throw new Error('A clusterId must be specified (string)')
    }

    this.peer = null
    this._emit = super.emit
    this.opts = opts
  }

  /**
   * A method that will generate a public and private key pair.
   * The ed25519 pair can be stored by an app with a secure API.
   *
   * @static
   * @return {Object<Pair>} pair - A pair of keys
   *
   */
  static async createKeys () {
    await sodium.ready
    const enc = new Encryption()

    return {
      publicKey: enc.publicKey,
      privateKey: enc.privateKey
    }
  }

  /**
   * Create a clusterId from random bytes
   * @return {string} id - a hex encoded sha256 hash
   */
  static async createClusterId () {
    return (await sha256(randomBytes(32))).toString('hex')
  }

  /**
   * Emits a message to the network
   *
   * @param {string} event - The name of the event to emit to the network
   * @param {Buffer} message - The data to emit to the network
   * @return {Object<Packet>} The packet that will be sent when possible
   */
  async emit (eventName, message, ...args) {
    if (!eventName.length) throw new Error('unable to emit unnamed event')
    if (!message) throw new Error('a message is required')
    if (!this.peer) throw new Error('must call socket.join() first')

    return await this.peer.publish({
      clusterId: this.opts.clusterId,
      to: this.opts.publicKey,
      usr1: eventName,
      message,
      ...args
    })
  }

  async query () {
    // TODO expose the query API
  }

  /**
   * Starts the process of connecting to the network.
   *
   * @return {Peer} Returns an instance of the underlying network peer
   */
  async join () {
    if (this.peer) return this.peer

    this.opts.clusterId = await sha256(this.opts.clusterId)
    this.opts.peerId = this.opts.peerId || (await sha256(randomBytes(32))).toString('hex')

    this.peer = await PeerFactory.create({ ...this.opts })

    this.peer.onPacket = (packet, port, address, data) => {
      const event = packet.usr1
      let message = packet.message

      if (!event || typeof event !== 'string') return

      if (isBufferLike(message)) {
        try {
          message = textDecoder.decode(message)
        } catch (err) {}
      }

      if (typeof message === 'string') {
        try {
          message = JSON.parse(message)
        } catch (err) {
          this.emit('error', err)
          return
        }
      }

      this._emit(event, message, address, port, data)
    }

    this.#peerSubscribe('onData', 'data')
    this.#peerSubscribe('onClusterData', 'cluster-data')
    this.#peerSubscribe('onClose', 'close')

    if (this.opts.publicKey && this.opts.privateKey) {
      this.peer.encryption.add(this.opts.publicKey, this.opts.privateKey)
    }

    await this.peer.init()
    return this.peer
  }

  getPeerId () {
    return this.peer?.peerId
  }

  close () {
    if (this.peer) this.peer.close()
  }

  #peerSubscribe (handlerName, eventName) {
    const oldHandler = this.peer[handlerName]
    this.peer[handlerName] = (...args) => {
      this._emit(eventName, ...args)
      if (oldHandler) oldHandler.call(this.peer, ...args)
    }
  }
}

export default Peer
