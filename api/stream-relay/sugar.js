import { wrap, Encryption, sha256, NAT } from './index.js'
import { sodium } from '../crypto.js'
import { Buffer } from '../buffer.js'
import { isBufferLike } from '../util.js'
import { CACHE_TTL } from './packets.js'

export default (dgram, events) => {
  let _peer = null
  let bus = null

  return async (options = {}) => {
    if (bus) return bus

    await sodium.ready
    bus = new events.EventEmitter()
    bus._on = bus.on
    bus._once = bus.once
    bus._emit = bus.emit

    if (!options.indexed) {
      if (!options.clusterId && !options.config?.clusterId) {
        throw new Error('expected options.clusterId to be of type string')
      }

      if (typeof options.signingKeys !== 'object') throw new Error('expected options.signingKeys to be of type Object')
      if (options.signingKeys.publicKey?.constructor.name !== 'Uint8Array') throw new Error('expected options.signingKeys.publicKey to be of type Uint8Array')
      if (options.signingKeys.privateKey?.constructor.name !== 'Uint8Array') throw new Error('expected options.signingKeys.privateKey to be of type Uint8Array')
    }

    let clusterId = bus.clusterId = options.clusterId || options.config?.clusterId

    if (clusterId) clusterId = Buffer.from(clusterId) // some peers don't have clusters

    _peer = new (wrap(dgram))(options) // only one peer per process makes sense

    _peer.onConnection = (packet, ...args) => {
    }

    _peer.onJoin = (packet, ...args) => {
      if (!packet.clusterId.equals(clusterId)) return
      bus._emit('#join', packet, ...args)
    }

    _peer.onPacket = (packet, ...args) => {
      if (!packet.clusterId.equals(clusterId)) return
      bus._emit('#packet', packet, ...args)
    }

    _peer.onStream = (packet, ...args) => {
      if (!packet.clusterId.equals(clusterId)) return
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

    // TODO check if its not a network error
    _peer.onError = (...args) => bus._emit('#error', ...args)

    _peer.onReady = () => {
      _peer.isReady = true
      bus._emit('#ready', bus.address())
    }

    bus.peer = _peer
    bus.peerId = _peer.peerId

    bus.subclusters = new Map()

    bus.address = () => ({
      address: _peer.address,
      port: _peer.port,
      natType: NAT.toString(_peer.natType)
    })

    bus.reconnect = () => {
      _peer.lastUpdate = 0
      _peer.requestReflection()
    }

    bus.disconnect = () => {
      _peer.natType = null
      _peer.reflectionStage = 0
      _peer.reflectionId = null
      _peer.reflectionTimeout = null
      _peer.probeReflectionTimeout = null
    }

    bus.sealMessage = (m, v = options.signingKeys) => _peer.encryption.sealMessage(m, v)
    bus.openMessage = (m, v = options.signingKeys) => _peer.encryption.openMessage(m, v)

    bus.seal = (m, v = options.signingKeys) => _peer.encryption.seal(m, v)
    bus.open = (m, v = options.signingKeys) => _peer.encryption.open(m, v)

    bus.query = (...args) => _peer.query(...args)
    bus.state = () => _peer.getState()
    bus.cacheSize = () => _peer.cache.size
    bus.cacheBytes = () => _peer.cache.bytes

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
      const sub = bus.subclusters.get(packet.subclusterId.toString('base64'))
      if (!sub) return {}

      try {
        opened = _peer.encryption.open(packet.message, packet.subclusterId.toString('base64'))
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

    bus.emit = async (eventName, value, opts = {}) => {
      return await _peer.publish(options.sharedKey, await pack(eventName, value, opts))
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
        opts.subclusterId = opts.subclusterId || subclusterId

        const args = await pack(eventName, value, opts)

        for (const p of sub.peers.values()) {
          await p._peer.write(sub.sharedKey, args)
        }

        return await _peer.publish(sub.sharedKey, args)
      }

      sub.on = async (eventName, cb) => {
        if (eventName[0] !== '#') eventName = await sha256(eventName)
        sub._on(eventName, cb)
      }

      sub.join = () => _peer.join(sub.sharedKey, options)

      bus._on('#ready', () => {
        _peer.join(sub.sharedKey, options)
      })

      _peer.join(sub.sharedKey, options)
      return sub
    }

    bus._on('#join', async (packet, peer) => {
      const sub = bus.subclusters.get(packet.subclusterId.toString('base64'))
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
      const scid = packet.subclusterId.toString('base64')
      const sub = bus.subclusters.get(scid)
      if (!sub) return

      const eventName = packet.usr1.toString('hex')
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
}
