import { Peer, Encryption, sha256 } from './index.js'
import { PeerWorkerProxy } from './proxy.js'
import { sodium } from '../crypto.js'
import { Buffer } from '../buffer.js'
import { isBufferLike } from '../util.js'
import { Packet, CACHE_TTL } from './packets.js'

/**
 * Initializes and returns the network bus.
 *
 * @async
 * @function
 * @param {object} options - Configuration options for the network bus.
 * @param {object} events - A nodejs compatibe implementation of the events module.
 * @param {object} dgram - A nodejs compatible implementation of the dgram module.
 * @returns {Promise<events.EventEmitter>} - A promise that resolves to the initialized network bus.
 */
async function api (options = {}, events, dgram) {
  await sodium.ready
  const bus = new events.EventEmitter()
  bus._on = bus.on
  bus._once = bus.once
  bus._emit = bus.emit

  if (!options.indexed) {
    if (!options.clusterId && !options.config?.clusterId) {
      throw new Error('expected options.clusterId')
    }

    if (typeof options.signingKeys !== 'object') throw new Error('expected options.signingKeys to be of type Object')
    if (options.signingKeys.publicKey?.constructor.name !== 'Uint8Array') throw new Error('expected options.signingKeys.publicKey to be of type Uint8Array')
    if (options.signingKeys.privateKey?.constructor.name !== 'Uint8Array') throw new Error('expected options.signingKeys.privateKey to be of type Uint8Array')
  }

  let clusterId = bus.clusterId = options.clusterId || options.config?.clusterId

  if (clusterId) clusterId = Buffer.from(clusterId) // some peers don't have clusters

  const Ctor = globalThis.isSocketRuntime ? PeerWorkerProxy : Peer
  const _peer = new Ctor(options, dgram)

  _peer.onJoin = (packet, ...args) => {
    if (!Buffer.from(packet.clusterId).equals(clusterId)) return
    bus._emit('#join', packet, ...args)
  }

  _peer.onPacket = (packet, ...args) => {
    if (!Buffer.from(packet.clusterId).equals(clusterId)) return
    bus._emit('#packet', packet, ...args)
  }

  _peer.onStream = (packet, ...args) => {
    if (!Buffer.from(packet.clusterId).equals(clusterId)) return
    bus._emit('#stream', packet, ...args)
  }

  _peer.onData = (...args) => bus._emit('#data', ...args)
  _peer.onSend = (...args) => bus._emit('#send', ...args)
  _peer.onFirewall = (...args) => bus._emit('#firewall', ...args)
  _peer.onMulticast = (...args) => bus._emit('#multicast', ...args)
  _peer.onJoin = (...args) => bus._emit('#join', ...args)
  _peer.onSync = (...args) => bus._emit('#sync', ...args)
  _peer.onSyncStart = (...args) => bus._emit('#sync-start', ...args)
  _peer.onSyncEnd = (...args) => bus._emit('#sync-end', ...args)
  _peer.onConnection = (...args) => bus._emit('#connection', ...args)
  _peer.onDisconnection = (...args) => bus._emit('#disconnection', ...args)
  _peer.onQuery = (...args) => bus._emit('#query', ...args)
  _peer.onNat = (...args) => bus._emit('#network-change', ...args)
  _peer.onWarn = (...args) => bus._emit('#warning', ...args)
  _peer.onState = (...args) => bus._emit('#state', ...args)
  _peer.onConnecting = (...args) => bus._emit('#connecting', ...args)
  _peer.onConnection = (...args) => bus._emit('#connection', ...args)

  // TODO check if its not a network error
  _peer.onError = (...args) => bus._emit('#error', ...args)

  _peer.onReady = info => {
    Object.assign(_peer, {
      isReady: true,
      ...info
    })

    bus._emit('#ready', info)
  }

  bus.subclusters = new Map()

  /**
   * Gets general, read only information of the network peer.
   *
   * @function
   * @returns {object} - The general information.
   */
  bus.getInfo = () => _peer.getInfo()

  /**
   * Gets the read only state of the network peer.
   *
   * @function
   * @returns {object} - The address information.
   */
  bus.getState = () => _peer.getState()

  /**
   * Indexes a new peer in the network.
   *
   * @function
   * @param {object} params - Peer information.
   * @param {string} params.peerId - The peer ID.
   * @param {string} params.address - The peer address.
   * @param {number} params.port - The peer port.
   * @throws {Error} - Throws an error if required parameters are missing.
   */
  bus.addIndexedPeer = ({ peerId, address, port }) => {
    return _peer.addIndexedPeer({ peerId, address, port })
  }

  bus.close = () => _peer.close()
  bus.sync = (peerId) => _peer.sync(peerId)
  bus.reconnect = () => _peer.reconnect()
  bus.disconnect = () => _peer.disconnect()

  bus.sealUnsigned = (m, v = options.signingKeys) => _peer.sealUnsigned(m, v)
  bus.openUnsigned = (m, v = options.signingKeys) => _peer.openUnsigned(m, v)

  bus.seal = (m, v = options.signingKeys) => _peer.seal(m, v)
  bus.open = (m, v = options.signingKeys) => _peer.open(m, v)

  bus.query = (...args) => _peer.query(...args)

  const pack = async (eventName, value, opts = {}) => {
    if (typeof eventName !== 'string') throw new Error('event name must be a string')
    if (eventName.length === 0) throw new Error('event name too short')

    if (opts.ttl) opts.ttl = Math.min(opts.ttl, CACHE_TTL)

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

    args.usr2 = Buffer.from(options.signingKeys.publicKey)
    args.sig = Encryption.sign(args.message, options.signingKeys.privateKey)

    return args
  }

  const unpack = async packet => {
    let opened
    let verified
    const scid = Buffer.from(packet.subclusterId).toString('base64')
    const sub = bus.subclusters.get(scid)
    if (!sub) return {}

    try {
      opened = await _peer.open(packet.message, scid)
    } catch (err) {
      sub._emit('warning', err)
      return {}
    }

    if (packet.sig) {
      try {
        if (Encryption.verify(opened.data || opened, packet.sig, packet.usr2)) {
          verified = true
        }
      } catch (err) {
        sub._emit('warning', err)
        return {}
      }
    }

    return { opened, verified }
  }

  /**
   * Publishes an event to the network bus.
   *
   * @async
   * @function
   * @param {string} eventName - The name of the event.
   * @param {any} value - The value associated with the event.
   * @param {object} opts - Additional options for publishing.
   * @returns {Promise<any>} - A promise that resolves to the published event details.
   */
  bus.emit = async (eventName, value, opts = {}) => {
    const args = await pack(eventName, value, opts)
    if (!options.sharedKey) {
      throw new Error('Can\'t emit to the top level cluster, a shared key was not provided in the constructor or the arguments options')
    }

    return await _peer.publish(options.sharedKey || opts.sharedKey, args)
  }

  bus.on = async (eventName, cb) => {
    if (eventName[0] !== '#') eventName = await sha256(eventName)
    bus._on(eventName, cb)
  }

  bus.subcluster = async (options = {}) => {
    if (!options.sharedKey?.constructor.name) {
      throw new Error('expected options.sharedKey to be of type Uint8Array')
    }

    const derivedKeys = await Encryption.createKeyPair(options.sharedKey)
    const subclusterId = Buffer.from(derivedKeys.publicKey)
    const scid = subclusterId.toString('base64')

    if (bus.subclusters.has(scid)) return bus.subclusters.get(scid)

    const sub = new events.EventEmitter()
    sub._emit = sub.emit
    sub._on = sub.on
    sub.peers = new Map()

    bus.subclusters.set(scid, sub)

    sub.peerId = _peer.peerId
    sub.subclusterId = subclusterId
    sub.sharedKey = options.sharedKey
    sub.derivedKeys = derivedKeys

    sub.emit = async (eventName, value, opts = {}) => {
      opts.clusterId = opts.clusterId || clusterId
      opts.subclusterId = opts.subclusterId || sub.subclusterId

      const args = await pack(eventName, value, opts)

      if (sub.peers.values().length) {
        let packets = []

        for (const p of sub.peers.values()) {
          const r = await p._peer.write(sub.sharedKey, args)
          if (packets.length === 0) packets = r
        }

        for (const packet of packets) {
          const p = Packet.from(packet)
          const pid = Buffer.from(packet.packetId).toString('hex')
          _peer.cache.insert(pid, p)

          _peer.unpublished[pid] = Date.now()
          if (globalThis.navigator && !globalThis.navigator.onLine) continue

          _peer.mcast(packet)
        }
        return packets
      } else {
        const packets = await _peer.publish(sub.sharedKey, args)
        return packets
      }
    }

    sub.on = async (eventName, cb) => {
      if (eventName[0] !== '#') eventName = await sha256(eventName)
      sub._on(eventName, cb)
    }

    sub.off = async (eventName, fn) => {
      if (eventName[0] !== '#') eventName = await sha256(eventName)
      sub.removeListener(eventName, fn)
    }

    sub.join = () => _peer.join(sub.sharedKey, options)

    bus._on('#ready', () => {
      const scid = Buffer.from(sub.subclusterId).toString('base64')
      const subcluster = bus.subclusters.get(scid)
      if (subcluster) _peer.join(subcluster.sharedKey, options)
    })

    _peer.join(sub.sharedKey, options)
    return sub
  }

  bus._on('#join', async (packet, peer) => {
    const scid = Buffer.from(packet.subclusterId).toString('base64')
    const sub = bus.subclusters.get(scid)
    if (!sub) return

    let ee = sub.peers.get(peer.peerId)

    if (!ee) {
      ee = new events.EventEmitter()

      ee._on = ee.on
      ee._emit = ee.emit

      ee.peerId = peer.peerId
      ee.address = peer.address
      ee.port = peer.port

      ee.emit = async (eventName, value, opts = {}) => {
        if (!ee._peer.write) return

        opts.clusterId = opts.clusterId || clusterId
        opts.subclusterId = opts.subclusterId || sub.subclusterId

        const args = await pack(eventName, value, opts)
        return peer.write(sub.sharedKey, args)
      }

      ee.on = async (eventName, cb) => {
        if (eventName[0] !== '#') eventName = await sha256(eventName)
        ee._on(eventName, cb)
      }
    }

    const oldPeer = sub.peers.has(peer.peerId)
    const portChange = oldPeer.port !== peer.port
    const addressChange = oldPeer.address !== peer.address
    const natChange = oldPeer.natType !== peer.natType
    const change = portChange || addressChange || natChange

    ee._peer = peer

    sub.peers.set(peer.peerId, ee)
    if (!oldPeer || change) sub._emit('#join', ee, packet)
  })

  const handlePacket = async (packet, peer, port, address) => {
    const scid = Buffer.from(packet.subclusterId).toString('base64')
    const sub = bus.subclusters.get(scid)
    if (!sub) return

    const eventName = Buffer.from(packet.usr1).toString('hex')
    const { verified, opened } = await unpack(packet)
    if (verified) packet.verified = true

    sub._emit(eventName, opened, packet)

    const ee = sub.peers.get(packet.streamFrom || peer?.peerId)
    if (ee) ee._emit(eventName, opened, packet)
  }

  bus._on('#stream', handlePacket)
  bus._on('#packet', handlePacket)

  bus._on('#disconnection', peer => {
    for (const sub of bus.subclusters) {
      sub._emit('#leave', peer)
      sub.peers.delete(peer.peerId)
    }
  })

  await _peer.init()
  return bus
}

export { api }
export default api
