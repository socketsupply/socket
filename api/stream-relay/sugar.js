import { wrap, Encryption, sha256, NAT } from './index.js'
import { sodium } from '../crypto.js'
import { Buffer } from '../buffer.js'
import { isBufferLike } from '../util.js'
import { PacketStream } from './packets.js'

export default (dgram, events) => {
  let peer = null
  let bus = null

  /*
   * Creates an instance of the underlying network protocol (this can only be
   * called once, subsequent calls will return the same instance).
   * The method returned method should be exposed to the user of the module.
   *
   * @param {object} - options
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

    if (!options.indexed && !options.clusterId) {
      throw new Error('expected .clusterId property')
    }

    const clusterId = bus.clusterId = options.clusterId

    peer = new (wrap(dgram))(options)

    peer.onPacket = (...args) => setTimeout(() => bus.emit('#packet', ...args))
    peer.onData = (...args) => setTimeout(() => bus.emit('#data', ...args))
    peer.onSend = (...args) => setTimeout(() => bus.emit('#send', ...args))
    peer.onMulticast = (...args) => setTimeout(() => bus.emit('#multicast', ...args))
    peer.onStream = (...args) => setTimeout(() => bus.emit('#stream', ...args))
    peer.onConnection = (...args) => setTimeout(() => bus.emit('#connection', ...args))
    peer.onDisconnection = (...args) => bus.emit('#disconnection', ...args)
    peer.onQuery = (...args) => setTimeout(() => bus.emit('#query', ...args))
    peer.onNat = (...args) => bus.emit('#network-change', ...args)
    peer.onError = (...args) => bus.emit('#error', ...args)
    peer.onState = (...args) => bus.emit('#state', ...args)
    peer.onConnecting = (...args) => bus.emit('#connecting', ...args)

    peer.onReady = () => {
      peer.isReady = true
      bus.emit('#ready')
    }

    bus.discovered = new Promise(resolve => bus.once('#ready', resolve))
    bus.peer = peer

    bus.setMaxListeners(1024)
    bus.subclusters = new Map()

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
     *```js
     *    import fs from '../fs.js'
     *    import { network, Encryption } from '../network.js'
     *
     *    const oldState = parse(fs.readFile('state.json'))
     *
     *    const sharedKey = await Encryption.createSharedKey()
     *
     *    const socket = network(oldState)
     *    const cats = await socket.subcluster({ sharedKey })
     *
     *    console.log(cats.sharedKey)
     *
     *    cats.emit('mew', value)
     *
     *    cats.on('mew', value => {
     *      console.log(value)
     *    })
     *
     *    cats.on('#connection', cat => {
     *      cat.emit('mew', value)
     *
     *      cat.on('mew', value => {
     *        console.log(value)
     *      })
     *    })
     *
     *    fs.writeFile('state.json', stringify(socket.getState()))
     *```
     */
    bus.subcluster = async (options = {}) => {
      const keys = await Encryption.createKeyPair(options.sharedKey)
      const subclusterId = Buffer.from(keys.publicKey).toString('base64')

      if (bus.subclusters.has(subclusterId)) return bus.subclusters.get(subclusterId)

      const selfEvents = new events.EventEmitter()
      const emitSelf = selfEvents.emit
      const onSelf = selfEvents.on

      selfEvents.sharedKey = options.sharedKey
      selfEvents.keys = keys

      peer.encryption.add(keys.publicKey, keys.privateKey)
      bus.subclusters.set(subclusterId, selfEvents)

      selfEvents.subclusterId = subclusterId
      selfEvents.peers = new Map()
      selfEvents.peerId = peer.peerId

      selfEvents.on = async (eventName, cb) => {
        if (eventName[0] !== '#') eventName = await sha256(eventName)
        onSelf.call(selfEvents, eventName, cb)
      }

      const onBeforeEmit = async (eventName, value, opts) => {
        if (typeof eventName !== 'string') throw new Error('event name must be a string')
        if (eventName.length === 0) throw new Error('event name too short')

        const args = {
          clusterId,
          subclusterId,
          usr1: await sha256(eventName, { bytes: true })
        }

        if (isBufferLike(value) || typeof value === 'string') {
          args.message = value
        } else {
          try {
            args.message = JSON.stringify(value)
          } catch (err) {
            return bus.emit('error', err)
          }
        }

        args.message = Buffer.from(args.message)

        if (opts.publicKey && opts.privateKey) {
          args.usr2 = Buffer.from(opts.publicKey)
          args.sig = Buffer.from(Encryption.sign(args.message, Buffer.from(opts.privateKey)))
        }

        args.previousId = opts.previousId
        return args
      }

      selfEvents.emit = async (...args) => {
        return await peer.publish(options.sharedKey, await onBeforeEmit(...args))
      }

      bus.on('#connection', (packet, p) => {
        if (selfEvents.peers.has(p.peerId)) return

        const peerEvents = selfEvents.peers.get(p.peerId) || new events.EventEmitter()
        const emitPeer = peerEvents.emit
        const onPeer = peerEvents.on

        const handleStreamedPacket = (packet, p) => {
          if (packet?.subclusterId !== subclusterId) return
          if (packet?.streamFrom !== p.peerId) return
          if (packet?.streamTo !== peer.peerId) return

          const eventName = packet.usr1.toString('hex')
          let opened

          try {
            opened = peer.encryption.open(packet.message, packet.subclusterId)
          } catch (err) {
            return emitPeer.call(peerEvents, '#error', err)
          }

          if (packet.sig) {
            try {
              if (Encryption.verify(opened, packet.sig, packet.usr2)) {
                packet.verified = true
              }
            } catch {}
          }

          emitPeer.call(peerEvents, eventName, opened, packet)
        }

        if (!selfEvents.peers.has(p.peerId)) {
          peerEvents.peerId = p.peerId

          selfEvents.peers.set(p.peerId, peerEvents)

          peerEvents.on = async (eventName, cb) => {
            if (eventName[0] !== '#') eventName = await sha256(eventName)
            onPeer.call(peerEvents, eventName, cb)
          }

          peerEvents.emit = async (...args) => {
            return p.write(options.sharedKey, await onBeforeEmit(...args))
          }

          bus.once('#disconnection', (packet, peer) => {
            emitSelf.call(selfEvents, '#disconnection', peerEvents)
          })

          bus.on('#stream', handleStreamedPacket)
        }

        emitSelf.call(selfEvents, '#connection', peerEvents, packet)

        if (packet.type === PacketStream.type) {
          queueMicrotask(() => handleStreamedPacket(packet, p))
        }
      })

      bus.on('#stream', (packet, p) => {
        if (selfEvents.peers.has(p.peerId)) return
        bus.emit('#connection', packet, p)
      })

      bus.on('#query', (packet, ...args) => {
        emitSelf.call(selfEvents, '#query', packet)
      })

      bus.on('#network-change', (packet, ...args) => {
        emitSelf.call(selfEvents, '#network-change', packet)
      })

      bus.on('#state', (packet, ...args) => {
        emitSelf.call(selfEvents, '#state', packet)
      })

      bus.on('#error', (...args) => {
        emitSelf.call(selfEvents, '#error', ...args)
      })

      bus.on('#packet', (packet, ...args) => {
        if (packet.subclusterId !== subclusterId) return
        const eventName = packet.usr1.toString('hex')

        let opened

        try {
          opened = peer.encryption.open(packet.message, packet.subclusterId)
        } catch (err) {
          return emitSelf.call(selfEvents, '#error', err)
        }

        if (packet.sig) {
          try {
            if (Encryption.verify(opened, packet.sig, packet.usr2)) {
              packet.verified = true
            }
          } catch {}
        }
        emitSelf.call(selfEvents, eventName, opened, packet)
      })

      bus.on('#data', (packet, port, address) => {
        const p = peer.peers.find(p => p.port === port && p.address === address)
        if (!p) return

        if (!p.received) p.received = 0
        p.received += 1
      })

      bus.on('#ready', () => {
        peer.join(options.sharedKey, { subclusterId, ...options })
      })

      return selfEvents
    }

    bus.ready = () => {
      peer.requestReflection()
      if (peer.isReady) bus.emit('#ready')
    }

    bus.getState = () => {
      return peer.getState()
    }

    bus.natType = () => {
      return NAT.toString(peer.natType)
    }

    bus.cacheSize = () => {
      return peer.cache.size
    }

    bus.open = packet => {
      return peer.encryption.open(packet.message, packet.subclusterId)
    }

    await peer.init()

    return bus
  }
}
