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
import { sodium, randomBytes } from '../crypto.js'

import { Encryption } from './encryption.js'
import { Cache } from './cache.js'
import * as NAT from './nat.js'

import {
  Packet,
  PacketPing,
  PacketPong,
  PacketIntro,
  PacketPublish,
  PacketStream,
  PacketSync,
  PacketJoin,
  PacketQuery,
  sha256,
  VERSION
} from './packets.js'

let logcount = 0

export const debug = (pid, ...args) =>
  typeof process !== 'undefined' &&
  process.env.DEBUG &&
  args.some(v => new RegExp(process.env.DEBUG).test(v)) &&
  console.log(String(logcount++).padStart(4), '│', pid.slice(0, 4), ...args)

export { sha256, Cache, Encryption, NAT }

/**
 * Retry delay in milliseconds for ping.
 * @type {number}
 */
export const PING_RETRY = 500

/**
 * Probe wait timeout in milliseconds.
 * @type {number}
 */
export const PROBE_WAIT = 512

/**
 * Default keep alive timeout.
 * @type {number}
 */
export const DEFAULT_KEEP_ALIVE = 30_000

/**
 * Default rate limit threshold in milliseconds.
 * @type {number}
 */
export const DEFAULT_RATE_LIMIT_THRESHOLD = 8000

const PRIV_PORTS = 1024
const MAX_PORTS = 65535 - PRIV_PORTS
const MAX_BANDWIDTH = 1024 * 32

/**
 * Port generator factory function.
 * @param {object} ports - the cache to use (a set)
 * @param {number?} p - initial port
 * @return {number}
 */
export const getRandomPort = (ports = new Set(), p) => {
  do {
    p = Math.max(1024, Math.ceil(Math.random() * 0xffff))
  } while (ports.has(p) && ports.size < MAX_PORTS)

  ports.add(p)
  return p
}

const isReplicatable = type => (
  type === PacketPublish.type ||
  type === PacketJoin.type
)

/**
 * Computes rate limit predicate value for a port and address pair for a given threshold
 * updating an input rates map.
 * @param {Map} rates
 * @param {number} type
 * @param {number} port
 * @param {string} address
 * @return {boolean}
 */
export function rateLimit (rates, type, port, address, subclusterIdQuota) {
  const R = isReplicatable(type)
  const key = (R ? 'R' : 'C') + ':' + address + ':' + port
  const quota = subclusterIdQuota || R ? 512 : 4096
  const time = Math.floor(Date.now() / 60000)
  const rate = rates.get(key) || { time, quota, used: 0 }

  rate.mtime = Date.now() // checked by mainLoop for garabge collection

  if (time !== rate.time) {
    rate.time = time
    if (rate.used > rate.quota) rate.quota -= 1
    else if (rate.used < quota) rate.quota += 1
    rate.used = 0
  }

  rate.used += 1

  rates.set(key, rate)
  if (rate.used >= rate.quota) return true
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
  clusters = {}
  pingId = null
  distance = 0
  connected = false
  probed = false
  proxy = null
  clock = 0
  uptime = 0
  lastUpdate = 0
  lastRequest = 0
  localPeer = null

  /**
   * `RemotePeer` class constructor.
   * @param {{
   *   peerId?: string,
   *   address?: string,
   *   port?: number,
   *   natType?: number,
   *   clusters: object,
   *   reflectionId?: string,
   *   distance?: number,
   *   publicKey?: string,
   *   privateKey?: string,
   *   clock?: number,
   *   lastUpdate?: number,
   *   lastRequest?: number
   * }} o
   */
  constructor (o, peer) {
    this.localPeer = peer

    if (!o.port) throw new Error('expected .port')
    if (!o.address) throw new Error('expected .address')
    if (!o.peerId) throw new Error('expected .peerId')
    if (o.indexed) o.natType = NAT.UNRESTRICTED
    if (o.natType && !NAT.isValid(o.natType)) throw new Error('invalid .natType')

    if (o.subclusterId && o.clusterId) {
      this.clusters[o.clusterId] = { [o.subclusterId]: MAX_BANDWIDTH }
    }

    Object.assign(this, o)
  }

  async write (sharedKey, args) {
    let rinfo = this
    if (this.proxy) rinfo = this.proxy

    const keys = await Encryption.createKeyPair(sharedKey)

    args.subclusterId = Buffer.from(keys.publicKey).toString('base64')
    args.clusterId = this.localPeer.clusterId
    args.streamTo = this.peerId
    args.streamFrom = this.localPeer.peerId
    args.message = this.localPeer.encryption.seal(args.message, keys)

    const packets = await this.localPeer._message2packets(PacketStream, args.message, args)

    if (this.proxy) {
      debug(this.localPeer.peerId, `>> WRITE STREAM HAS PROXY ${this.proxy.address}:${this.proxy.port}`)
    }

    for (const packet of packets) {
      debug(args.streamFrom, `>> WRITE STREAM (from=${args.streamFrom.slice(0, 6)}, to=${args.streamTo.slice(0, 6)}, via=${rinfo.address}:${rinfo.port})`)
      this.localPeer.send(await Packet.encode(packet), rinfo.port, rinfo.address, this.socket)
    }
  }
}

/**
 * `Peer` class factory.
 * @param {{ createSocket: function('udp4', null, object?): object }} options
 */
export const wrap = dgram => {
  class Peer {
    port = null
    address = null
    natType = NAT.UNKNOWN
    nextNatType = NAT.UNKNOWN
    clusters = {}
    reflectionId = null
    reflectionTimeout = null
    reflectionStage = 0
    reflectionFirstResponder = null
    peerId = ''
    isListening = false
    ctime = Date.now()
    lastUpdate = 0
    lastSync = Date.now()
    closing = false
    clock = 0
    unpublished = {}
    cache = null
    uptime = 0
    maxHops = 16
    bdpCache = /** @type {number[]} */ ([])

    onListening = null
    onDelete = null

    firewall = null
    rates = new Map()
    streamBuffer = new Map()
    controlPackets = new Map()

    metrics = { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0, 5: 0, 6: 0, 7: 0 }

    peers = JSON.parse(/* snapshot_start=1691579150299, filter=easy,static */`[
      {"address":"3.70.160.181","port":41141,"peerId":"707c07171ac9371b2f1de23e78dad15d29b56d47abed5e5a187944ed55fc8483"},
      {"address":"3.122.250.236","port":64236,"peerId":"a830615090d5cdc3698559764e853965a0d27baad0e3757568e6c7362bc6a12a"},
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
      {"address":"3.122.250.236","port":64236,"peerId":"a830615090d5cdc3698559764e853965a0d27baad0e3757568e6c7362bc6a12a"},
      {"address":"18.130.98.23","port":25111,"peerId":"ba483c1477ab7a99de2d9b60358d9641ff6a6dc6ef4e3d3e1fc069b19ac89da4"},
      {"address":"18.129.76.26","port":9210,"peerId":"099b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"18.138.72.20","port":2909,"peerId":"199b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"18.229.140.216","port":36056,"peerId":"73d726c04c05fb3a8a5382e7a4d7af41b4e1661aadf9020545f23781fefe3527"},
      {"address":"13.230.254.25","port":65049,"peerId":"592e9fd43abcf854fa5cc9a203bebd15afb2e900308044b77599cd8f46f2532d"},
      {"address":"18.138.72.20","port":2909,"peerId":"199b00e0e516b2f1599459b2429d0dbb3a08eeadce00e68bbc4a7e3f2171bb65"},
      {"address":"18.229.140.216","port":36056,"peerId":"73d726c04c05fb3a8a5382e7a4d7af41b4e1661aadf9020545f23781fefe3527"},
      {"address":"3.76.100.161","port":25761,"peerId":"07bffa90d89bf74e06ff7f83938b90acb1a1c5ce718d1f07854c48c6c12cee49"},
      {"address":"3.68.89.3","port":22787,"peerId":"448b083bd8a495ce684d5837359ce69d0ff8a5a844efe18583ab000c99d3a0ff"}
    ]`/* snapshot_end=1691579150299 */).map((/** @type {object} */ o) => new RemotePeer({ ...o, indexed: true }, this))

    /**
     * `Peer` class constructor. Avoid calling this directly (use the create method).
     * @private
     * @param {object?} [persistedState]
     */
    constructor (persistedState = {}) {
      const config = persistedState?.config ?? persistedState ?? {}

      this.encryption = new Encryption()

      if (!config.peerId) throw new Error('constructor expected .peerId')

      this.config = {
        keepalive: DEFAULT_KEEP_ALIVE,
        lastGroupBroadcast: -1,
        ...config
      }

      let cacheData

      if (persistedState?.data?.length > 0) {
        cacheData = new Map(persistedState.data)
      }

      this.cache = new Cache(cacheData, config.siblingResolver)

      this.unpublished = persistedState?.unpublished || {}
      this._onError = err => this.onError && this.onError(err)

      Object.assign(this, config)

      if (!this.indexed && !this.clusterId) throw new Error('constructor expected .clusterId')
      if (typeof this.peerId !== 'string') throw new Error('peerId should be of type string')

      this.port = config.port || null
      this.natType = config.natType || null
      this.address = config.address || null

      this.socket = dgram.createSocket('udp4', null, this)
      this.probeSocket = dgram.createSocket('udp4', null, this)
    }

    /**
     * An implementation for clearning an interval that can be overridden by the test suite
     * @param Number the number that identifies the timer
     * @return {undefined}
     * @ignore
     */
    _clearInterval (tid) {
      clearInterval(tid)
    }

    /**
     * An implementation for clearning a timeout that can be overridden by the test suite
     * @param Number the number that identifies the timer
     * @return {undefined}
     * @ignore
     */
    _clearTimeout (tid) {
      clearTimeout(tid)
    }

    /**
     * An implementation of an internal timer that can be overridden by the test suite
     * @return {Number}
     * @ignore
     */
    _setInterval (fn, t) {
      return setInterval(fn, t)
    }

    /**
     * An implementation of an timeout timer that can be overridden by the test suite
     * @return {Number}
     * @ignore
     */
    _setTimeout (fn, t) {
      return setTimeout(fn, t)
    }

    /**
     * A method that encapsulates the listing procedure
     * @return {undefined}
     * @ignore
     */
    async _listen () {
      await sodium.ready

      this.socket.on('message', (...args) => this._onMessage(...args))
      this.socket.on('error', (...args) => this._onError(...args))
      this.probeSocket.on('message', (...args) => this._onProbeMessage(...args))
      this.probeSocket.on('error', (...args) => this._onError(...args))

      this.socket.setMaxListeners(2048)
      this.probeSocket.setMaxListeners(2048)

      const listening = Promise.all([
        new Promise(resolve => this.socket.on('listening', resolve)),
        new Promise(resolve => this.probeSocket.on('listening', resolve))
      ])

      this.socket.bind(this.config.port || 0)
      this.probeSocket.bind(this.config.probeInternalPort || 0)

      await listening

      this.config.port = this.socket.address().port
      this.config.probeInternalPort = this.probeSocket.address().port

      if (this.onListening) this.onListening()
      this.isListening = true

      debug(this.peerId, `++ INIT (config.internalPort=${this.config.port}, config.probeInternalPort=${this.config.probeInternalPort})`)
    }

    /*
     * This method will bind the sockets, begin pinging known peers, and start
     * the main program loop.
     * @return {Any}
     */
    async init (cb) {
      if (!this.isListening) await this._listen()
      if (cb) this.onReady = cb

      // tell all well-known peers that we would like to hear from them, if
      // we hear from any we can ask for the reflection information we need.
      for (const peer of this.peers.filter(p => p.indexed)) {
        this.ping(peer, false, { requesterPeerId: this.peerId })
      }

      //
      // in browser envrionments we want to kick off a request for reflection
      // when we come back online (our nat may have changed, etc).
      //
      globalThis.window?.addEventListener('online', () => this.requestReflection())

      this._mainLoop(Date.now())
      this.mainLoopTimer = this._setInterval(ts => this._mainLoop(ts), this.config.keepalive)

      if (this.indexed && this.onReady) return this.onReady()
    }

    /**
     * Continuously evaluate the state of the peer and its network
     * @return {undefined}
     * @ignore
     */
    _mainLoop (ts) {
      if (this.closing) return this._clearInterval(this.mainLoopTimer)

      const offline = globalThis.navigator && !globalThis.navigator.onLine
      if (offline) return true

      if (!this.reflectionId) this.requestReflection()
      if (this.onInterval) this.onInterval()

      this.uptime += this.config.keepalive

      // wait for nat type to be discovered
      if (!NAT.isValid(this.natType)) return true

      for (const [k, packet] of [...this.cache.data]) {
        const p = Packet.from(packet)
        if (!p) continue
        if (!p.timestamp) p.timestamp = ts

        const ttl = (p.ttl < Packet.ttl) ? p.ttl : Packet.ttl
        const deadline = p.timestamp + ttl

        if (deadline <= ts) {
          this.mcast(p)
          this.cache.delete(k)
          debug(this.peerId, '-- DELETE', k, this.cache.size)
          if (this.onDelete) this.onDelete(p)
        }
      }

      for (let [k, v] of this.controlPackets.entries()) {
        v -= 1
        if (!v) this.controlPackets.delete(k)
        else this.controlPackets.set(k, v)
      }

      for (const [i, peer] of Object.entries(this.peers)) {
        if (peer.natType === NAT.UNKNOWN) continue

        if (!peer.indexed) {
          if ((peer.lastUpdate + this.config.keepalive) < Date.now()) {
            const p = this.peers.splice(i, 1)
            if (this.onDisconnect) this.onDisconnect(p)
            continue
          }
        }

        this.ping(peer, false, {
          requesterPeerId: this.peerId,
          natType: this.natType,
          cacheSize: this.cache.size
        })
      }

      // if this peer has previously tried to join any clusters, multicast a
      // join messages for each into the network so we are always searching.
      for (const cluster of Object.values(this.clusters)) {
        for (const subcluster of Object.values(cluster)) {
          this.join(subcluster.sharedKey, subcluster)
        }
      }
      return true
    }

    /**
     * Continuously evaluate the state of the peer and its network
     * @param {Buffer} data - An encoded packet
     * @param {number} port - The desination port of the remote host
     * @param {string} address - The destination address of the remote host
     * @param {Socket=this.socket} socket - The socket to send on
     * @return {undefined}
     * @ignore
     */
    send (data, port, address, socket = this.socket) {
      socket.send(data, port, address, err => {
        if (err) return this._onError(err)
        const packet = Packet.decode(data)
        if (this.onSend) this.onSend(packet, port, address)
        debug(this.peerId, `>> SEND (from=${this.address}:${this.port}, to=${address}:${port}, type=${packet.type})`)
      })
    }

    /**
     * Send any unpublished packets
     * @return {undefined}
     * @ignore
     */
    async sendUnpublished () {
      for (const [packetId] of Object.entries(this.unpublished)) {
        const packet = this.cache.get(packetId)
        if (packet) this.mcast(packet)
      }
    }

    /**
     * Get the serializable state of the peer (can be passed to the constructor or create method)
     * @return {undefined}
     */
    getState () {
      return {
        config: this.config,
        data: [...this.cache.data.entries()],
        unpublished: this.unpublished
      }
    }

    /**
     * Get a selection of known peers
     * @return {Array<RemotePeer>}
     * @ignore
     */
    getPeers (packet, peers, ignorelist, filter = o => o) {
      const n = 3
      const rand = () => Math.random() - 0.5

      const base = p => {
        if (ignorelist.findIndex(ilp => ilp.peerId === p.peerId) > -1) return false
        if (p.lastUpdate !== 0 && p.lastUpdate > this.keepalive) return false
        if (this.peerId === p.peerId) return false // same as introducer
        if (packet.message.requesterPeerId === p.peerId) return false // same as requester
        if (!p.port || (!p.indexed && !NAT.isValid(p.natType))) return false
        return true
      }

      const candidates = peers
        .filter(filter)
        .filter(base)
        .sort(rand)

      const list = candidates.filter(peer => peer.clusters && peer.clusters[packet.clusterId])

      if (list.length < n) {
        list.push(...candidates.filter(p => !p.indexed).slice(0, n - list.length))
      }

      if (list.length < n) {
        list.push(...candidates.filter(p => p.indexed).slice(0, n - list.length))
      }

      return list
    }

    /**
     * Send an eventually consistent packet to a selection of peers (fanout)
     * @return {undefined}
     * @ignore
     */
    async mcast (packet, isTaxed, ignorelist = []) {
      const peers = this.getPeers(packet, this.peers, ignorelist)

      packet.hops += 1

      for (const peer of peers) {
        this.send(await Packet.encode(packet), peer.port, peer.address)
      }

      if (this.controlPackets.has(packet.packetId)) return
      this.controlPackets.set(packet.packetId, 1)
    }

    /**
     * The process of determining this peer's NAT behavior (firewall and dependentness)
     * @return {undefined}
     * @ignore
     */
    async requestReflection () {
      if (this.onConnecting) this.onConnecting({ code: -2 })

      if (this.closing || this.indexed || this.reflectionId) {
        debug(this.peerId, '<> REFLECT ABORTED', this.reflectionId)
        return
      }

      debug(this.peerId, '-> REQ REFLECT', this.reflectionId, this.reflectionStage)
      if (this.onConnecting) this.onConnecting({ code: -1 })

      const peers = [...this.peers]
        .filter(p => p.lastUpdate !== 0)
        .filter(p => p.natType === NAT.UNRESTRICTED || p.natType === NAT.ADDR_RESTRICTED || p.indexed)

      if (peers.length < 2) {
        if (this.onConnecting) this.onConnecting({ code: 0 })
        debug(this.peerId, 'XX REFLECT NOT ENOUGH PINGABLE PEERS - RETRYING')
        return this._setTimeout(() => this.requestReflection(), 256)
      }

      const requesterPeerId = this.peerId
      const opts = { requesterPeerId, isReflection: true }

      this.reflectionId = opts.reflectionId = randomBytes(6).toString('hex').padStart(12, '0')

      if (this.onConnecting) this.onConnecting({ code: 0.5, stage: this.reflectionStage })
      //
      // # STEP 1
      // The purpose of this step is strictily to discover the external port of
      // the probe socket.
      //
      if (this.reflectionStage === 0) {
        if (this.onConnecting) this.onConnecting({ code: 1 })
        // start refelection with an zeroed NAT type
        if (this.reflectionTimeout) this._clearTimeout(this.reflectionTimeout)
        this.reflectionStage = 1

        debug(this.peerId, '-> NAT REFLECT - STAGE1: A', this.reflectionId)
        const list = peers.filter(p => p.probed).sort(() => Math.random() - 0.5)
        const peer = list.length ? list[0] : peers[0]
        peer.probed = true // mark this peer as being used to provide port info
        this.ping(peer, false, { ...opts, isProbe: true }, this.probeSocket)

        // we expect onMessageProbe to fire and clear this timer or it will timeout
        this.probeReflectionTimeout = this._setTimeout(() => {
          this.probeReflectionTimeout = null
          if (this.reflectionStage !== 1) return
          debug(this.peerId, 'XX NAT REFLECT - STAGE1: C - TIMEOUT', this.reflectionId)

          this.reflectionStage = 1
          this.reflectionId = null
          this.requestReflection()
        }, 1024)

        debug(this.peerId, '-> NAT REFLECT - STAGE1: B', this.reflectionId)
        return
      }

      //
      // # STEP 2
      //
      // The purpose of step 2 is to ask two different peers for the external
      // port and address for our primary socket. If they are the not same, we
      // can determine that the primary socket is a `ENDPOINT_DEPENDENT`.
      //
      if (this.reflectionStage === 1) {
        if (this.onConnecting) this.onConnecting({ code: 2 })
        this.reflectionStage = 2
        const { probeExternalPort } = this.config

        const list = peers.filter(p => p.probed)
        const peer2 = list.sort(() => Math.random() - 0.5)[0]

        if (!peer2) { // how did it advance?
          debug(this.peerId, 'XX NAT REFLECT - STAGE2: NO PEERS HAVE BEEN PROBED YET - RETRYING')
          return this._setTimeout(() => this.requestReflection(), 256)
        }

        const peer1 = peers.filter(p => p.peerId !== peer2.peerId).sort(() => Math.random() - 0.5)[0]

        debug(this.peerId, '-> NAT REFLECT - STAGE2: START', this.reflectionId)

        // reset reflection variables to defaults
        this.nextNatType = NAT.UNKNOWN
        this.reflectionFirstResponder = null

        this.ping(peer1, false, { ...opts, probeExternalPort })
        this.ping(peer2, false, { ...opts, probeExternalPort })

        if (this.reflectionTimeout) {
          this._clearTimeout(this.reflectionTimeout)
          this.reflectionTimeout = null
        }

        this.reflectionTimeout = this._setTimeout(ts => {
          this.reflectionTimeout = null
          if (this.reflectionStage !== 2) return
          debug(this.peerId, 'XX NAT REFLECT - STAGE2: TIMEOUT', this.reflectionId)
          return this.requestReflection()
        }, 2048)
      }
    }

    /**
     * Ping another peer
     * @return {PacketPing}
     * @ignore
     */
    async ping (peer, withRetry, props, socket) {
      if (!peer) {
        return
      }

      const opts = {
        message: {
          requesterPeerId: this.peerId,
          uptime: this.uptime,
          timestamp: Date.now(),
          ...props
        }
      }

      const packet = new PacketPing(opts)
      const data = await Packet.encode(packet)

      const send = () => {
        if (this.closing) return false

        const p = this.peers.find(p => p.peerId === peer.peerId)
        if (p?.reflectionId && p.reflectionId === packet.message.reflectionId) {
          return false
        }

        this.send(data, peer.port, peer.address, socket)
        if (p) p.lastRequest = Date.now()
      }

      send()

      if (withRetry) {
        this._setTimeout(send, PING_RETRY)
        this._setTimeout(send, PING_RETRY * 4)
      }

      return packet
    }

    /**
     * Add a peer to this peer's known peers list
     * @return {RemotePeer}
     * @ignore
     */
    addPeer (args) {
      const existingPeer = this.getPeer(args.peerId)
      if (existingPeer) return this.updatePeer(existingPeer)

      if (this.peers.length > 1024) {
        this.peers
          .sort(() => Math.random() - 0.5)
          .filter(p => !p.indexed)
          .pop()
      }

      if (!args.peerId) return null

      const peer = new RemotePeer(args, this)
      if (args.connected) peer.lastUpdate = Date.now()

      if (args.clusterId) peer.clusters[args.clusterId] ??= {}
      if (args.subclusterId) peer.clusters[args.subclusterId] = MAX_BANDWIDTH
      this.peers.push(peer)
      return peer
    }

    /**
     * @return {RemotePeer}
     * @ignore
     */
    getPeer (id) {
      return this.peers.find(p => p.peerId === id)
    }

    /**
     * @return {RemotePeer}
     * @ignore
     */
    updatePeer (args) {
      const existingPeer = this.getPeer(args.peerId)
      if (!existingPeer) return this.addPeer(args)

      if (args.connected) existingPeer.lastUpdate = Date.now()

      if (isNaN(args.clock) || existingPeer.clock > args.clock) {
        return existingPeer
      }

      if (existingPeer.indexed) return existingPeer

      if (args.pingId) existingPeer.pingId = args.pingId

      if (args.clusterId) existingPeer.clusters[args.clusterId] ??= {}
      if (args.subclusterId) existingPeer.clusters[args.subclusterId] = MAX_BANDWIDTH

      debug(this.peerId, `<- PEER UPDATE (peerId=${args.peerId.slice(0, 6)}, clock=${args.clock}, address=${args.address}:${args.port})`)

      args.connected ||= existingPeer.connected

      if (!existingPeer.port) {
        delete args.port
        delete args.address
      }

      Object.assign(existingPeer, args)
      return existingPeer
    }

    /**
     * This should be called at least once when an app starts to multicast
     * this peer, and starts querying the network to discover peers.
     * @param {object} keys - Created by `Encryption.createKeyPair()`.
     * @param {object=} args - Options
     * @param {number=MAX_BANDWIDTH} args.rateLimit - How many requests per second to allow for this subclusterId.
     * @return {RemotePeer}
     */
    async join (sharedKey, args = { rateLimit: MAX_BANDWIDTH }) {
      const keys = await Encryption.createKeyPair(sharedKey)
      this.encryption.add(keys.publicKey, keys.privateKey)

      if (!this.port || !this.natType || this.indexed) return

      args.sharedKey = sharedKey

      const clusterId = args.clusterId || this.config.clusterId
      const subclusterId = Buffer.from(keys.publicKey).toString('base64')

      this.clusters[clusterId] ??= {}
      this.clusters[clusterId][subclusterId] = args

      this.clock += 1

      const packet = new PacketJoin({
        clock: this.clock,
        clusterId,
        subclusterId,
        message: {
          requesterPeerId: this.peerId,
          natType: this.natType,
          address: this.address,
          port: this.port
        }
      })

      debug(this.peerId, `-> JOIN (clusterId=${clusterId}, clock=${packet.clock}/${this.clock})`)
      if (this.onState) await this.onState(this.getState())
      this.mcast(packet)
      this.controlPackets.set(packet.packetId, 1)
    }

    /**
     * @param {Packet} T - The constructor to be used to create packets.
     * @param {Any} message - The message to be split and packaged.
     * @return {Array<Packet<T>>}
     * @ignore
     */
    async _message2packets (T, message, args) {
      const { clusterId, subclusterId, packet, nextId, meta = {}, usr1, usr2, sig } = args

      let messages = [message]
      const len = message?.byteLength ?? message?.length ?? 0
      let clock = packet?.clock || 0

      const siblings = [...this.cache.data.values()]
        .filter(Boolean)
        .filter(p => p?.previousId === packet?.packetId)

      if (siblings.length) {
        // if there are siblings of the previous packet
        // pick the highest clock value, the parent packet or the sibling
        const sort = (a, b) => a.clock - b.clock
        const sib = siblings.sort(sort).reverse()[0]
        clock = Math.max(clock, sib.clock) + 1
      }

      clock += 1

      if (len > 1024) { // Split packets that have messages bigger than Packet.maxLength
        messages = [{
          meta,
          size: message.length,
          indexes: Math.ceil(message.length / 1024)
        }]
        let pos = 0
        while (pos < message.length) messages.push(message.slice(pos, pos += 1024))
      }

      // turn each message into an actual packet
      const packets = messages.map(message => new T({
        clusterId,
        subclusterId,
        streamTo: args.streamTo,
        streamFrom: args.streamFrom,
        clock,
        message,
        usr1,
        usr2,
        sig
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

      return packets
    }

    /**
     * Sends a packet into the network that will be replicated and buffered.
     * Each peer that receives it will buffer it until TTL and then replicate
     * it provided it has has not exceeded their maximum number of allowed hops.
     *
     * @param {object} keys - the public and private key pair created by `Encryption.createKeyPair()`.
     * @param {object} args - The arguments to be applied.
     * @param {Buffer} args.message - The message to be encrypted by keys and sent.
     * @param {Packet<T>=} args.packet - The previous packet in the packet chain.
     * @param {Buffer} args.usr1 - 32 bytes of arbitrary clusterId in the protocol framing.
     * @param {Buffer} args.usr2 - 32 bytes of arbitrary clusterId in the protocol framing.
     * @return {Array<PacketPublish>}
     */
    async publish (sharedKey, args) { // wtf to do here, we need subclusterId and the actual user keys
      if (!sharedKey) throw new Error('.publish() expected "sharedKey" argument in first position')
      if (!isBufferLike(args.message)) throw new Error('.publish() will only accept a message of type buffer')

      const keys = await Encryption.createKeyPair(sharedKey)

      args.subclusterId = Buffer.from(keys.publicKey).toString('base64')
      args.clusterId = args.clusterId || this.config.clusterId

      const message = this.encryption.seal(args.message, keys)
      const packets = await this._message2packets(PacketPublish, message, args)

      for (const packet of packets) {
        const p = Packet.from(packet)
        this.cache.insert(packet.packetId, p)

        if (this.onPacket) this.onPacket(p, this.port, this.address, true)

        this.unpublished[packet.packetId] = Date.now()
        if (globalThis.navigator && !globalThis.navigator.onLine) continue

        this.mcast(packet)
      }

      return packets
    }

    /**
     * @return {undefined}
     * @ignore
     */
    async sync (peer, packet, port, address) {
      const summary = await this.cache.summarize()

      // if our caches are in sync, do nothing, perf is O(1)
      if (packet.message.cacheSummaryHash === summary.hash) return

      // if we are out of sync send our cache summary
      const data = await Packet.encode(new PacketSync({
        message: Cache.encodeSummary(summary)
      }))

      this.send(data, port, address)
    }

    close () {
      this._clearInterval(this.mainLoopTimer)

      if (this.closing) return

      this.closing = true
      this.socket.close()
      this.probeSocket.close()

      if (this.onClose) this.onClose()
    }

    /**
     * A connection was made, call the onConnection if it is defined by the user.
     * @return {undefined}
     * @ignore
     */
    async _onConnection (packet, peer, port, address) {
      if (this.closing) return

      debug(this.peerId, '<- CONNECTION', peer.peerId, address, port, packet.type)
      if (!peer.localPeer) peer.localPeer = this

      this.sync(peer, packet, port, address)

      if (this.onConnection) this.onConnection(packet, peer, port, address)
    }

    /**
     * Received a Sync Packet
     * @return {undefined}
     * @ignore
     */
    async _onSync (packet, port, address) {
      this.metrics[packet.type]++

      if (!isBufferLike(packet.message)) return

      debug(this.peerId, '<< SYNC CACHE', port, address, this.cache.size)

      const remote = Cache.decodeSummary(packet.message)

      const local = await this.cache.summarize(remote.prefix)
      if (local.hash === remote.hash) {
        if (this.onSyncFinished) this.onSyncFinished()
        return
      }

      for (let i = 0; i < local.buckets.length; i++) {
        //
        // nothing to send/sync, expect peer to send everything they have
        //
        if (!local.buckets[i]) continue
        //
        // you dont have any of these, im going to send them to you
        //
        if (!remote.buckets[i]) {
          const keys = [...this.cache.data.keys()]
            .filter(Boolean)
            .filter(key => key.startsWith(local.prefix + i.toString(16)))

          for (const key of keys) {
            const packet = Packet.from(this.cache.data.get(key))
            let data
            try {
              data = await Packet.encode(packet)
            } catch (err) {
              if (this.onWarn) this.onWarn(err)
              continue
            }
            this._setTimeout(() => this.send(data, port, address), 2048)
          }
        } else if (remote.buckets[i] !== local.buckets[i]) {
          //
          // need more details about what exactly isn't synce'd
          //
          const nextLevel = await this.cache.summarize(local.prefix + i.toString(16))
          const encoded = Cache.encodeSummary(nextLevel)
          const data = await Packet.encode(new PacketSync({
            message: encoded
          }))
          this._setTimeout(() => this.send(data, port, address), 2048)
        }
      }
    }

    /**
     * Received a Query Packet
     * @return {undefined}
     * @ignore
     */
    async _onQuery (packet, port, address) {
      this.metrics[packet.type]++

      debug(this.peerId, '<- QUERY', port, address)
      if (this.controlPackets.has(packet.packetId)) return
      this.controlPackets.set(packet.packetId, 1)

      if (this.onEvaluateQuery) await this.onEvaluateQuery(packet, port, address)

      if (!Array.isArray(packet.message.history)) {
        packet.message.history = []
      }

      packet.message.history.push({ address: this.address, port: this.port })
      if (packet.message.history.length >= 16) packet.message.history.shift()

      return await this.mcast(packet)
    }

    /**
     * Received a Ping Packet
     * @return {undefined}
     * @ignore
     */
    async _onPing (packet, port, address) {
      this.metrics[packet.type]++

      this.lastUpdate = Date.now()
      debug(this.peerId, `<- PING (from=${address}:${port})`)
      const { reflectionId, isReflection, isConnection, isHeartbeat } = packet.message
      if (packet.message.requesterPeerId === this.peerId) return

      const { probeExternalPort, isProbe, pingId } = packet.message

      let peer = this.getPeer(packet.message.requesterPeerId)

      if (isProbe) {
        debug(this.peerId, 'PING PROBE', port, address)
      }

      if (packet.message.natType) {
        const peerId = packet.message.requesterPeerId
        const natType = packet.message.natType
        peer = this.addPeer({ peerId, natType, port, address })
      }

      if (isHeartbeat) return

      if (peer && reflectionId) peer.reflectionId = reflectionId
      if (!port) port = packet.message.port
      if (!address) address = packet.message.address

      const message = {
        cacheSize: this.cache.size,
        uptime: this.uptime,
        responderPeerId: this.peerId,
        requesterPeerId: packet.message.requesterPeerId,
        port,
        isProbe,
        address
      }

      if (reflectionId) message.reflectionId = reflectionId
      if (isHeartbeat) message.isHeartbeat = Date.now()
      if (pingId) {
        message.pingId = pingId
        message.isDebug = true
      }

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
        this._onConnection(packet, peer, port, address)
      }

      const packetPong = new PacketPong({ message })
      const buf = await Packet.encode(packetPong)

      this.send(buf, port, address)

      if (probeExternalPort) {
        message.port = probeExternalPort
        const packetPong = new PacketPong({ message })
        const buf = await Packet.encode(packetPong)
        this.send(buf, probeExternalPort, address, this.probeSocket)
      }
    }

    /**
     * Received a Pong Packet
     * @return {undefined}
     * @ignore
     */
    async _onPong (packet, port, address) {
      this.metrics[packet.type]++

      this.lastUpdate = Date.now()

      const { reflectionId, pingId, isReflection, responderPeerId } = packet.message

      debug(this.peerId, '<- PONG', port, address)
      const peer = this.updatePeer({ peerId: packet.message.responderPeerId, port, address })
      if (!peer) return

      peer.lastUpdate = Date.now()

      if (packet.message.isConnection) {
        if (pingId) peer.pingId = pingId

        this._onConnection(packet, peer, packet.message.port, packet.message.address)
        return
      }

      if (isReflection && !this.indexed) {
        if (reflectionId !== this.reflectionId) return

        this._clearTimeout(this.reflectionTimeout)

        if (!this.reflectionFirstResponder) {
          this.reflectionFirstResponder = { port, address, responderPeerId, reflectionId, packet }
          debug(this.peerId, '<- NAT REFLECT - STAGE2: FIRST RESPONSE', port, address, this.reflectionId)
        } else {
          debug(this.peerId, '<- NAT REFLECT - STAGE2: SECOND RESPONSE', port, address, this.reflectionId)
          if (packet.message.address !== this.address) return

          this.nextNatType |= (
            packet.message.port === this.reflectionFirstResponder.packet.message.port
          )
            ? NAT.MAPPING_ENDPOINT_INDEPENDENT
            : NAT.MAPPING_ENDPOINT_DEPENDENT

          debug(
            this.peerId,
            `++ NAT REFLECT - STATE UPDATE (nextType=${this.nextNatType})`,
            packet.message.port,
            this.reflectionFirstResponder.packet.message.port
          )

          // wait PROBE_WAIT milliseconds for zero or more probe responses to arrive.
          this._setTimeout(() => {
            // build the NAT type by combining information about the firewall with
            // information about the endpoint independence
            let natType = this.nextNatType

            // in the case where we recieved zero probe responses, we assume the firewall
            // is of the hardest type 'FIREWALL_ALLOW_KNOWN_IP_AND_PORT'.
            if (!NAT.isFirewallDefined(natType)) natType |= NAT.FIREWALL_ALLOW_KNOWN_IP_AND_PORT

            if (NAT.isValid(natType)) {
              const oldType = this.natType
              this.natType = natType
              this.reflectionId = null
              this.reflectionStage = 0

              if (natType !== oldType) {
                // alert both peers of our new NAT type
                const peersToUpdate = [
                  this.getPeer(responderPeerId),
                  this.getPeer(this.reflectionFirstResponder.responderPeerId)
                ]

                peersToUpdate.forEach(peer => {
                  if (!peer) return
                  peer.lastRequest = Date.now()

                  this.ping(peer, false, {
                    requesterPeerId: this.peerId,
                    natType: this.natType,
                    cacheSize: this.cache.size,
                    isHeartbeat: Date.now()
                  })
                })

                if (this.onNat) this.onNat(this.natType)
                if (this.onConnecting) this.onConnecting({ code: 3 })
                this.sendUnpublished()
                debug(this.peerId, `++ NAT (type=${NAT.toString(this.natType)})`)

                if (this.onReady) this.onReady()
              }
            }

            this.reflectionId = null
            this.reflectionFirstResponder = null
          }, PROBE_WAIT)
        }

        this.address = packet.message.address
        this.port = packet.message.port
        debug(this.peerId, `++ NAT UPDATE STATE (address=${this.address}, port=${this.port})`)
      }
    }

    /**
     * Received an Intro Packet
     * @return {undefined}
     * @ignore
     */
    async _onIntro (packet, port, address) {
      this.metrics[packet.type]++

      if (packet.message.timestamp + this.config.keepalive < Date.now()) return
      if (packet.message.requesterPeerId === this.peerId) return // intro to myself?
      if (packet.message.responderPeerId === this.peerId) return // intro from myself?
      if (packet.hops > this.maxHops) return

      debug(this.peerId, '<- INTRO (' +
        `isRendezvous=${packet.message.isRendezvous}, ` +
        `from=${address}:${port}, ` +
        `to=${packet.message.address}:${packet.message.port}, ` +
        `clustering=${packet.clusterId.slice(0, 4)}/${packet.subclusterId.slice(0, 4)}` +
      ')')

      const { clusterId, subclusterId, clock } = packet

      // this is the peer that is being introduced to the new peers
      const peerId = packet.message.requesterPeerId
      const peerPort = packet.message.port
      const peerAddress = packet.message.address
      const natType = packet.message.natType
      // update the info about the peer that is introducing us
      // this.updatePeer({ peerId: packet.message.responderPeerId, port, address })

      // update the info about the peer we are being introduced to

      const peer = this.updatePeer({ peerId, natType, port: peerPort, address: peerAddress, clock, clusterId, subclusterId })
      if (!peer) return // not enough information provided to create a peer
      if (peer.connecting) return // already connecting

      const pingId = Math.random().toString(16).slice(2)
      const { hash } = await this.cache.summarize('')

      const props = {
        natType: this.natType,
        isConnection: true,
        pingId: packet.message.pingId,
        requesterPeerId: this.peerId
      }

      const strategy = NAT.connectionStrategy(this.natType, packet.message.natType)

      debug(this.peerId, `++ NAT INTRO (strategy=${NAT.toStringStrategy(strategy)}, from=${this.address}:${this.port} [${NAT.toString(this.natType)}], to=${packet.message.address}:${packet.message.port} [${NAT.toString(packet.message.natType)}])`)

      if (strategy === NAT.STRATEGY_TRAVERSAL_CONNECT) {
        if (peer.connecting) return

        debug(this.peerId, `## NAT CONNECT (from=${this.address}:${this.port}, to=${peerAddress}:${peerPort}, pingId=${pingId})`)

        this.ping(peer, true)

        const portCache = new Set()
        peer.connecting = true

        let i = 0

        if (!this.socketPool) {
          this.socketPool = Array.from({ length: 1024 }, () => dgram.createSocket('udp4', null, this))
        }

        // A probes 1 target port on B from 1024 source ports
        //   (this is 1.59% of the search clusterId)
        // B probes 256 target ports on A from 1 source port
        //   (this is 0.40% of the search clusterId)
        //
        // Probability of successful traversal: 98.35%
        //
        const interval = setInterval(async () => { // TODO should be this._setInterval
          // send messages until we receive a message from them. giveup after sending ±1024
          // packets and fall back to using the peer that sent this as the initial proxy.
          if (i++ >= 1024) {
            this._clearInterval(interval)
            clearInterval(interval)
            peer.connecting = false

            this._setTimeout(() => this._onIntro(packet, port, address), 2048)
            return false
          }

          // if we got a pong packet with an id that matches the id ping packet,
          // the peer will have that pingId on it so we should check for it.
          const pair = this.peers.find(p => p.pingId === pingId)

          if (pair) {
            pair.lastUpdate = Date.now()
            peer.connecting = false
            pair.connected = true
            this._onConnection(packet, pair, pair.port, pair.address)

            if (this.onJoin && this.clusters[packet.clusterId]) {
              this.onJoin(packet, peer, peerPort, peerAddress)
            }

            this._clearInterval(interval)
            clearInterval(interval)
            return false
          }

          const to = getRandomPort(portCache)
          const data = await Packet.encode(new PacketPing({
            message: {
              requesterPeerId: this.peerId,
              cacheSummaryHash: hash || null,
              natType: this.natType,
              uptime: this.uptime,
              isConnection: true,
              timestamp: Date.now(),
              pingId
            }
          }))

          const rand = () => Math.random() - 0.5
          const pooledSocket = this.socketPool.sort(rand).find(s => !s.socket)
          if (!pooledSocket) return // TODO recover from exausted socket pool

          try {
            pooledSocket.send(data, to, packet.message.address)
          } catch (err) {
          }

          pooledSocket.on('message', (...args) => {
            clearTimeout(pooledSocket.expire)
            const remote = this.peers.find(r => r.peerId === peerId)
            remote.socket = pooledSocket

            clearInterval(interval)
            this._onMessage(...args)
          })

          if (!pooledSocket.socket) {
            pooledSocket.expire = setTimeout(() => {
              if (pooledSocket._bindState === 2) {
                try {
                  pooledSocket.close()
                } catch (err) {}
              }
              this.socketPool[i] = dgram.createSocket('udp4', null, this)
            }, 2048)
          }
        }, 10)

        return
      }

      if (strategy === NAT.STRATEGY_PROXY && !peer.proxy) {
        // TODO could allow multiple proxies
        let proxy = this.peers.find(p => p.address === address && p.port === port)

        if (!proxy) {
          proxy = this.addPeer({ peerId: packet.message.responderPeerId, port, address, natType: NAT.UNRESTRICTED })
          debug(this.peerId, '++ INTRO CRAETED PROXY', packet.message.responderPeerId)
        }

        if (proxy) {
          peer.proxy = proxy
          debug(this.peerId, '++ INTRO CHOSE PROXY STRATEGY', peer.proxy.address)
        }
      }

      if (strategy === NAT.STRATEGY_TRAVERSAL_OPEN) {
        if (peer.opening) return
        peer.opening = true

        if (!this.bdpCache.length) {
          globalThis.bdpCache = this.bdpCache = Array.from({ length: 256 }, () => getRandomPort())
        }

        this.ping(peer, true, props)

        for (const port of this.bdpCache) {
          const data = await Packet.encode(new PacketPing({
            message: {
              requesterPeerId: this.peerId,
              cacheSummaryHash: hash || null,
              natType: this.natType,
              uptime: this.uptime,
              isConnection: true,
              timestamp: Date.now()
            }
          }))
          this.send(data, port, packet.message.address)
        }

        return
      }

      if (strategy === NAT.STRATEGY_DIRECT_CONNECT) {
        peer.connected = true // TODO this could be req/res
        debug(this.peerId, '++ NAT STRATEGY_DIRECT_CONNECT')
      }

      if (strategy === NAT.STRATEGY_DEFER) {
        peer.connected = true // TODO this could be req/res
        debug(this.peerId, '++ NAT STRATEGY_DEFER')
      }

      this.ping(peer, true, props)
      if (packet.hops === 1) this._onConnection(packet, peer, port, address)

      if (this.onJoin && this.clusters[packet.clusterId]) {
        this.onJoin(packet, peer, port, address)
      }
    }

    /**
     * Received an Join Packet
     * @return {undefined}
     * @ignore
     */
    async _onJoin (packet, port, address, data) {
      this.metrics[packet.type]++

      if (packet.message.requesterPeerId === this.peerId) return
      if (!packet.clusterId) return

      this.lastUpdate = Date.now()

      const peerId = packet.message.requesterPeerId
      const natType = packet.message.natType
      const subclusterId = packet.subclusterId
      const clusterId = packet.clusterId
      const clock = packet.clock
      const peerAddress = packet.message.address
      const peerPort = packet.message.port

      debug(this.peerId, '<- JOIN (' +
        `peerId=${peerId.slice(0, 6)}, ` +
        `clock=${packet.clock}, ` +
        `hops=${packet.hops}, ` +
        `clusterId=${packet.clusterId}, ` +
        `address=${address}:${port})`
      )

      this.updatePeer({
        peerId,
        natType,
        port: natType === NAT.ENDPOINT_RESTRICTED ? port : packet.message.port,
        clock,
        connected: packet.hops === 1,
        address: peerAddress,
        subclusterId,
        clusterId
      })

      const filter = p => p.connected || p.natType === NAT.UNRESTRICTED // you can't intro a peer who aren't connecteds
      let peers = this.getPeers(packet, this.peers, [{ port, address }], filter)

      //
      // This packet represents a peer who wants to join the network and is a
      // member of our cluster. The packet was replicated though the network
      // and contains the details about where the peer can be reached, in this
      // case we want to ping that peer so we can be introduced to them.
      //
      if (packet.message.rendezvousDeadline && !packet.message.rendezvousRequesterPeerId) {
        if (packet.message.rendezvousDeadline > Date.now() && this.clusters[packet.clusterId]) {
          // TODO it would tighten up the transition time between dropped peers
          // if we check strategy from (packet.message.natType, this.natType) and
          // make introductions that create more mutually known peers.
          debug(this.peerId, '<- INTRO JOIN RENDEZVOUS INTERCEPTED')

          const data = await Packet.encode(new PacketJoin({
            clock: packet.clock,
            subclusterId: packet.subclusterId,
            clusterId: packet.clusterId,
            message: {
              requesterPeerId: this.peerId,
              natType: this.natType,
              address: this.address,
              port: this.port,
              rendezvousType: packet.message.natType,
              rendezvousRequesterPeerId: packet.message.requesterPeerId
            }
          }))

          this.send(
            data,
            packet.message.rendezvousPort,
            packet.message.rendezvousAddress
          )
        }
      }

      //
      // A peer who belongs to the same cluster as the peer who's replicated
      // join was discovered, sent us a join that has a specification for who
      // they want to be introduced to.
      //
      if (packet.message.rendezvousRequesterPeerId && this.peerId === packet.message.rendezvousPeerId) {
        const peer = this.peers.find(p => p.peerId === packet.message.rendezvousRequesterPeerId)

        if (!peer) {
          debug(this.peerId, '<- INTRO JOIN RENDEZVOUS FAILED', packet)
          return
        }

        peer.natType = packet.message.rendezvousType
        peers = [peer]

        debug(this.peerId, '<- INTRO JOIN RENDEZVOUS')
      }

      for (const peer of peers) {
        if (peer.peerId === packet.message.requesterPeerId) continue

        const message1 = {
          requesterPeerId: peer.peerId,
          responderPeerId: this.peerId,
          isRendezvous: !!packet.message.rendezvousPeerId,
          natType: peer.natType,
          address: peer.address,
          port: peer.port
        }

        const message2 = {
          requesterPeerId: packet.message.requesterPeerId,
          responderPeerId: this.peerId,
          isRendezvous: !!packet.message.rendezvousPeerId,
          natType: packet.message.natType,
          address: packet.message.address,
          port: packet.message.port
        }

        const opts1 = {
          hops: packet.hops + 1
        }

        if (peer.clusters && peer.clusters[clusterId]) {
          opts1.clusterId = clusterId
          if (peer.clusters[clusterId][subclusterId]) opts1.subclusterId = subclusterId
        }

        const opts2 = {
          hops: packet.hops + 1,
          clusterId,
          subclusterId
        }

        const intro1 = await Packet.encode(new PacketIntro({ ...opts1, message: message1 }))
        const intro2 = await Packet.encode(new PacketIntro({ ...opts2, message: message2 }))

        //
        // Send intro1 to the peer described in the message
        // Send intro2 to the peer in this loop
        //
        debug(this.peerId, `>> INTRO SEND (from=${peer.address}:${peer.port}, to=${packet.message.address}:${packet.message.port})`)
        debug(this.peerId, `>> INTRO SEND (from=${packet.message.address}:${packet.message.port}, to=${peer.address}:${peer.port})`)

        this.send(intro2, peer.port, peer.address)
        this.send(intro1, packet.message.port, packet.message.address)
      }

      if (this.indexed && !packet.clusterId) return
      if (packet.hops > this.maxHops) return

      if (this.natType === NAT.UNRESTRICTED && !packet.message.rendezvousDeadline) {
        packet.message.rendezvousAddress = this.address
        packet.message.rendezvousPort = this.port
        packet.message.rendezvousType = this.natType
        packet.message.rendezvousPeerId = this.peerId
        packet.message.rendezvousDeadline = Date.now() + this.config.keepalive
      }

      debug(this.peerId, `-> JOIN RELAY (peerId=${peerId.slice(0, 6)}, from=${peerAddress}:${peerPort})`)
      this.mcast(packet, [{ port, address }, { port: peerPort, address: peerAddress }])
      this.controlPackets.set(packet.packetId, 2)
    }

    /**
     * Received an Publish Packet
     * @return {undefined}
     * @ignore
     */
    async _onPublish (packet, port, address, data) {
      this.metrics[packet.type]++

      debug(this.peerId, '<- PUBLISH', packet.clusterId, packet.subclusterId, port, address)
      // const subclusterId = this.clusters[packet.clusterId]

      // only cache if this packet if i am part of this subclusterId
      // if (subclusterId && subclusterId[packet.subclusterId]) {
      if (this.cache.has(packet.packetId)) return
      this.cache.insert(packet.packetId, packet)

      const ignorelist = [{ address, port }]

      if (!this.indexed && this.encryption.has(packet.subclusterId)) {
        let p = { ...packet }
        if (p.index > -1) p = await this.cache.compose(p)

        if (p) {
          if (this.onPacket && p.index === -1) this.onPacket(p, port, address)
        } else {
          // if we cant find the packet, sync with some more peers
          for (const peer of this.getPeers(packet, this.peers, ignorelist)) {
            // const peer = this.peers.find(p => p.address === address && p.port === port)
            if (peer) this.sync(peer, packet, peer.port, peer.address)
          }
        }
      }
      // }

      if (packet.hops > this.maxHops) return
      if (this.onMulticast) this.onMulticast(packet)
      this.mcast(packet, ignorelist)
    }

    /**
     * Received an Stream Packet
     * @return {undefined}
     * @ignore
     */
    async _onStream (packet, port, address, data) {
      this.metrics[packet.type]++

      const { streamTo, streamFrom } = packet

      // only help packets with a higher hop count if they are in our cluster
      // if (packet.hops > 2 && !this.clusters[packet.cluster]) return

      const peerFrom = this.peers.find(p => p.peerId === streamFrom)
      if (!peerFrom) return

      // stream message is for this peer
      if (streamTo === this.peerId) {
        if (this.encryption.has(packet.subclusterId)) {
          let p = Packet.from(packet) // clone the packet so it's not modified

          if (p.index > -1) { // if it needs to be composed...
            packet.timestamp = Date.now()
            this.streamBuffer.set(packet.packetId, packet) // cache the partial

            p = await this.cache.compose(p, this.streamBuffer) // try to compose
            if (!p) return // could not compose

            if (p) { // if successful, delete the artifacts
              const previousId = packet.index === 0 ? packet.packetId : packet.previousId

              this.streamBuffer.forEach((v, k) => {
                if (k === previousId) this.streamBuffer.delete(k)
                if (v.previousId === previousId) this.streamBuffer.delete(k)
              })
            }
          }

          if (this.onStream) this.onStream(p, peerFrom, port, address)
        }

        return
      }

      // stream message is for another peer
      const peerTo = this.peers.find(p => p.peerId === streamTo)
      if (!peerTo) {
        debug(this.peerId, `XX STREAM RELAY FORWARD DESTINATION NOT REACHABLE (to=${streamTo})`)
        return
      }

      if (packet.hops > this.maxHops) {
        debug(this.peerId, `XX STREAM RELAY MAX HOPS EXCEEDED (to=${streamTo})`)
        return
      }

      debug(this.peerId, `>> STREAM RELAY (to=${peerTo.address}:${peerTo.port}, id=${peerTo.peerId.slice(0, 6)})`)
      this.send(await Packet.encode(packet), peerTo.port, peerTo.address)
      if (packet.hops <= 2 && this.natType === NAT.UNRESTRICTED) this.mcast(packet)
    }

    /**
     * Received any packet on the probe port to determine the firewall:
     * are you port restricted, host restricted, or unrestricted.
     * @return {undefined}
     * @ignore
     */
    _onProbeMessage (data, { port, address }) {
      this._clearTimeout(this.probeReflectionTimeout)

      const packet = Packet.decode(data)
      if (packet?.type !== 2) return

      if (this.controlPackets.has(packet.packetId)) return
      this.controlPackets.set(packet.packetId, 1)

      const { reflectionId } = packet.message
      debug(this.peerId, `<- NAT PROBE (from=${address}:${port}, stage=${this.reflectionStage}, id=${reflectionId})`)

      if (this.onProbe) this.onProbe(data, port, address)
      if (this.reflectionId !== reflectionId || !this.reflectionId) return

      // reflection stage is encoded in the last hex char of the reflectionId, or 0 if not available.
      // const reflectionStage = reflectionId ? parseInt(reflectionId.slice(-1), 16) : 0

      if (this.reflectionStage === 1) {
        debug(this.peerId, '<- NAT REFLECT - STAGE1: probe received', reflectionId)
        if (!packet.message?.port) return // message must include a port number

        // successfully discovered the probe socket external port
        this.config.probeExternalPort = packet.message.port

        // move to next reflection stage
        this.reflectionStage = 1
        this.reflectionId = null
        this.requestReflection()
        return
      }

      if (this.reflectionStage === 2) {
        debug(this.peerId, '<- NAT REFLECT - STAGE2: probe received', reflectionId)

        // find the peer who sent the probe
        const probeFrom = this.peers.find(p => p.peerId === packet.message.responderPeerId)
        if (!probeFrom) return

        // if we have previously sent an outbount message to this peer on the probe port
        // then our NAT will have a mapping for their IP, but not their IP+Port.
        if (probeFrom.probed) {
          this.nextNatType |= NAT.FIREWALL_ALLOW_KNOWN_IP
          debug(this.peerId, `<> PROBE STATUS: NAT.FIREWALL_ALLOW_KNOWN_IP (${packet.message.port} -> ${this.nextNatType})`)
        } else {
          this.nextNatType |= NAT.FIREWALL_ALLOW_ANY
          debug(this.peerId, `<> PROBE STATUS: NAT.FIREWALL_ALLOW_ANY (${packet.message.port} -> ${this.nextNatType})`)
        }

        // wait for all messages to arrive
      }
    }

    /**
     * When a packet is received it is decoded, the packet contains the type
     * of the message. Based on the message type it is routed to a function.
     * like WebSockets, don't answer queries unless we know its another SRP peer.
     *
     * @param {Buffer|Uint8Array} data
     * @param {{ port: number, address: string }} info
     */
    async _onMessage (data, { port, address }) {
      const packet = Packet.decode(data)
      if (!packet || packet.version < VERSION) return

      //
      // If we get a duplicate intro or pong we can throw it away. possibly same
      // goes for query and sync packets. But we may get lots of pings from a peer
      // who is trying to connect with us, so we allow the limiter to handle the
      // alotment.
      //
      if (this.indexed || packet.type === PacketIntro.type || packet.type === PacketPong.type) {
        if (this.controlPackets.has(packet.packetId)) return
        this.controlPackets.set(packet.packetId, 1)
      }

      // debug('<- PACKET', packet.type, port, address)
      const clusters = this.clusters[packet.clusterId]
      const subclusterId = clusters && clusters[packet.subclusterId]

      if (!this.config.limitExempt) {
        if (rateLimit(this.rates, packet.type, port, address, subclusterId)) {
          debug(this.peerId, `XX RATE LIMIT HIT (from=${address}, type=${packet.type})`)
          return
        }
        if (this.onLimit && !this.onLimit(packet, port, address)) return
      }

      const args = [packet, port, address, data]

      if (this.firewall) if (!this.firewall(...args)) return
      if (this.onData) this.onData(...args)

      switch (packet.type) {
        case PacketPing.type: return this._onPing(...args)
        case PacketPong.type: return this._onPong(...args)
      }

      const peer = this.peers.find(p => p.address === address && p.port === port)
      if (peer && packet.hops === 1) peer.lastUpdate = Date.now()

      if (!this.natType && !this.indexed) return

      switch (packet.type) {
        case PacketIntro.type: return this._onIntro(...args)
        case PacketJoin.type: return this._onJoin(...args)
        case PacketPublish.type: return this._onPublish(...args)
        case PacketStream.type: return this._onStream(...args)
        case PacketSync.type: return this._onSync(...args)
        case PacketQuery.type: return this._onQuery(...args)
      }
    }
  }

  return Peer
}

export default wrap
