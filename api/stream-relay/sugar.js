import { wrap, Encryption, sha256, NAT } from './index.js'
import { sodium } from '../crypto.js'
import { Buffer } from '../buffer.js'
import { isBufferLike } from '../util.js'
import { CACHE_TTL, PACKET_BYTES } from './packets.js'

export default (dgram, events) => {
  let _peer = null
  let bus = null

  /*
   * Creates an instance of the underlying network protocol (this can only be
   * called once, subsequent calls will return the same instance).
   * The method returned method should be exposed to the user of the module.
   *
   * @param {object} - options
   * @param {clusterId} - options.clusterId
   * @param {peerId} - options.peerId
   *
   * @return {object}
   *
   * @example
   *
   *```js
   * import { network } from '../network.js'
   *
   * const socket = network()
   *```
   */
  return async options => {
    if (bus) return bus

    await sodium.ready
    bus = new events.EventEmitter()
    bus.peers = new Map()
    bus._on = bus.on
    bus._once = bus.once
    bus._emit = bus.emit

    if (!options.indexed && !options.clusterId && !options.config?.clusterId) {
      throw new Error('expected .clusterId property')
    }

    const clusterId = bus.clusterId = options.clusterId || options.config?.clusterId

    _peer = new (wrap(dgram))(options) // only one peer per process makes sense

    _peer.onPacket = (packet, ...args) => {
      if (packet.clusterId !== clusterId) return
      bus._emit('#packet', packet, ...args)
    }

    _peer.onJoin = (packet, ...args) => {
      if (packet.clusterId !== clusterId) return
      bus._emit('#join', packet, ...args)
    }

    _peer.onStream = (packet, ...args) => {
      if (packet.clusterId !== clusterId) return
      bus._emit('#stream', packet, ...args)
    }

    _peer.onData = (...args) => bus._emit('#data', ...args)
    _peer.onSend = (...args) => bus._emit('#send', ...args)
    _peer.onMulticast = (...args) => bus._emit('#multicast', ...args)
    _peer.onConnection = (...args) => bus._emit('#connection', ...args)
    _peer.onDisconnection = (...args) => bus._emit('#disconnection', ...args)
    _peer.onQuery = (...args) => bus._emit('#query', ...args)
    _peer.onNat = (...args) => bus._emit('#network-change', ...args)
    _peer.onError = (...args) => bus._emit('#error', ...args)
    _peer.onWarn = (...args) => bus._emit('#warning', ...args)
    _peer.onState = (...args) => bus._emit('#state', ...args)
    _peer.onConnecting = (...args) => bus._emit('#connecting', ...args)

    bus.discovered = new Promise(resolve => bus._once('#discovered', resolve))
    bus.peer = _peer
    bus.peerId = _peer.peerId

    bus.setMaxListeners(1024)
    bus.subclusters = new Map()

    bus.address = () => ({
      address: _peer.address,
      port: _peer.port,
      natType: NAT.toString(_peer.natType)
    })

    bus.join = () => _peer.requestReflection()
    bus.cacheSize = () => _peer.cache.size * PACKET_BYTES
    bus.open = (m, pk) => _peer.encryption.open(m, pk)

    bus.emit = async (...args) => {
      return await _peer.publish(options.sharedKey, await pack(...args))
    }

    bus.on = async (eventName, cb) => {
      if (eventName[0] !== '#') eventName = await sha256(eventName)
      bus._on(eventName, cb)
    }

    _peer.onReady = () => {
      _peer.isReady = true
      bus._emit('#discovered')
    }

    const pack = async (sub, eventName, value, opts = {}) => {
      if (typeof eventName !== 'string') throw new Error('event name must be a string')
      if (eventName.length === 0) throw new Error('event name too short')

      opts.ttl = Math.min(opts.ttl, CACHE_TTL)

      const args = {
        clusterId,
        ...opts,
        usr1: await sha256(eventName, { bytes: true })
      }

      if (!isBufferLike(value) && typeof value === 'object') {
        try {
          args.message = Buffer.from(JSON.stringify(value))
        } catch (err) {
          return bus._emit('error', err)
        }
      } else {
        args.message = Buffer.from(value)
      }

      if (opts.publicKey && opts.privateKey) {
        args.usr2 = Buffer.from(opts.publicKey)
        args.sig = Buffer.from(Encryption.sign(args.message, Buffer.from(opts.privateKey)))
      }

      return args
    }

    const unpack = async packet => {
      let opened
      let verified
      const sub = bus.subclusters.get(packet.subclusterId)
      if (!sub) return {}

      try {
        opened = _peer.encryption.open(packet.message, packet.subclusterId)
      } catch (err) {
        sub._emit('warning', err)
        return {}
      }

      if (packet.sig) {
        try {
          if (Encryption.verify(opened, packet.sig, packet.usr2)) {
            verified = true
          }
        } catch (err) {
          sub._emit('warning', err)
          return {}
        }
      }

      return { opened, verified }
    }

    //
    // Creates a peer if one doesn't exist, adds it to the right subcluster
    // and cluster.
    //
    const getPeerRepresentative = (packet, sub, peer) => {
      let ee

      if (sub && sub.peers.has(peer.peerId)) {
        ee = sub.peers.get(peer.peerId)
      } else {
        ee = new events.EventEmitter()
        ee._on = ee.on
        ee._once = ee.once
        ee._emit = ee.emit
        ee.peerId = peer.peerId
        ee.address = peer.address
        ee.port = peer.port
      }

      if (!bus.peers.has(peer.peerId)) {
        bus.peers.set(peer.peerId, ee)
      }

      if (!sub) return ee
      sub.peers.set(peer.peerId, ee)

      return ee
    }

    /*
     * Creates an event emitter that will handle inbound and outbound
     * events as well as connects for the specified subcluster.
     *
     * @param {string} name - The name of the subcluster
     * @param {object} keys - A keys object
     * @param {string} keys.publicKey - The public key property of the subcluster
     * @param {string} keys.privateKey - The private key property of the subcluster
     * @return {EventEmitter}
     *
     * @example
     *
     * ```js
     *    import fs from '../fs.js'
     *    import { network, Encryption } from '../network.js'
     *
     *    const clusterId = await Encryption.createClusterId()
     *    const socket = network({ clusterId })
     *
     *    socket.emit('foo', value) // publish a message to anyone in this cluster
     *    socket.on('foo', () => {}) // read published messags from anyone in this cluster
     *
     *    const sharedKey = await Encryption.createSharedKey()
     *    const sub = await socket.subcluster({ sharedKey })
     *
     *    sub.emit('foo', value) // publish a message to anyone in this cluster and this subcluster
     *    sub.on('foo', () => {}) // read streamed messages from anyone in this cluster and this subcluster
     *
     *    sub.on('#join', peer => { // subscribe to a specific peer in the subcluster
     *      peer.emit('foo', value) // send an event named 'foo' only to peer
     *
     *      peer.on('foo', value => { // listen for events named 'foo' only from peer
     *        console.log(value)
     *      })
     *    })
     * ```
     */
    bus.subcluster = async (options = {}) => {
      if (!options.sharedKey) throw new Error('expected options.sharedKey to be of type Uint8Array')

      const keys = await Encryption.createKeyPair(options.sharedKey)
      const subclusterId = Buffer.from(keys.publicKey).toString('base64')

      if (bus.subclusters.has(subclusterId)) return bus.subclusters.get(subclusterId)

      const sub = new events.EventEmitter()
      bus.subclusters.set(subclusterId, sub)

      sub._emit = sub.emit
      sub._on = sub.on
      sub.peers = new Map()
      sub.peerId = _peer.peerId
      sub.subclusterId = subclusterId
      sub.sharedKey = options.sharedKey
      sub.keys = keys

      sub.emit = async (eventName, ...args) => {
        return await _peer.publish(sub.sharedKey, await pack(sub, eventName, ...args))
      }

      sub.on = async (eventName, cb) => {
        if (eventName[0] !== '#') eventName = await sha256(eventName)
        sub._on(eventName, cb)
      }

      bus._on('#disconnection', peer => {
        sub._emit('#leave', peer)
        sub.peers.delete(peer.peerId)
        bus.peers.delete(peer.peerId)
      })

      bus._on('#discovered', () => {
        _peer.join(sub.sharedKey, { subclusterId, ...options })
        sub._emit('#ready', bus.address())
      })

      _peer.join(sub.sharedKey, { subclusterId, ...options })
      return sub
    }

    bus._on('#join', async (packet, peer) => {
      const sub = bus.subclusters.get(packet.subclusterId)
      if (!sub) return

      const ee = getPeerRepresentative(packet, sub, peer)

      ee.emit = async (eventName, ...args) => {
        return peer.write && peer.write(sub.sharedKey, await pack(sub, eventName, ...args))
      }

      ee.on = async (eventName, cb) => {
        if (eventName[0] !== '#') eventName = await sha256(eventName)
        ee._on(eventName, cb)
      }

      if (ee) {
        ee.peerId = peer.peerId
        ee.address = peer.address
        sub._emit('#join', ee, packet)
      }
    })

    bus._on('#packet', async (packet, peer) => {
      const sub = bus.subclusters.get(packet.subclusterId)
      if (!sub) return

      const exists = sub.peers.has(peer.peerId)
      const ee = getPeerRepresentative(packet, sub, peer)
      if (!exists) bus._emit('#join', packet, peer)

      const eventName = packet.usr1.toString('hex')
      const { verified, opened } = await unpack(packet)
      if (verified) packet.verified = true
      sub._emit(eventName, opened, packet)
      ee._emit(eventName, opened, packet)
    })

    bus._on('#stream', async (packet, peer, port, address) => {
      const sub = bus.subclusters.get(packet.subclusterId)
      if (!sub) return

      const exists = sub.peers.has(peer.peerId)
      const ee = getPeerRepresentative(packet, sub, peer)
      if (!exists) bus._emit('#join', packet, peer)

      const eventName = packet.usr1.toString('hex')
      const { verified, opened } = await unpack(packet)
      if (verified) packet.verified = true
      sub._emit(eventName, opened, packet)
      ee._emit(eventName, opened, packet)
    })

    await _peer.init()
    return bus
  }
}
