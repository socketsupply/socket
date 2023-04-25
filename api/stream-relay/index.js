/**
 * @module stream-relay
 * @status Experimental
 *
 * This module provides primitives for constructing a distributed network
 * for relaying messages between peers. Peers can be addressed by their unique
 * peer ID and can also be directly connected to by their IP address and port.
 *
 * Note: The code in the module may change a lot in the next few weeks but the
 * API will continue to be backward compatible thoughout all future releases.
 */

import { isBufferLike } from '../util.js'
import { Buffer } from '../buffer.js'
import { sodium } from '../crypto.js'

import { Encryption } from './encryption.js'
import { Cache } from './cache.js'
import { PTP } from './scheme-ptp.js'

import {
  Packet,
  PacketPing,
  PacketPong,
  PacketIntro,
  PacketAsk,
  PacketAnswer,
  PacketPublish,
  PacketJoin,
  PacketQuery,
  addHops,
  sha256,
  VERSION
} from './packets.js'

export { sha256, Cache, Encryption }

/**
 * Retry delay in milliseconds for ping.
 * @type {number}
 */
export const PING_RETRY = 500

/**
 * Default port to bind peer UDP socket port to.
 * @type {number}
 */
export const DEFAULT_PORT = 9777

/**
 * Default static test port to bind peer UDP socket port to.
 * @type {number}
 */
export const DEFAULT_TEST_PORT = 9778

/**
 * Default keep alive timeout.
 * @type {number}
 */
export const DEFAULT_KEEP_ALIVE = 28_000

/**
 * Default rate limit threshold in milliseconds.
 * @type {number}
 */
export const DEFAULT_RATE_LIMIT_THRESHOLD = 8000

/**
 * Creates a factory function for generating a port to bind to.
 * @param {number?} [localPort] Initial local port
 * @param {number?} [testPort] Initial static test port
 * @return {function(object?, number?): number}
 */
export const createPortGenerator = (localPort = 0, testPort = 0) => {
  const portCache = { [localPort || 0]: true, [testPort || 0]: true }
  const next = () => ~~(Math.random() * 0xffff) || 1

  /**
   * Port generator factory function.
   * @param {object?} ports
   * @param {number?} p
   * @return {number}
   */
  return (ports = portCache, p = null) => {
    do ports[p = next()] = true; while (!ports[p] && p)
    return p
  }
}

/**
 * Computes rate limit predicate value for a port and address pair for a given threshold
 * updating an input rates map.
 * @param {Map} rates
 * @param {number} port
 * @param {string} address
 * @param {number?} [threshold = DEFAULT_RATE_LIMIT_THRESHOLD]
 * @return {boolean}
 */
export function rateLimit (rates, port, address, threshold = DEFAULT_RATE_LIMIT_THRESHOLD) {
  const now = Date.now()
  const key = address + ':' + port
  const rate = rates.get(key) || { d: 0, n: 0, e: 0, t: now }
  const quota = 1024

  rate.n++
  rate.e = Math.pow(2, rate.n)
  rate.d = now - rate.t
  rate.t = now

  if (!rates.has(key)) {
    rates.set(key, rate)
  }

  if (rate.e > quota) {
    if (rate.d < threshold) {
      return false
    }

    rates.delete(key)
  }

  return true
}

/**
 * A `RemotePeer` represents an initial, discovered, or connected remote peer.
 * Typically, you will not need to create instances of this class directly.
 */
export class RemotePeer {
  peerId = null
  address = null
  port = 0
  natType = null
  clusterId = null
  pingId = null
  distance = 0
  publicKey = null
  privateKey = null
  clock = 0
  lastUpdate = 0
  lastRequest = 0

  /**
   * `RemotePeer` class constructor.
   * @param {{
   *   peerId?: string,
   *   address?: string,
   *   port?: number,
   *   natType?: string,
   *   clusterId?: string,
   *   pingId?: string,
   *   distance?: number,
   *   publicKey?: string,
   *   privateKey?: string,
   *   clock?: number,
   *   lastUpdate?: number,
   *   lastRequest?: number
   * }} o
   */
  constructor (o) {
    if (!o.port) throw new Error('expected .port')
    if (!o.address) throw new Error('expected .address')
    if (!o.peerId) throw new Error('expected .peerId')

    if (o.clusterId) this.clusterId = o.clusterId
    if (!o.clusterId) this.natType = 'static'

    if (o.scheme === 'PTP') {
      PTP.init(this, o, 'remote')
    } else {
      if (o.publicKey) this.publicKey = o.publicKey
      if (o.privateKey) this.privateKey = o.privateKey
    }

    Object.assign(this, o)
  }
}

/**
 * `Peer` class factory.
 * @param {{ createSocket: function('udp4', object?): object }} options
 */
export const createPeer = options => {
  const { createSocket } = options
  class Peer {
    port = null
    address = null
    natType = null
    clusterId = null
    reflectionId = null
    reflectionFirstResponder = null
    peerId = ''
    pingStart = 0
    ctime = Date.now()
    lastUpdate = 0
    closing = false
    listening = false
    unpublished = {}
    cache = null
    queries = {}
    joins = {}
    connects = {}
    uptime = 0
    maxHops = 16
    bdpCache = /** @type {number[]} */ ([])

    onListening = null
    onDelete = null

    firewall = null

    peers = JSON.parse(/* snapshot_start=1682350677046, filter=easy,static */`[
      {"address":"44.213.42.133","port":10885,"peerId":"4825fe0475c44bc0222e76c5fa7cf4759cd5ef8c66258c039653f06d329a9af5"},
      {"address":"107.20.123.15","port":31503,"peerId":"2de8ac51f820a5b9dc8a3d2c0f27ccc6e12a418c9674272a10daaa609eab0b41"},
      {"address":"54.227.171.107","port":43883,"peerId":"7aa3d21ceb527533489af3888ea6d73d26771f30419578e85fba197b15b3d18d"},
      {"address":"44.213.42.133","port":10885,"peerId":"4825fe0475c44bc0222e76c5fa7cf4759cd5ef8c66258c039653f06d329a9af5"},
      {"address":"107.20.123.15","port":31503,"peerId":"2de8ac51f820a5b9dc8a3d2c0f27ccc6e12a418c9674272a10daaa609eab0b41"},
      {"address":"54.227.171.107","port":43883,"peerId":"7aa3d21ceb527533489af3888ea6d73d26771f30419578e85fba197b15b3d18d"},
      {"address":"54.157.134.116","port":34420,"peerId":"1d2315f6f16e5f560b75fbfaf274cad28c12eb54bb921f32cf93087d926f05a9"},
      {"address":"184.169.205.9","port":52489,"peerId":"db00d46e23d99befe42beb32da65ac3343a1579da32c3f6f89f707d5f71bb052"},
      {"address":"35.158.123.13","port":31501,"peerId":"4ba1d23266a2d2833a3275c1d6e6f7ce4b8657e2f1b8be11f6caf53d0955db88"},
      {"address":"35.160.18.99","port":11205,"peerId":"2bad1ff23dd5e0e32d3d85e1d9bb6324fef54fec0224c4f132066ad3d0096a35"},
      {"address":"3.77.22.13","port":40501,"peerId":"1998b9e5e0b05ae7865545499da298d2064afe07c36187fd4047c603a4ac2d7"},
      {"address":"3.68.89.3","port":22787,"peerId":"448b083bd8a495ce684d5837359ce69d0ff8a5a844efe18583ab000c99d3a0ff"},
      {"address":"3.76.100.161","port":25761,"peerId":"07bffa90d89bf74e06ff7f83938b90acb1a1c5ce718d1f07854c48c6c12cee49"},
      {"address":"3.70.241.230","port":61926,"peerId":"1d7ee8d965794ee286ac425d060bab27698a1de92986dc6f4028300895c6aa5c"},
      {"address":"3.70.160.181","port":41141,"peerId":"707c07171ac9371b2f1de23e78dad15d29b56d47abed5e5a187944ed55fc8483"},
      {"address":"3.122.250.236","port":64236,"peerId":"a830615090d5cdc3698559764e853965a0d27baad0e3757568e6c7362bc6a12a"},
      {"address":"18.130.98.23","port":25111,"peerId":"ba483c1477ab7a99de2d9b60358d9641ff6a6dc6ef4e3d3e1fc069b19ac89da4"},
      {"address":"18.129.76.26","port":9210,"peerId":"099b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"13.230.254.25","port":65049,"peerId":"592e9fd43abcf854fa5cc9a203bebd15afb2e900308044b77599cd8f46f2532d"},
      {"address":"18.138.72.20","port":2909,"peerId":"199b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"18.229.140.216","port":36056,"peerId":"73d726c04c05fb3a8a5382e7a4d7af41b4e1661aadf9020545f23781fefe3527"}
    ]`/* snapshot_end=1682350693060 */).map((/** @type {object} */ o) => new RemotePeer(o))

    /**
     * Factory for creating a `Peer` instances. This function ensures all
     * internal dependencies are loaded and ready.
     * @param {object?} [options]
     * @return {Promise<Peer>}
     */
    static async create (options) {
      await sodium.ready
      return new this(options)
    }

    /**
     * `Peer` class constructor.
     * @private
     * @param {object?} [persistedState]
     */
    constructor (persistedState = {}) {
      const config = persistedState?.config ?? persistedState ?? {}

      this.encryption = new Encryption(config.keys)

      if (config.scheme === 'PTP') PTP.init(this, config, 'local')
      if (!config.peerId) config.peerId = this.encryption.publicKey

      this.config = {
        keepalive: DEFAULT_KEEP_ALIVE,
        lastGroupBroadcast: -1,
        clock: 0,
        ...config
      }

      let cacheData

      if (persistedState?.data?.length > 0) {
        cacheData = new Map(persistedState.data)
      }

      this.cache = new Cache(cacheData, config.siblingResolver)

      this.unpublished = persistedState?.unpublished || {}
      this.onError = (/** @type {Error} */ err) => { console.error(err) }

      Object.assign(this, config)

      this.port = config.port || null
      this.natType = config.natType || null
      this.address = config.address || null

      this.socket = createSocket('udp4', this)
      this.testSocket = createSocket('udp4', this)
    }

    init () {
      return new Promise(resolve => {
        this.socket.on('listening', () => {
          this.listening = true
          this.config.port = this.socket.address().port

          this.testSocket.on('listening', () => {
            this.config.testPort = this.testSocket.address().port
            if (this.onListening) this.onListening()
            this.requestReflection()
            resolve()
          })
          this.testSocket.bind(this.config.testPort || 0)
        })

        this.socket.on('message', (...args) => this.onMessage(...args))
        this.socket.on('error', (...args) => this.onError(...args))
        this.testSocket.on('message', (...args) => this.onMessage(...args))
        this.testSocket.on('error', (...args) => this.onError(...args))

        this.socket.setMaxListeners(2048)
        this.testSocket.setMaxListeners(2048)

        this.socket.bind(this.config.port || 0)

        globalThis.window?.addEventListener('online', async () => {
          await this.join()

          setTimeout(async () => {
            for (const [packetId, clusterId] of Object.entries(this.unpublished)) {
              const packet = await this.cache.get(packetId)
              if (packet) this.mcast(packet, packetId, clusterId)
            }
          }, 1024)
        })

        const keepaliveHandler = async ts => {
          if (this.closing) return false // cancel timer

          if ((this.pingStart + this.config.keepalive) < Date.now()) {
            this.pingStart = 0
            this.reflectionFirstResponder = null
          }

          this.uptime += this.config.keepalive

          const offline = globalThis.navigator && !globalThis.navigator.onLine
          if (!offline) this.requestReflection()

          if (this.onInterval) this.onInterval()

          for (const [k, packet] of [...this.cache.data]) {
            const TTL = Packet.ttl

            if (packet.timestamp + TTL < ts) {
              await this.mcast(packet, packet.packetId, packet.clusterId, true)
              await this.cache.delete(k)
              if (this.onDelete) this.onDelete(packet)
            }
          }

          for (const k of Object.keys(this.queries)) {
            if (--this.queries[k] === 0) delete this.queries[k]
          }

          for (const k of Object.keys(this.joins)) {
            if (--this.joins[k] === 0) delete this.joins[k]
          }

          for (const k of Object.keys(this.connects)) {
            if (--this.connects[k] === 0) delete this.connects[k]
          }

          // debug('PING HART', this.peerId, this.peers)
          for (const [i, peer] of Object.entries(this.peers)) {
            if (peer.clusterId) {
              const then = Date.now() - peer.lastUpdate

              if (then >= this.config.keepalive) {
                this.peers.splice(i, 1)
                continue
              }
            }

            this.ping(peer, false, {
              clusterId: this.clusterId,
              peerId: this.peerId,
              natType: this.natType,
              cacheSize: this.cache.size,
              isHeartbeat: true
            })
          }
        }

        this.timer(0, this.config.keepalive, keepaliveHandler)
      })
    }

    async send (data, ...args) {
      try {
        await this.socket.send(data, ...args)
        return true
      } catch (err) {
        this.onError(new Error('ESEND'), ...args)
        return false
      }
    }

    getState () {
      return {
        config: this.config,
        data: [...this.cache.data],
        unpublished: this.unpublished
      }
    }

    getPeers (packet, peers, n = 3) {
      let list = peers.filter(p => p.peerId !== this.peerId)

      // if our peer is behind a hard NAT, filter out peers with the same NATs
      if (this.natType === 'hard') {
        list = list.filter(p => p.natType !== 'hard' || !p.natType)
      }

      const candidates = list
        .filter(p => p.natType !== null) // some may not be known yet
        .sort((a, b) => a.distance - b.distance)
        .slice(0, 16)
        .sort(() => Math.random() - 0.5)
        .slice(0, n)

      const members = list.filter(p => p.clusterId === this.clusterId)

      if (members.length) {
        if (candidates.length === n) {
          candidates[0] = members[0]
        } else {
          candidates.push(members[0])
        }
      }

      return candidates
    }

    // Packets MAY be addressed directly to a peer using IP address and port,
    // but should never assume that any particular peer will be available.
    // Instead, packets should be multicast (not broadcast) to the network.
    async mcast (packet, packetId, clusterId, isTaxed, ignorelist = []) {
      let list = this.peers

      if (Array.isArray(packet.message?.history)) {
        ignorelist = [...ignorelist, packet.message.history]
      }

      if (ignorelist.length) {
        list = list.filter(p => !ignorelist.find(peer => {
          return p.address === peer.address && p.port === peer.port
        }))
      }

      const peers = this.getPeers(packet, list)

      for (const peer of peers) {
        this.timer(10, 0, async () => {
          const data = await Packet.encode(packet)

          if (await this.send(isTaxed ? addHops(data) : data, peer.port, peer.address)) {
            delete this.unpublished[packetId]
          }
        })
      }
    }

    timer (delay, repeat, fn) {
      if (this.closing) return false

      let tid = null
      const interval = () => {
        if (fn(Date.now()) === false) clearInterval(tid)
      }

      if (!delay && fn(Date.now()) !== false && repeat) {
        tid = setInterval(interval, repeat)
        return
      }

      setTimeout(() => {
        if (fn(Date.now()) === false) return
        if (!repeat) return
        tid = setInterval(interval, repeat)
      }, delay)
    }

    requestReflection (reflectionId) {
      // this.reflectionId will have to get unassigned at some point to allow this to try again
      if (this.closing || !this.clusterId || this.pingStart > 0) return

      // get two random easy peers from the known peers and send pings, we can
      // learn this peer's nat type if we have at least two Pongs from peers
      // that are outside of this peer's NAT.
      const peers = this.peers.filter(p => p.natType !== 'static' || p.natType !== 'easy')

      const [peer1, peer2] = peers.sort(() => Math.random() - 0.5)

      const peerId = this.peerId
      const clusterId = this.clusterId
      const pingId = Math.random().toString(16).slice(2)
      const testPort = this.config.testPort

      this.pingStart = Date.now()

      // debug(`  requestReflection peerId=${peerId}, pingId=${pingId}`)
      this.ping(peer1, true, { peerId, pingId, isReflection: true, clusterId })
      this.ping(peer2, true, { peerId, pingId, isReflection: true, clusterId, testPort })
    }

    async ping (peer, withRetry, props = {}) {
      if (!peer) return

      const p = {
        message: {
          isHeartbeat: props.heartbeat === true,
          peerId: peer.peerId, // potentially verify this
          ...props
        }
      }

      const packet = new PacketPing(p)
      const data = await Packet.encode(packet)

      const retry = async () => {
        if (this.closing) return false

        const p = this.peers.find(p => p.peerId === peer.peerId)
        if (p && p.pingId === packet.message.pingId) return false
        await this.send(data, peer.port, peer.address)
        if (p) p.lastRequest = Date.now()
      }

      await retry()

      if (withRetry) {
        this.timer(PING_RETRY, 0, retry)
        this.timer(PING_RETRY * 3, 0, retry)
      }
      return packet
    }

    setPeer (packet, port, address) {
      if (this.address === address && this.port === port) return this
      const { peerId, clusterId, natType } = packet.message

      const existingPeer = this.peers.find(p => p.peerId === peerId)
      if (existingPeer?.clock > packet.clock) return

      const newPeer = {
        peerId,
        port,
        address,
        lastUpdate: Date.now(),
        clock: packet.clock,
        distance: 0
      }

      if (natType) newPeer.natType = natType

      if (!existingPeer) {
        if (clusterId) newPeer.clusterId = clusterId
        try {
          const peer = new RemotePeer(newPeer)

          if (this.peers.length > 4096) {
            const oldest = [...this.peers].sort((a, b) => {
              return a.lastUpdate - b.lastUpdate
            })
            const index = this.peers.find(p => p.peerId === oldest.peerId)
            this.peers.splice(index, 1)
          }

          this.peers.push(peer)
          return peer
        } catch (err) {
          return null
        }
      }

      newPeer.distance = existingPeer.lastRequest - existingPeer.lastUpdate
      Object.assign(existingPeer, newPeer)
      return existingPeer
    }

    //
    // users call this at least once, when their app starts. It will mcast
    // this peer, their latest clusters and starts querying toe network for peers.
    //
    async join () {
      if (!this.port) return

      const packet = new PacketJoin({
        clock: this.config.clock++,
        message: {
          clusterId: this.clusterId,
          peerId: this.peerId,
          natType: this.natType,
          address: this.address,
          port: this.port
        }
      })

      const data = await Packet.encode(packet)
      const sends = []

      for (const peer of this.getPeers(packet, this.peers)) {
        if (peer.address === this.address && peer.port === this.port) continue
        sends.push(this.send(data, peer.port, peer.address))
      }

      await Promise.all(sends)
    }

    async publish (args) {
      let { clusterId, message, to, packet, nextId, meta = {} } = args

      if (!to) throw new Error('.to field required to publish')
      if (typeof to !== 'string') to = Buffer.from(to).toString('base64')

      if (message && typeof message === 'object' && !isBufferLike(message)) {
        try {
          message = Buffer.from(JSON.stringify(message))
        } catch {}
      }

      if (this.prepareMessage) message = this.prepareMessage(message, to)
      else if (this.encryption.has(to)) message = this.encryption.seal(message, to)

      let messages = [message]
      const len = message?.byteLength ?? message?.length ?? 0

      let clock = packet ? packet.clock : 0

      const siblings = [...this.cache.data.values()].filter(p => {
        return p.previousId === packet?.packetId
      })

      if (siblings.length) {
        // if there are siblings of the previous packet
        // pick the highest clock value, the parent packet or the sibling
        const sort = (a, b) => a.clock - b.clock
        const sib = siblings.sort(sort).reverse()[0]
        clock = Math.max(clock, sib.clock) + 1
      }

      if (len > 1024) { // Split packets that have messages bigger than Packet.maxLength
        messages = [{
          meta,
          peerId: this.peerId,
          size: message.length,
          indexes: Math.ceil(message.length / 1024)
        }]
        let pos = 0
        while (pos < message.length) messages.push(message.slice(pos, pos += 1024))
      }

      // turn each message into an actual packet
      const packets = messages.map(message => new PacketPublish({
        to,
        clusterId,
        clock: ++clock,
        message
      }))

      if (packet) packets[0].previousId = packet.packetId
      if (nextId) packets[packets.length - 1].nextId = nextId

      // set the .packetId (any maybe the .previousId and .nextId)
      for (let i = 0; i < packets.length; i++) {
        if (packets.length > 1) packets[i].index = i

        if (i === 0) {
          packets[0].packetId = await sha256(packets[0].message)
        } else {
          // all fragments will have the same previous packetId
          // the index is used to stitch them back together in order.
          packets[i].previousId = packets[0].packetId
        }

        if (packets[i + 1]) {
          packets[i + 1].packetId = await sha256(
            await sha256(packets[i].packetId) +
            await sha256(packets[i + 1].message)
          )
          packets[i].nextId = packets[i + 1].packetId
        }
      }

      for (const packet of packets) {
        const inserted = await this.cache.insert(packet.packetId, packet)

        if (inserted && this.onPacket) {
          this.onPacket(packet, this.port, this.address)
        }

        this.unpublished[packet.packetId] = clusterId
        if (globalThis.navigator && !globalThis.navigator.onLine) continue

        await this.mcast(packet, packet.packetId, clusterId)
      }

      return packets
    }

    async negotiateCache (peer, packet, port, address) {
      const packets = [...this.cache.data.values()].sort((a, b) => a.hops - b.hops)

      // the peer has no data, don't ask them anything, tell them everything
      if (packet.message?.cacheSize === 0) {
        return Promise.all(packets.map(async (p, i) => {
          return this.send(await Packet.encode(p), port, address)
        }))
      }

      // tell the other peer about what we know we need for sure.

      const toSend = [] // don't send tails if they are also heads

      // a tail is a packet with a .previousId that we don't have, the purpose
      // of sending it is to request packets that came before these.
      this.cache.tails((p) => {
        toSend.push(new PacketAsk({
          clusterId: p.clusterId,
          message: {
            previousId: p.previousId,
            peerId: this.peerId
          }
        }))
      })

      // a head is the last packet we know about in the chain, the purpose
      // of sending it is to ask for packets that might come after these.
      this.cache.heads((p, i) => {
        if (toSend.includes(p.packetId)) return
        toSend.push(new PacketAsk({
          clusterId: p.clusterId,
          message: {
            packetId: p.packetId,
            peerId: this.peerId
          }
        }))
      })

      let count = 0
      for (const packet of packets) {
        if (count >= 3) break

        if (!toSend.find(p => p.packetId === packet.packetId)) {
          count++
          toSend.push(packet)
        }
      }

      const sends = toSend.map(packet => Packet.encode(packet).then(data => {
        return this.send(data, port, address)
      }))

      return Promise.all(sends)
    }

    async onAskPacket (packet, port, address) {
      if (this.queries[packet.packetId]) return
      this.queries[packet.packetId] = 6

      const packets = [...this.cache.data]

      const reply = async p => {
        p = { ...p, type: PacketAnswer.type }
        this.send(await Packet.encode(p), port, address)
      }

      if (packet.message.previousId) { // get the previous packet
        const p = packets.find(p => p.packetId === packet.message.previousId)
        if (p) reply(p)
      } else if (packet.message.packetId) { // get the next packet
        const p = packets.find(p => p.previousId === packet.message.packetId)
        if (p) reply(p)
      }
    }

    async onAnswerPacket (packet, port, address) {
      if (this.queries[packet.packetId]) return
      this.queries[packet.packetId] = 6

      packet = { ...packet, type: PacketPublish.type }

      if (await this.cache.insert(packet.packetId, packet)) {
        if (this.onPub) this.onPub(packet, port, address)

        // we may want to continue this conversation if the peer responded
        // successfully with an answer and we are still missing data.
        const isMissingTail = packet.previousId && !await this.cache.get(packet.previousId)
        const isMissingHead = packet.previousId && !await this.cache.get(packet.nextId)

        if (isMissingTail) {
          await this.send(await Packet.encode(new PacketAsk({
            message: {
              peerId: this.peerId,
              previousId: packet.previousId
            }
          })), port, address)
        }

        if (isMissingHead) {
          await this.send(await Packet.encode(new PacketAsk({
            message: {
              peerId: this.peerId,
              packetId: packet.nextId
            }
          })), port, address)
        }
      }
    }

    async onQueryPacket (packet, port, address) {
      if (this.queries[packet.packetId]) return
      this.queries[packet.packetId] = 6

      const { packetId, clusterId } = packet

      if (this.onQuery && !await this.onQuery(packet, port, address)) {
        if (!Array.isArray(packet.message.history)) {
          packet.message.history = []
        }

        packet.message.history.push({ address: this.address, port: this.port })
        if (packet.message.history.length >= 16) packet.message.history.shift()

        // await for async call-stack
        return await this.mcast(packet, packetId, clusterId, true)
      }
    }

    close () {
      this.closing = true
    }

    //
    // Events
    //
    onConnection (peer, packet, port, address) {
      if (this.connects[packet.packetId]) return
      this.connects[packet.packetId] = true

      if (this.closing || !this.clusterId) return
      // this.negotiateCache(peer, packet, port, address)
      if (this.onConnect) this.onConnect(peer, packet, port, address)
    }

    async onPing (packet, port, address) {
      this.lastUpdate = Date.now()
      const { pingId, isReflection, isConnection, clusterId, isHeartbeat, testPort } = packet.message
      const peer = this.setPeer(packet, port, address)

      if (!isReflection && isHeartbeat && !pingId && !testPort && clusterId) {
        await this.negotiateCache(peer, packet, port, address)
        return
      }

      if (!port) port = packet.messsage.port
      if (!address) address = packet.message.address

      const message = {
        cacheSize: this.cache.size,
        clusterId: peer.clusterId || clusterId,
        peerId: this.peerId,
        port,
        address
      }

      if (pingId) message.pingId = pingId

      if (isReflection) {
        message.isReflection = true
        message.port = port
        message.address = address
      } else {
        message.natType = this.natType
      }

      if (isConnection && peer) {
        message.isConnection = true
        message.port = this.port || port
        message.address = this.address || address
        if (peer.clusterId) this.onConnection(peer, packet, port, address)
      }

      const packetPong = new PacketPong({ message })
      const buf = await Packet.encode(packetPong)

      await this.send(buf, port, address)

      if (testPort) { // also send to the test port
        message.testPort = testPort
        const packetPong = new PacketPong({ message })
        const buf = await Packet.encode(packetPong)
        await this.send(buf, testPort, address)
      }
    }

    async onPong (packet, port, address) {
      this.lastUpdate = Date.now()
      const { testPort, pingId, isReflection } = packet.message
      const oldType = this.natType

      // debug('<- PONG', this.address, this.port, this.peerId, '<>', packet.message.peerId, address, port)

      if (packet.message.isConnection) {
        const peer = this.setPeer(packet, packet.message.port, packet.message.address)
        if (!peer) return
        peer.lastUpdate = Date.now()
        if (pingId) peer.pingId = pingId

        if (peer.clusterId) this.onConnection(peer, packet, port, address)
        return
      }

      if (isReflection && this.natType !== 'static') {
        // this isn't a connection, it's just verifying that this is or is not a static peer
        if (packet.message.address === this.address && testPort === this.config.testPort) {
          this.reflectionId = null
          this.natType = 'static'
          this.address = packet.message.address
          this.port = packet.message.port
          // debug(`  response ${this.peerId} nat=${this.natType}`)
          if (this.onNat && oldType !== this.natType) this.onNat(this.natType)
          this.join()
          return
        }

        if (!testPort && pingId && this.reflectionId === null) {
          this.reflectionId = pingId
          this.reflectionFirstResponder = { port, address, pingId }
        } else if (!testPort && pingId && pingId === this.reflectionId) {
          if (packet.message.address === this.address && packet.message.port === this.port) {
            this.natType = 'easy'
          } else if (packet.message.address === this.address && packet.message.port !== this.port) {
            this.natType = 'hard'
          }

          {
            const peer = this.peers.find(p => p.port === port && p.address === address)
            this.ping(peer, false, {
              clusterId: this.clusterId,
              peerId: this.peerId,
              natType: this.natType,
              cacheSize: this.cache.size,
              isHeartbeat: true
            })
          }

          if (this.reflectionFirstResponder) {
            const { port, address } = this.reflectionFirstResponder
            const peer = this.setPeer(packet, port, address)

            this.ping(peer, false, {
              clusterId: this.clusterId,
              peerId: this.peerId,
              natType: this.natType,
              cacheSize: this.cache.size,
              isHeartbeat: true
            })
          }

          this.reflectionId = null
          this.reflectionFirstResponder = null

          if (oldType !== this.natType) {
            // debug(`  response ${this.peerId} nat=${this.natType}`)
            if (this.onNat) this.onNat(this.natType)
            this.join()
          }
        }

        this.address = packet.message.address
        this.port = packet.message.port
      } else if (!this.peers.find(p => p.address === address && p.port === port)) {
        const peer = this.setPeer(packet, port, address)
        if (peer && peer.clusterId) this.onConnection(peer, packet, port, address)
      } else {
        this.setPeer(packet, port, address)
        if (this.clusterId) {
          this.join()
        }
      }
    }

    //
    // The caller is offering us a peer to connect with
    //
    async onIntro (packet, port, address) {
      const peer = this.setPeer(packet, packet.message.port, packet.message.address)
      if (!peer || peer.connecting) return

      const remoteHard = packet.message.natType === 'hard'
      const localHard = this.natType === 'hard'
      const localEasy = ['easy', 'static'].includes(this.natType)
      const remoteEasy = ['easy', 'static'].includes(packet.message.natType)

      // debug(`intro arriving at ${this.peerId} (${this.address}:${this.port}:${this.natType}) from ${packet.message.peerId} (${address}:${port}:${packet.message.natType})`)

      const portCache = {}
      const getPort = createPortGenerator(DEFAULT_PORT, DEFAULT_TEST_PORT)
      const pingId = Math.random().toString(16).slice(2)

      const encoded = await Packet.encode(new PacketPing({
        message: {
          clusterId: packet.message.clusterId,
          peerId: this.peerId,
          natType: this.natType,
          isConnection: true,
          pingId
        }
      }))

      if (localEasy && remoteHard) {
        // debug(`onintro (${this.peerId} easy, ${packet.message.peerId} hard)`)
        peer.connecting = true

        let i = 0
        this.timer(0, 10, async (_ts) => {
          // send messages until we receive a message from them. giveup after
          // sending 1000 packets. 50% of the time 250 messages should be enough.
          if (i++ > 512) {
            peer.connecting = false
            return false
          }

          // if we got a pong packet with an id that matches the id ping packet,
          // the peer will have that pingId on it so we should check for it.
          const pair = this.peers.find(p => p.pingId === pingId)

          if (pair) {
            // debug('intro - connected:', i, s)
            pair.lastUpdate = Date.now()
            peer.connecting = false
            this.onConnection(pair, packet, pair.port, pair.address)
            return false
          }

          await this.send(encoded, getPort(portCache), packet.message.address)
        })

        return
      }

      if (localHard && remoteEasy) {
        // debug(`onintro (${this.peerId} hard, ${packet.message.peerId} easy)`)
        if (!this.bdpCache.length) {
          this.bdpCache = Array.from({ length: 256 }, () => getPort(portCache))
        }

        for (const port of this.bdpCache) {
          this.send(encoded, port, packet.message.address)
        }
        return
      }

      if (peer.clusterId) this.onConnection(peer, packet, port, address)

      // debug(`onintro (${this.peerId} easy/static, ${packet.message.peerId} easy/static)`)
      this.ping(peer, false, { clusterId: this.clusterId, peerId: this.peerId, natType: this.natType, isConnection: true, pingId: packet.message.pingId })
    }

    //
    // A peer is letting us know they want to join to a cluster
    //  - update the peer in this.peers to include the cluster
    //  - tell the peer what heads we have with the specified clusterId
    //  - tell the peer what tails we have to see if they can help with missing packets
    //  - introduce this peer to some random peers in this.peers (introductions
    //    are only made to live peers, we do not track dead peers -- our TTL is
    //    30s so this would create a huge amount of useless data sent over the network.
    async onJoin (packet, port, address, data) {
      if (this.joins[packet.packetId] || packet.hops > this.maxHops) return

      this.joins[packet.packetId] = 2

      this.lastUpdate = Date.now()
      this.setPeer(packet, port, address)

      let list = this.peers.filter(p => {
        if (p.natType === 'hard' && this.natType === 'hard') return null
        if (this.peerId === p.peerId) return null
        if (this.peerId === packet.message.peerId) return null
        if (p.peerId === packet.message.peerId) return null
        if (!p.port || !p.natType) return null
        return p
      })

      // if this is a propagated join, only intro peers that are a cluster member
      if (packet.hops > 1) list = list.filter(p => p.clusterId === packet.message.clusterId)

      const peers = this.getPeers(packet, list, 3)
      for (const peer of peers) {
        // tell the caller to ping a peer from the list
        const i1p = new PacketIntro({
          message: {
            clusterId: peer.clusterId,
            peerId: peer.peerId,
            natType: peer.natType,
            address: peer.address,
            port: peer.port
          }
        })
        const intro1 = await Packet.encode(i1p)

        // tell the peer from the list to ping the caller
        const i2p = new PacketIntro({
          message: {
            clusterId: packet.message.clusterId,
            peerId: packet.message.peerId,
            natType: packet.message.natType,
            address: packet.message.address,
            port: packet.message.port
          }
        })
        const intro2 = await Packet.encode(i2p)

        await new Promise(resolve => {
          this.timer(Math.random() * 2, 0, async () => {
            const sends = []
            // console.log(`JOIN: ${peer.address}:${peer.port} -> ${i2p.message.address}:${i2p.message.port}`)
            sends.push(this.send(intro2, peer.port, peer.address))

            // console.log(`JOIN: ${packet.message.address}:${packet.message.port} -> ${i1p.message.address}:${i1p.message.port}`)
            sends.push(this.send(intro1, packet.message.port, packet.message.address))

            await Promise.all(sends).then(resolve)
          })
        })
      }

      if (!this.clusterId && !packet.message.clusterId) return

      const ignorelist = [{ address, port }, { address: packet.message.address, port: packet.message.port }]
      this.mcast(packet, packet.packetId, packet.clusterId, true, ignorelist)
    }

    //
    // Event is fired when a user calls .publish and a peer receives the
    // packet, or when a peer receives a join packet.
    //
    async onPub (packet, port, address, data) {
      if (this.clusterId === packet.clusterId) {
        packet.hops += 2
      } else {
        packet.hops += 1
      }

      if (this.cache.has(packet.packetId)) return
      await this.cache.insert(packet.packetId, packet)

      let predicate = false

      if (this.predicateOnPacket) {
        predicate = this.predicateOnPacket(packet, port, address)
      } else if (this.encryption.has(packet.to)) {
        let p = { ...packet }

        if (packet.index > -1) p = await this.cache.compose(p, this)

        if (p) {
          try {
            p.message = this.encryption.open(p.message, p.to)
          } catch (e) { return this.onError(e) }

          predicate = true
          if (this.onPacket && p.index === -1) this.onPacket(p, port, address)
        } else {
          for (const peer of this.getPeers(packet, this.peers)) {
            this.negotiateCache(peer, packet, peer.port, peer.address)
          }
        }
      }

      if (predicate === false || packet.hops > this.maxHops) return
      this.mcast(packet, packet.packetId, packet.clusterId, true, [{ address, port }])
    }

    rateLimit (data, port, address) {
      this.rates ??= new Map()

      if (rateLimit(this.rates, port, address)) {
        return true
      }

      if (typeof this.limit === 'function' && this.limit(data, port, address)) {
        return true
      }

      return false
    }

    /**
     * When a packet is received it is decoded, the packet contains the type
     * of the message. Based on the message type it is routed to a function.
     * like WebSockets, don't answer queries unless we know its another SRP peer.
     *
     * @param {Buffer|Uint8Array} data
     * @param {{ port: number, address: string }} info
     */
    onMessage (data, { port, address }) {
      if (!this.rateLimit(data, port, address)) return

      const packet = Packet.decode(data)
      if (packet.version < VERSION) return

      const args = [packet, port, address, data]
      if (this.firewall) if (!this.firewall(...args)) return
      if (this.onData) this.onData(...args)

      switch (packet.type) {
        case PacketPing.type: return this.onPing(...args)
        case PacketPong.type: return this.onPong(...args)
        case PacketIntro.type: return this.onIntro(...args)
        case PacketJoin.type: return this.onJoin(...args)
        case PacketPublish.type: return this.onPub(...args)
        case PacketAsk.type: return this.onAskPacket(...args)
        case PacketAnswer.type: return this.onAnswerPacket(...args)
        case PacketQuery.type: return this.onQueryPacket(...args)
      }
    }
  }

  return Peer
}

export default createPeer
