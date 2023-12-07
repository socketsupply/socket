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
const process = globalThis.process || window.__args
const COLOR_GRAY = '\x1b[90m'
const COLOR_WHITE = '\x1b[37m'
const COLOR_RESET = '\x1b[0m'

export const debug = (pid, ...args) => {
  if (!process.env.DEBUG) return

  const output = COLOR_GRAY +
    String(logcount++).padStart(6) + ' │ ' + COLOR_WHITE +
    pid.slice(0, 4) + ' ' + args.join(' ') + COLOR_RESET

  if (new RegExp(process.env.DEBUG).test(output)) console.log(output)
}

export { Packet, sha256, Cache, Encryption, NAT }

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

const PEERID_REGEX = /^([A-Fa-f0-9]{2}){32}$/

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
 * Computes rate limit predicate value for a port and address pair for a given
 * threshold updating an input rates map. This method is accessed concurrently,
 * the rates object makes operations atomic to avoid race conditions.
 *
 * @param {Map} rates
 * @param {number} type
 * @param {number} port
 * @param {string} address
 * @return {boolean}
 */
export function rateLimit (rates, type, port, address, subclusterIdQuota) {
  const R = isReplicatable(type)
  const key = (R ? 'R' : 'C') + ':' + address + ':' + port
  const quota = subclusterIdQuota || (R ? 512 : 4096)
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
  opening = 0
  probed = 0
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

    if (!o.peerId) throw new Error('expected .peerId')
    if (o.indexed) o.natType = NAT.UNRESTRICTED
    if (o.natType && !NAT.isValid(o.natType)) throw new Error('invalid .natType')

    const cid = o.clusterId?.toString('base64')
    const scid = o.subclusterId?.toString('base64')

    if (cid && scid) {
      this.clusters[cid] = { [scid]: { rateLimit: MAX_BANDWIDTH } }
    }

    Object.assign(this, o)
  }

  async write (sharedKey, args) {
    let rinfo = this
    if (this.proxy) rinfo = this.proxy

    const keys = await Encryption.createKeyPair(sharedKey)

    args.subclusterId = Buffer.from(keys.publicKey)
    args.clusterId = Buffer.from(this.localPeer.clusterId, 'base64')
    args.usr3 = Buffer.from(this.peerId, 'hex')
    args.usr4 = Buffer.from(this.localPeer.peerId, 'hex')
    args.message = this.localPeer.encryption.seal(args.message, keys)

    const packets = await this.localPeer._message2packets(PacketStream, args.message, args)

    if (this.proxy) {
      debug(this.localPeer.peerId, `>> WRITE STREAM HAS PROXY ${this.proxy.address}:${this.proxy.port}`)
    }

    for (const packet of packets) {
      const from = this.localPeer.peerId.slice(0, 6)
      const to = this.peerId.slice(0, 6)
      debug(this.localPeer.peerId, `>> WRITE STREAM (from=${from}, to=${to}, via=${rinfo.address}:${rinfo.port})`)

      this.localPeer.gate.set(Buffer.from(packet.packetId).toString('hex'), 1)
      await this.localPeer.send(await Packet.encode(packet), rinfo.port, rinfo.address, this.socket)
    }

    return packets
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
    lastSync = 0
    closing = false
    clock = 0
    unpublished = {}
    cache = null
    uptime = 0
    maxHops = 16
    bdpCache = /** @type {number[]} */ ([])

    onListening = null
    onDelete = null

    sendQueue = []
    firewall = null
    rates = new Map()
    streamBuffer = new Map()
    gate = new Map()
    returnRoutes = new Map()

    metrics = {
      i: { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0, 5: 0, 6: 0, 7: 0, 8: 0, REJECTED: 0 },
      o: { 0: 0, 1: 0, 2: 0, 3: 0, 4: 0, 5: 0, 6: 0, 7: 0, 8: 0 }
    }

    peers = JSON.parse(/* snapshot_start=1691579150299, filter=easy,static */`
      [{"address":"44.213.42.133","port":10885,"peerId":"4825fe0475c44bc0222e76c5fa7cf4759cd5ef8c66258c039653f06d329a9af5","natType":31,"indexed":true},{"address":"107.20.123.15","port":31503,"peerId":"2de8ac51f820a5b9dc8a3d2c0f27ccc6e12a418c9674272a10daaa609eab0b41","natType":31,"indexed":true},{"address":"54.227.171.107","port":43883,"peerId":"7aa3d21ceb527533489af3888ea6d73d26771f30419578e85fba197b15b3d18d","natType":31,"indexed":true},{"address":"54.157.134.116","port":34420,"peerId":"1d2315f6f16e5f560b75fbfaf274cad28c12eb54bb921f32cf93087d926f05a9","natType":31,"indexed":true},{"address":"184.169.205.9","port":52489,"peerId":"db00d46e23d99befe42beb32da65ac3343a1579da32c3f6f89f707d5f71bb052","natType":31,"indexed":true},{"address":"35.158.123.13","port":31501,"peerId":"4ba1d23266a2d2833a3275c1d6e6f7ce4b8657e2f1b8be11f6caf53d0955db88","natType":31,"indexed":true},{"address":"3.68.89.3","port":22787,"peerId":"448b083bd8a495ce684d5837359ce69d0ff8a5a844efe18583ab000c99d3a0ff","natType":31,"indexed":true},{"address":"3.76.100.161","port":25761,"peerId":"07bffa90d89bf74e06ff7f83938b90acb1a1c5ce718d1f07854c48c6c12cee49","natType":31,"indexed":true},{"address":"3.70.241.230","port":61926,"peerId":"1d7ee8d965794ee286ac425d060bab27698a1de92986dc6f4028300895c6aa5c","natType":31,"indexed":true},{"address":"3.70.160.181","port":41141,"peerId":"707c07171ac9371b2f1de23e78dad15d29b56d47abed5e5a187944ed55fc8483","natType":31,"indexed":true},{"address":"3.122.250.236","port":64236,"peerId":"a830615090d5cdc3698559764e853965a0d27baad0e3757568e6c7362bc6a12a","natType":31,"indexed":true},{"address":"18.130.98.23","port":25111,"peerId":"ba483c1477ab7a99de2d9b60358d9641ff6a6dc6ef4e3d3e1fc069b19ac89da4","natType":31,"indexed":true},{"address":"13.42.10.247","port":2807,"peerId":"032b79de5b4581ee39c6d15b12908171229a8eb1017cf68fd356af6bbbc21892","natType":31,"indexed":true},{"address":"18.229.140.216","port":36056,"peerId":"73d726c04c05fb3a8a5382e7a4d7af41b4e1661aadf9020545f23781fefe3527","natType":31,"indexed":true}]
    `/* snapshot_end=1691579150299 */).map((/** @type {object} */ o) => new RemotePeer({ ...o, indexed: true }, this))

    /**
     * `Peer` class constructor. Avoid calling this directly (use the create method).
     * @private
     * @param {object?} [persistedState]
     */
    constructor (persistedState = {}) {
      const config = persistedState?.config ?? persistedState ?? {}

      this.encryption = new Encryption()

      if (!config.peerId) throw new Error('constructor expected .peerId')
      if (typeof config.peerId !== 'string' || !PEERID_REGEX.test(config.peerId)) throw new Error('invalid .peerId')

      this.config = {
        keepalive: DEFAULT_KEEP_ALIVE,
        ...config
      }

      let cacheData

      if (persistedState?.data?.length > 0) {
        cacheData = new Map(persistedState.data)
      }

      this.cache = new Cache(cacheData, config.siblingResolver)
      this.cache.onEjected = p => this.mcast(p)

      this.unpublished = persistedState?.unpublished || {}
      this._onError = err => this.onError && this.onError(err)

      Object.assign(this, config)

      if (!this.indexed && !this.clusterId) throw new Error('constructor expected .clusterId')
      if (typeof this.peerId !== 'string') throw new Error('peerId should be of type string')

      this.port = config.port || null
      this.natType = config.natType || null
      this.address = config.address || null

      this.socket = dgram.createSocket('udp4', null, this)
      this.probeSocket = dgram.createSocket('udp4', null, this).unref()

      const isRecoverable = err =>
        err.code === 'ECONNRESET' ||
        err.code === 'ECONNREFUSED' ||
        err.code === 'EADDRINUSE' ||
        err.code === 'ETIMEDOUT'

      this.socket.on('error', err => isRecoverable(err) && this._listen())
      this.probeSocket.on('error', err => isRecoverable(err) && this._listen())
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

      this.socket.removeAllListeners()
      this.probeSocket.removeAllListeners()

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
        await this.ping(peer, false, { message: { requesterPeerId: this.peerId } })
      }

      this._mainLoop(Date.now())
      this.mainLoopTimer = this._setInterval(ts => this._mainLoop(ts), this.config.keepalive)

      if (this.indexed && this.onReady) return this.onReady()
    }

    /**
     * Continuously evaluate the state of the peer and its network
     * @return {undefined}
     * @ignore
     */
    async _mainLoop (ts) {
      if (this.closing) return this._clearInterval(this.mainLoopTimer)

      const offline = globalThis.navigator && !globalThis.navigator.onLine
      if (offline) {
        if (this.onConnecting) this.onConnecting({ code: -2, status: 'Offline' })
        return true
      }

      if (!this.reflectionId) this.requestReflection()
      if (this.onInterval) this.onInterval()

      this.uptime += this.config.keepalive

      // wait for nat type to be discovered
      if (!NAT.isValid(this.natType)) return true

      for (const [k, packet] of [...this.cache.data]) {
        const p = Packet.from(packet)
        if (!p) continue
        if (!p.timestamp) p.timestamp = ts
        const clusterId = p.clusterId.toString('base64')

        const mult = this.clusters[clusterId] ? 2 : 1
        const ttl = (p.ttl < Packet.ttl) ? p.ttl : Packet.ttl * mult
        const deadline = p.timestamp + ttl

        if (deadline <= ts) {
          if (p.hops < this.maxHops) this.mcast(p)
          this.cache.delete(k)
          debug(this.peerId, '-- DELETE', k, this.cache.size)
          if (this.onDelete) this.onDelete(p)
        }
      }

      for (let [k, v] of this.gate.entries()) {
        v -= 1
        if (!v) this.gate.delete(k)
        else this.gate.set(k, v)
      }

      for (let [k, v] of this.returnRoutes.entries()) {
        v -= 1
        if (!v) this.returnRoutes.delete(k)
        else this.returnRoutes.set(k, v)
      }

      // prune peer list
      for (const [i, peer] of Object.entries(this.peers)) {
        if (peer.indexed) continue
        const expired = (peer.lastUpdate + this.config.keepalive) < Date.now()
        if (expired) { // || !NAT.isValid(peer.natType)) {
          const p = this.peers.splice(i, 1)
          if (this.onDisconnect) this.onDisconnect(p)
          continue
        }
      }

      // heartbeat
      const { hash } = await this.cache.summarize('', this.cachePredicate)
      for (const [, peer] of Object.entries(this.peers)) {
        this.ping(peer, false, {
          message: {
            requesterPeerId: this.peerId,
            natType: this.natType,
            cacheSummaryHash: hash || null,
            cacheSize: this.cache.size
          }
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
     * Enqueue packets to be sent to the network
     * @param {Buffer} data - An encoded packet
     * @param {number} port - The desination port of the remote host
     * @param {string} address - The destination address of the remote host
     * @param {Socket=this.socket} socket - The socket to send on
     * @return {undefined}
     * @ignore
     */
    send (data, port, address, socket = this.socket) {
      this.sendQueue.push({ data, port, address, socket })
      this._scheduleSend()
    }

    /**
     * @private
     */
    _scheduleSend () {
      if (this.sendTimeout) this._clearTimeout(this.sendTimeout)
      this.sendTimeout = this._setTimeout(() => { this._dequeue() })
    }

    /**
     * @private
     */
    _dequeue () {
      if (!this.sendQueue.length) return
      const { data, port, address, socket } = this.sendQueue.shift()

      socket.send(data, port, address, err => {
        if (this.sendQueue.length) this._scheduleSend()
        if (err) return this._onError(err)

        const packet = Packet.decode(data)
        if (!packet) return

        this.metrics.o[packet.type]++
        delete this.unpublished[packet.packetId.toString('hex')]
        if (this.onSend && packet.type) this.onSend(packet, port, address)
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

        if (!packet) { // it may have been purged already
          delete this.unpublished[packetId]
          continue
        }

        await this.mcast(packet)
        debug(this.peerId, `-> RESEND (packetId=${packetId})`)
        if (this.onState) await this.onState(this.getState())
      }
    }

    /**
     * Get the serializable state of the peer (can be passed to the constructor or create method)
     * @return {undefined}
     */
    getState () {
      this.config.clock = this.clock // save off the clock

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
      const rand = () => Math.random() - 0.5

      const base = p => {
        if (ignorelist.findIndex(ilp => (ilp.port === p.port) && (ilp.address === p.address)) > -1) return false
        if (p.lastUpdate === 0) return false
        if (p.lastUpdate < Date.now() - (this.config.keepalive * 4)) return false
        if (this.peerId === p.peerId) return false // same as me
        if (packet.message.requesterPeerId === p.peerId) return false // same as requester - @todo: is this true in all cases?
        if (!p.port || !NAT.isValid(p.natType)) return false
        return true
      }

      const candidates = peers
        .filter(filter)
        .filter(base)
        .sort(rand)

      const list = candidates.slice(0, 3)

      if (!list.some(p => p.indexed)) {
        const indexed = candidates.filter(p => p.indexed && !list.includes(p))
        if (indexed.length) list.push(indexed[0])
      }

      const clusterId = packet.clusterId.toString('base64')
      const friends = candidates.filter(p => p.clusters && p.clusters[clusterId] && !list.includes(p))
      if (friends.length) {
        list.unshift(friends[0])
        list.unshift(...candidates.filter(c => c.address === friends[0].address && c.peerId === friends[0].peerId))
      }

      return list
    }

    /**
     * Send an eventually consistent packet to a selection of peers (fanout)
     * @return {undefined}
     * @ignore
     */
    async mcast (packet, ignorelist = []) {
      const peers = this.getPeers(packet, this.peers, ignorelist)
      const pid = packet.packetId.toString('hex')

      packet.hops += 1

      for (const peer of peers) {
        this.send(await Packet.encode(packet), peer.port, peer.address)
      }

      if (this.onMulticast) this.onMulticast(packet)
      if (this.gate.has(pid)) return
      this.gate.set(pid, 1)
    }

    /**
     * The process of determining this peer's NAT behavior (firewall and dependentness)
     * @return {undefined}
     * @ignore
     */
    async requestReflection () {
      if (this.closing || this.indexed || this.reflectionId) {
        debug(this.peerId, '<> REFLECT ABORTED', this.reflectionId)
        return
      }

      if (this.natType && (this.lastUpdate > 0 && (Date.now() - this.config.keepalive) < this.lastUpdate)) {
        debug(this.peerId, `<> REFLECT NOT NEEDED (last-recv=${Date.now() - this.lastUpdate}ms)`)
        return
      }

      debug(this.peerId, '-> REQ REFLECT', this.reflectionId, this.reflectionStage)
      if (this.onConnecting) this.onConnecting({ code: -1, status: `Entering reflection (lastUpdate ${Date.now() - this.lastUpdate}ms)` })

      const peers = [...this.peers]
        .filter(p => p.lastUpdate !== 0)
        .filter(p => p.natType === NAT.UNRESTRICTED || p.natType === NAT.ADDR_RESTRICTED || p.indexed)

      if (peers.length < 2) {
        if (this.onConnecting) this.onConnecting({ code: -1, status: 'Not enough pingable peers' })
        debug(this.peerId, 'XX REFLECT NOT ENOUGH PINGABLE PEERS - RETRYING')
        return this._setTimeout(() => this.requestReflection(), 256)
      }

      const requesterPeerId = this.peerId
      const opts = { requesterPeerId, isReflection: true }

      this.reflectionId = opts.reflectionId = randomBytes(6).toString('hex').padStart(12, '0')

      if (this.onConnecting) {
        this.onConnecting({ code: 0.5, status: `Found ${peers.length} elegible peers for reflection` })
      }
      //
      // # STEP 1
      // The purpose of this step is strictily to discover the external port of
      // the probe socket.
      //
      if (this.reflectionStage === 0) {
        if (this.onConnecting) this.onConnecting({ code: 1, status: 'Discover External Port' })
        // start refelection with an zeroed NAT type
        if (this.reflectionTimeout) this._clearTimeout(this.reflectionTimeout)
        this.reflectionStage = 1

        debug(this.peerId, '-> NAT REFLECT - STAGE1: A', this.reflectionId)
        const list = peers.filter(p => p.probed).sort(() => Math.random() - 0.5)
        const peer = list.length ? list[0] : peers[0]
        peer.probed = Date.now() // mark this peer as being used to provide port info
        this.ping(peer, false, { message: { ...opts, isProbe: true } }, this.probeSocket)

        // we expect onMessageProbe to fire and clear this timer or it will timeout
        this.probeReflectionTimeout = this._setTimeout(() => {
          this.probeReflectionTimeout = null
          if (this.reflectionStage !== 1) return
          debug(this.peerId, 'XX NAT REFLECT - STAGE1: C - TIMEOUT', this.reflectionId)
          if (this.onConnecting) this.onConnecting({ code: 1, status: 'Timeout' })

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
      // The purpose of step 2 is twofold:
      //
      // 1) ask two different peers for the external port and address for our primary socket.
      // If they are different, we can determine that our NAT is a `ENDPOINT_DEPENDENT`.
      //
      // 2) ask the peers to also reply to our probe socket from their probe socket.
      // These packets will both be dropped for `FIREWALL_ALLOW_KNOWN_IP_AND_PORT` and will both
      // arrive for `FIREWALL_ALLOW_ANY`. If one packet arrives (which will always be from the peer
      // which was previously probed), this indicates `FIREWALL_ALLOW_KNOWN_IP`.
      //
      if (this.reflectionStage === 1) {
        this.reflectionStage = 2
        const { probeExternalPort } = this.config
        if (this.onConnecting) this.onConnecting({ code: 1.5, status: 'Discover NAT' })

        // peer1 is the most recently probed (likely the same peer used in step1)
        // using the most recent guarantees that the the NAT mapping is still open
        const peer1 = peers.filter(p => p.probed).sort((a, b) => b.probed - a.probed)[0]

        // peer has NEVER previously been probed
        const peer2 = peers.filter(p => !p.probed).sort(() => Math.random() - 0.5)[0]

        if (!peer1 || !peer2) {
          debug(this.peerId, 'XX NAT REFLECT - STAGE2: INSUFFICENT PEERS - RETRYING')
          if (this.onConnecting) this.onConnecting({ code: 1.5, status: 'Insufficent Peers' })
          return this._setTimeout(() => this.requestReflection(), 256)
        }

        debug(this.peerId, '-> NAT REFLECT - STAGE2: START', this.reflectionId)

        // reset reflection variables to defaults
        this.nextNatType = NAT.UNKNOWN
        this.reflectionFirstResponder = null

        this.ping(peer1, false, { message: { ...opts, probeExternalPort } })
        this.ping(peer2, false, { message: { ...opts, probeExternalPort } })

        if (this.onConnecting) {
          this.onConnecting({ code: 2, status: `Requesting reflection from ${peer1.address}` })
          this.onConnecting({ code: 2, status: `Requesting reflection from ${peer2.address}` })
        }

        if (this.reflectionTimeout) {
          this._clearTimeout(this.reflectionTimeout)
          this.reflectionTimeout = null
        }

        this.reflectionTimeout = this._setTimeout(ts => {
          this.reflectionTimeout = null
          if (this.reflectionStage !== 2) return
          if (this.onConnecting) this.onConnecting({ code: 2, status: 'Timeout' })
          this.reflectionStage = 1
          this.reflectionId = null
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

      props.message.requesterPeerId = this.peerId
      props.message.uptime = this.uptime
      props.message.timestamp = Date.now()

      const packet = new PacketPing(props)
      const data = await Packet.encode(packet)

      const send = async () => {
        if (this.closing) return false

        const p = this.peers.find(p => p.peerId === peer.peerId)
        // if (p?.reflectionId && p.reflectionId === packet.message.reflectionId) {
        //  return false
        // }

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

    getPeer (id) {
      return this.peers.find(p => p.peerId === id)
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

      if (!this.port || !this.natType) return

      args.sharedKey = sharedKey

      const clusterId = args.clusterId || this.config.clusterId
      const subclusterId = Buffer.from(keys.publicKey)

      const cid = clusterId?.toString('base64')
      const scid = subclusterId?.toString('base64')

      this.clusters[cid] ??= {}
      this.clusters[cid][scid] = args

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

      debug(this.peerId, `-> JOIN (clusterId=${cid}, subclusterId=${scid}, clock=${packet.clock}/${this.clock})`)
      if (this.onState) await this.onState(this.getState())
      this.mcast(packet)
      this.gate.set(packet.packetId.toString('hex'), 1)
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
        .filter(p => p?.previousId?.toString('hex') === packet?.packetId?.toString('hex'))

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
          ts: Date.now(),
          size: message.length,
          indexes: Math.ceil(message.length / 1024)
        }]
        let pos = 0
        while (pos < message.length) messages.push(message.slice(pos, pos += 1024))
      }

      // turn each message into an actual packet
      const packets = messages.map(message => new T({
        ...args,
        clusterId,
        subclusterId,
        clock,
        message,
        usr1,
        usr2,
        usr3: args.usr3,
        usr4: args.usr4,
        sig
      }))

      if (packet) packets[0].previousId = packet.packetId
      if (nextId) packets[packets.length - 1].nextId = nextId

      // set the .packetId (any maybe the .previousId and .nextId)
      for (let i = 0; i < packets.length; i++) {
        if (packets.length > 1) packets[i].index = i

        if (i === 0) {
          packets[0].packetId = await sha256(packets[0].message, { bytes: true })
        } else {
          // all fragments will have the same previous packetId
          // the index is used to stitch them back together in order.
          packets[i].previousId = packets[0].packetId
        }

        if (packets[i + 1]) {
          packets[i + 1].packetId = await sha256(
            Buffer.concat([
              await sha256(packets[i].packetId, { bytes: true }),
              await sha256(packets[i + 1].message, { bytes: true })
            ]),
            { bytes: true }
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

      args.subclusterId = Buffer.from(keys.publicKey)
      args.clusterId = args.clusterId || this.config.clusterId

      const message = this.encryption.seal(args.message, keys)
      const packets = await this._message2packets(PacketPublish, message, args)

      for (const packet of packets) {
        const p = Packet.from(packet)
        this.cache.insert(packet.packetId.toString('hex'), p)

        if (this.onPacket) this.onPacket(p, this.port, this.address, true)

        this.unpublished[packet.packetId.toString('hex')] = Date.now()
        if (globalThis.navigator && !globalThis.navigator.onLine) continue

        this.mcast(packet)
      }

      return packets
    }

    /**
     * @return {undefined}
     */
    async sync (peer) {
      const rinfo = peer?.proxy || peer

      this.lastSync = Date.now()
      const summary = await this.cache.summarize('', this.cachePredicate)

      debug(this.peerId, `-> SYNC START (dest=${peer.peerId.slice(0, 8)}, to=${rinfo.address}:${rinfo.port})`)
      if (this.onSyncStart) this.onSyncStart(peer, rinfo.port, rinfo.address)

      // if we are out of sync send our cache summary
      const data = await Packet.encode(new PacketSync({
        message: Cache.encodeSummary(summary)
      }))

      this.send(data, rinfo.port, rinfo.address, peer.socket)
    }

    close () {
      this._clearInterval(this.mainLoopTimer)

      if (this.closing) return

      this.closing = true
      this.socket.close()

      if (this.onClose) this.onClose()
    }

    /**
     * Deploy a query into the network
     * @return {undefined}
     *
     */
    async query (query) {
      const packet = new PacketQuery({
        message: query,
        usr1: Buffer.from(String(Date.now())),
        usr3: Buffer.from(randomBytes(32)),
        usr4: Buffer.from(String(1))
      })
      const data = await Packet.encode(packet)

      const p = Packet.decode(data) // finalize a packet
      const pid = p.packetId.toString('hex')

      if (this.gate.has(pid)) return
      this.returnRoutes.set(p.usr3.toString('hex'), {})
      this.gate.set(pid, 1) // don't accidentally spam

      debug(this.peerId, `-> QUERY (type=question, query=${query}, packet=${pid.slice(0, 8)})`)

      await this.mcast(p)
    }

    /**
     *
     * This is a default implementation for deciding what to summarize
     * from the cache when receiving a request to sync. that can be overridden
     *
     */
    cachePredicate (packet) {
      return packet.version === VERSION && packet.timestamp > Date.now() - Packet.ttl
    }

    /**
     * A connection was made, add the peer to the local list of known
     * peers and call the onConnection if it is defined by the user.
     *
     * @return {undefined}
     * @ignore
     */
    async _onConnection (packet, peerId, port, address, proxy, socket) {
      if (this.closing) return

      const natType = packet.message.natType

      const { clusterId, subclusterId } = packet

      let peer = this.getPeer(peerId)

      if (!peer) {
        peer = new RemotePeer({ peerId })

        if (this.peers.length >= 256) {
          // TODO evicting an older peer definitely needs some more thought.
          const oldPeerIndex = this.peers.findIndex(p => !p.lastUpdate && !p.indexed)
          if (oldPeerIndex > -1) this.peers.splice(oldPeerIndex, 1)
        }

        this.peers.push(peer)
      }

      peer.connected = true
      peer.lastUpdate = Date.now()
      peer.port = port
      peer.natType = natType
      peer.address = address
      if (proxy) peer.proxy = proxy
      if (socket) peer.socket = socket

      const cid = clusterId.toString('base64')
      const scid = subclusterId.toString('base64')

      if (cid) peer.clusters[cid] ??= {}

      if (cid && scid) {
        const cluster = peer.clusters[cid]
        cluster[scid] = { rateLimit: MAX_BANDWIDTH }
      }

      if (!peer.localPeer) peer.localPeer = this
      if (!this.connections) this.connections = new Map()

      debug(this.peerId, '<- CONNECTION ( ' +
        `peerId=${peer.peerId.slice(0, 6)}, ` +
        `address=${address}:${port}, ` +
        `type=${packet.type}, ` +
        `cluster=${cid.slice(0, 8)}, ` +
        `sub-cluster=${scid.slice(0, 8)})`
      )

      if (this.onJoin && this.clusters[cid]) {
        this.onJoin(packet, peer, port, address)
      }

      if (!this.connections.has(peer)) {
        this.onConnection && this.onConnection(packet, peer, port, address)
        this.connections.set(peer, packet.message.cacheSummaryHash)
      }
    }

    /**
     * Received a Sync Packet
     * @return {undefined}
     * @ignore
     */
    async _onSync (packet, port, address) {
      this.metrics.i[packet.type]++

      this.lastSync = Date.now()
      const pid = packet.packetId?.toString('hex')

      if (!isBufferLike(packet.message)) return
      if (this.gate.has(pid)) return

      this.gate.set(pid, 1)

      const remote = Cache.decodeSummary(packet.message)
      const local = await this.cache.summarize(remote.prefix, this.cachePredicate)

      if (!remote || !remote.hash || !local || !local.hash || local.hash === remote.hash) {
        if (this.onSyncFinished) this.onSyncFinished(packet, port, address)
        return
      }

      if (this.onSync) this.onSync(packet, port, address, { remote, local })

      const remoteBuckets = remote.buckets.filter(Boolean).length
      debug(this.peerId, `<- ON SYNC (from=${address}:${port}, local=${local.hash.slice(0, 8)}, remote=${remote.hash.slice(0, 8)} remote-buckets=${remoteBuckets})`)

      for (let i = 0; i < local.buckets.length; i++) {
        //
        // nothing to send/sync, expect peer to send everything they have
        //
        if (!local.buckets[i] && !remote.buckets[i]) continue

        //
        // you dont have any of these, im going to send them to you
        //
        if (!remote.buckets[i]) {
          for (const [key, p] of this.cache.data.entries()) {
            if (!key.startsWith(local.prefix + i.toString(16))) continue

            const packet = Packet.from(p)
            if (!this.cachePredicate(packet)) continue

            const pid = packet.packetId.toString('hex')
            debug(this.peerId, `-> SYNC SEND PACKET (type=data, packetId=${pid.slice(0, 8)}, to=${address}:${port})`)

            this.send(await Packet.encode(packet), port, address)
          }
        } else {
          //
          // need more details about what exactly isn't synce'd
          //
          const nextLevel = await this.cache.summarize(local.prefix + i.toString(16), this.cachePredicate)
          const data = await Packet.encode(new PacketSync({
            message: Cache.encodeSummary(nextLevel)
          }))
          this.send(data, port, address)
        }
      }
    }

    /**
     * Received a Query Packet
     *
     * a -> b -> c -> (d) -> c -> b -> a
     *
     * @return {undefined}
     * @example
     *
     * ```js
     * peer.onQuery = (packet) => {
     *   //
     *   // read a database or something
     *   //
     *   return {
     *     message: Buffer.from('hello'),
     *     publicKey: '',
     *     privateKey: ''
     *   }
     * }
     * ```
     */
    async _onQuery (packet, port, address) {
      this.metrics.i[packet.type]++

      const pid = packet.packetId.toString('hex')
      if (this.gate.has(pid)) return
      this.gate.set(pid, 1)

      const queryTimestamp = parseInt(Buffer.from(packet.usr1).toString(), 10)
      const queryId = Buffer.from(packet.usr3).toString('hex')
      const queryType = parseInt(Buffer.from(packet.usr4).toString(), 10)

      // if the timestamp in usr1 is older than now - 2s, bail
      if (queryTimestamp < (Date.now() - 2048)) return

      const type = queryType === 1 ? 'question' : 'answer'
      debug(this.peerId, `<- QUERY (type=${type}, from=${address}:${port}, packet=${pid.slice(0, 8)})`)

      let rinfo = { port, address }

      //
      // receiving an answer
      //
      if (this.returnRoutes.has(queryId)) {
        rinfo = this.returnRoutes.get(queryId)

        let p = packet.copy()
        if (p.index > -1) p = await this.cache.compose(p)

        if (p?.index === -1) {
          this.returnRoutes.delete(p.previousId.toString('hex'))
          p.type = PacketPublish.type
          delete p.usr3
          delete p.usr4
          if (this.onAnswer) return this.onAnswer(p.message, p, port, address)
        }

        if (!rinfo.address) return
      } else {
        //
        // receiving a query
        //
        this.returnRoutes.set(queryId, { address, port })

        const query = packet.message
        const packets = []

        //
        // The requestor is looking for an exact packetId. In this case,
        // the peer has a packet with a previousId or nextId that points
        // to a packetId they don't have. There is no need to specify the
        // index in the query, split packets will have a nextId.
        //
        // if cache packet = { nextId: 'deadbeef...' }
        // then query = { packetId: packet.nextId }
        // or query = { packetId: packet.previousId }
        //
        if (query.packetId && this.cache.has(query.packetId)) {
          const p = this.cache.get(query.packetId)
          if (p) packets.push(p)
        } else if (this.onQuery) {
          const q = await this.onQuery(query)
          if (q) packets.push(...await this._message2packets(PacketQuery, q.message, q))
        }

        if (packets.length) {
          for (const p of packets) {
            p.type = PacketQuery.type // convert the type during transport
            p.usr3 = packet.usr3 // ensure the packet has the queryId
            p.usr4 = Buffer.from(String(2)) // mark it as an answer packet
            this.send(await Packet.encode(p), rinfo.port, rinfo.address)
          }
          return
        }
      }

      if (packet.hops >= this.maxHops) return
      debug(this.peerId, '>> QUERY RELAY', port, address)
      return await this.mcast(packet)
    }

    /**
     * Received a Ping Packet
     * @return {undefined}
     * @ignore
     */
    async _onPing (packet, port, address) {
      this.metrics.i[packet.type]++

      this.lastUpdate = Date.now()
      const { reflectionId, isReflection, isConnection, isHeartbeat } = packet.message

      if (packet.message.requesterPeerId === this.peerId) return

      const { probeExternalPort, isProbe, pingId } = packet.message

      if (isHeartbeat) {
        // const peer = this.getPeer(packet.message.requesterPeerId)
        // if (peer && natType) peer.natType = natType
        return
      }

      // if (peer && reflectionId) peer.reflectionId = reflectionId
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
      if (pingId) message.pingId = pingId

      if (isReflection) {
        message.isReflection = true
        message.port = port
        message.address = address
      } else {
        message.natType = this.natType
      }

      if (isConnection) {
        const peerId = packet.message.requesterPeerId
        this._onConnection(packet, peerId, port, address)

        message.isConnection = true
        delete message.address
        delete message.port
        delete message.isProbe
      }

      const { hash } = await this.cache.summarize('', this.cachePredicate)
      message.cacheSummaryHash = hash

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
      this.metrics.i[packet.type]++

      this.lastUpdate = Date.now()

      const { reflectionId, pingId, isReflection, responderPeerId } = packet.message

      debug(this.peerId, `<- PONG (from=${address}:${port}, hash=${packet.message.cacheSummaryHash}, isConnection=${!!packet.message.isConnection})`)
      const peer = this.getPeer(packet.message.responderPeerId)

      if (packet.message.isConnection) {
        if (pingId) peer.pingId = pingId
        this._onConnection(packet, packet.message.responderPeerId, port, address)
        return
      }

      if (!peer) return

      if (isReflection && !this.indexed) {
        if (reflectionId !== this.reflectionId) return

        this._clearTimeout(this.reflectionTimeout)

        if (!this.reflectionFirstResponder) {
          this.reflectionFirstResponder = { port, address, responderPeerId, reflectionId, packet }
          if (this.onConnecting) this.onConnecting({ code: 2.5, status: `Received reflection from ${address}:${port}` })
          debug(this.peerId, '<- NAT REFLECT - STAGE2: FIRST RESPONSE', port, address, this.reflectionId)
        } else {
          if (this.onConnecting) this.onConnecting({ code: 2.5, status: `Received reflection from ${address}:${port}` })
          debug(this.peerId, '<- NAT REFLECT - STAGE2: SECOND RESPONSE', port, address, this.reflectionId)
          if (packet.message.address !== this.address) return

          this.nextNatType |= (
            packet.message.port === this.reflectionFirstResponder.packet.message.port
          )
            ? NAT.MAPPING_ENDPOINT_INDEPENDENT
            : NAT.MAPPING_ENDPOINT_DEPENDENT

          debug(
            this.peerId,
            `++ NAT REFLECT - STATE UPDATE (natType=${this.natType}, nextType=${this.nextNatType})`,
            packet.message.port,
            this.reflectionFirstResponder.packet.message.port
          )

          // wait PROBE_WAIT milliseconds for zero or more probe responses to arrive.
          this._setTimeout(async () => {
            // build the NAT type by combining information about the firewall with
            // information about the endpoint independence
            let natType = this.nextNatType

            // in the case where we recieved zero probe responses, we assume the firewall
            // is of the hardest type 'FIREWALL_ALLOW_KNOWN_IP_AND_PORT'.
            if (!NAT.isFirewallDefined(natType)) natType |= NAT.FIREWALL_ALLOW_KNOWN_IP_AND_PORT

            // if ((natType & NAT.MAPPING_ENDPOINT_DEPENDENT) === 1) natType = NAT.ENDPOINT_RESTRICTED

            if (NAT.isValid(natType)) {
              // const oldType = this.natType
              this.natType = natType
              this.reflectionId = null
              this.reflectionStage = 0

              // if (natType !== oldType) {
              // alert all connected peers of our new NAT type
              for (const peer of this.peers) {
                peer.lastRequest = Date.now()

                debug(this.peerId, `-> PING (to=${peer.address}:${peer.port}, peer-id=${peer.peerId.slice(0, 8)}, is-connection=true)`)

                await this.ping(peer, false, {
                  message: {
                    requesterPeerId: this.peerId,
                    natType: this.natType,
                    cacheSize: this.cache.size,
                    isConnection: true
                  }
                })
              }

              if (this.onNat) this.onNat(this.natType)

              debug(this.peerId, `++ NAT (type=${NAT.toString(this.natType)})`)
              this.sendUnpublished()
              // }

              if (this.onConnecting) this.onConnecting({ code: 3, status: `Discovered! (nat=${NAT.toString(this.natType)})` })
              if (this.onReady) this.onReady()
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
    async _onIntro (packet, port, address, _, opts = { attempts: 0 }) {
      this.metrics.i[packet.type]++
      if (this.closing) return

      const pid = packet.packetId.toString('hex')
      // the packet needs to be gated, but should allow for attempt
      // recursion so that the fallback can still be selected.
      if (this.gate.has(pid) && opts.attempts === 0) return
      this.gate.set(pid, 1)

      if (packet.hops >= this.maxHops) return
      if (packet.message.timestamp + this.config.keepalive < Date.now()) return
      if (packet.message.requesterPeerId === this.peerId) return // intro to myself?
      if (packet.message.responderPeerId === this.peerId) return // intro from myself?

      // this is the peer that is being introduced to the new peers
      const peerId = packet.message.requesterPeerId
      const peerPort = packet.message.port
      const peerAddress = packet.message.address
      const natType = packet.message.natType
      const { clusterId, subclusterId, clock } = packet

      // already introduced in the laste minute, just drop the packet
      if (opts.attempts === 0 && this.gate.has(peerId + peerAddress + peerPort)) return
      this.gate.set(peerId + peerAddress + peerPort, 2)

      // we already know this peer, and we're even connected to them!
      let peer = this.getPeer(peerId)
      if (!peer) peer = new RemotePeer({ peerId, natType, port: peerPort, address: peerAddress, clock, clusterId, subclusterId })
      if (peer.connected) return // already connected
      if (clock > 0 && clock < peer.clock) return
      peer.clock = clock

      // a mutex per inbound peer to ensure that it's not connecting concurrently,
      // the check of the attempts ensures its allowed to recurse before failing so
      // it can still fall back
      if (this.gate.has('CONN' + peer.peerId) && opts.attempts === 0) return
      this.gate.set('CONN' + peer.peerId, 1)

      const cid = clusterId.toString('base64')
      const scid = subclusterId.toString('base64')

      debug(this.peerId, '<- INTRO (' +
        `isRendezvous=${packet.message.isRendezvous}, ` +
        `from=${address}:${port}, ` +
        `to=${packet.message.address}:${packet.message.port}, ` +
        `clustering=${cid.slice(0, 4)}/${scid.slice(0, 4)}` +
      ')')

      if (this.onIntro) this.onIntro(packet, peer, peerPort, peerAddress)

      const pingId = Math.random().toString(16).slice(2)
      const { hash } = await this.cache.summarize('', this.cachePredicate)

      const props = {
        clusterId,
        subclusterId,
        message: {
          natType: this.natType,
          isConnection: true,
          cacheSummaryHash: hash || null,
          pingId: packet.message.pingId,
          requesterPeerId: this.peerId
        }
      }

      const strategy = NAT.connectionStrategy(this.natType, packet.message.natType)
      const proxyCandidate = this.peers.find(p => p.peerId === packet.message.responderPeerId)

      if (opts.attempts >= 2) {
        this._onConnection(packet, peer.peerId, peerPort, peerAddress, proxyCandidate)
        return false
      }

      this._setTimeout(() => {
        if (this.getPeer(peer.peerId)) return
        opts.attempts = 2
        this._onIntro(packet, port, address, _, opts)
      }, 1024 * 2)

      if (packet.message.isRendezvous) {
        debug(this.peerId, `<- INTRO FROM RENDEZVOUS (to=${packet.message.address}:${packet.message.port}, dest=${packet.message.requesterPeerId.slice(0, 6)}, via=${address}:${port}, strategy=${NAT.toStringStrategy(strategy)})`)
      }

      debug(this.peerId, `++ NAT INTRO (strategy=${NAT.toStringStrategy(strategy)}, from=${this.address}:${this.port} [${NAT.toString(this.natType)}], to=${packet.message.address}:${packet.message.port} [${NAT.toString(packet.message.natType)}])`)

      if (strategy === NAT.STRATEGY_TRAVERSAL_CONNECT) {
        debug(this.peerId, `## NAT CONNECT (from=${this.address}:${this.port}, to=${peerAddress}:${peerPort}, pingId=${pingId})`)

        let i = 0
        if (!this.socketPool) {
          this.socketPool = Array.from({ length: 256 }, (_, index) => {
            return dgram.createSocket('udp4', null, this, index).unref()
          })
        }

        // A probes 1 target port on B from 1024 source ports
        //   (this is 1.59% of the search clusterId)
        // B probes 256 target ports on A from 1 source port
        //   (this is 0.40% of the search clusterId)
        //
        // Probability of successful traversal: 98.35%
        //
        const interval = this._setInterval(async () => {
          // send messages until we receive a message from them. giveup after sending ±1024
          // packets and fall back to using the peer that sent this as the initial proxy.
          if (i++ >= 1024) {
            this._clearInterval(interval)

            opts.attempts++
            this._onIntro(packet, port, address, _, opts)
            return false
          }

          const p = {
            clusterId,
            subclusterId,
            message: {
              requesterPeerId: this.peerId,
              cacheSummaryHash: hash || null,
              natType: this.natType,
              uptime: this.uptime,
              isConnection: true,
              timestamp: Date.now(),
              pingId
            }
          }

          const data = await Packet.encode(new PacketPing(p))

          const rand = () => Math.random() - 0.5
          const pooledSocket = this.socketPool.sort(rand).find(s => !s.active)
          if (!pooledSocket) return // TODO recover from exausted socket pool

          // mark socket as active & deactivate it after timeout
          pooledSocket.active = true
          pooledSocket.reclaim = this._setTimeout(() => {
            pooledSocket.active = false
            pooledSocket.removeAllListeners()
          }, 1024)

          pooledSocket.on('message', async (msg, rinfo) => {
            // if (rinfo.port !== peerPort || rinfo.address !== peerAddress) return

            // cancel scheduled events
            this._clearInterval(interval)
            this._clearTimeout(pooledSocket.reclaim)

            // remove any events currently bound on the socket
            pooledSocket.removeAllListeners()
            pooledSocket.on('message', (msg, rinfo) => {
              this._onMessage(msg, rinfo)
            })

            this._onConnection(packet, peer.peerId, rinfo.port, rinfo.address, undefined, pooledSocket)

            const p = {
              clusterId,
              subclusterId,
              clock: this.clock,
              message: {
                requesterPeerId: this.peerId,
                natType: this.natType,
                isConnection: true
              }
            }

            const data = await Packet.encode(new PacketPing(p))

            pooledSocket.send(data, rinfo.port, rinfo.address)

            // create a new socket to replace it in the pool
            const oldIndex = this.socketPool.findIndex(s => s === pooledSocket)
            this.socketPool[oldIndex] = dgram.createSocket('udp4', null, this).unref()

            this._onMessage(msg, rinfo)
          })

          try {
            pooledSocket.send(data, peerPort, peerAddress)
          } catch (err) {
            console.error('STRATEGY_TRAVERSAL_CONNECT error', err)
          }
        }, 10)

        return
      }

      if (strategy === NAT.STRATEGY_PROXY && !peer.proxy) {
        // TODO could allow multiple proxies
        this._onConnection(packet, peer.peerId, peerPort, peerAddress, proxyCandidate)
        debug(this.peerId, '++ INTRO CHOSE PROXY STRATEGY')
      }

      if (strategy === NAT.STRATEGY_TRAVERSAL_OPEN) {
        peer.opening = Date.now()

        const portsCache = new Set()

        if (!this.bdpCache.length) {
          globalThis.bdpCache = this.bdpCache = Array.from({ length: 1024 }, () => getRandomPort(portsCache))
        }

        for (const port of this.bdpCache) {
          this.send(Buffer.from([0x1]), port, packet.message.address)
        }

        return
      }

      if (strategy === NAT.STRATEGY_DIRECT_CONNECT) {
        debug(this.peerId, '++ NAT STRATEGY_DIRECT_CONNECT')
      }

      if (strategy === NAT.STRATEGY_DEFER) {
        debug(this.peerId, '++ NAT STRATEGY_DEFER')
      }

      this.ping(peer, true, props)
    }

    /**
     * Received an Join Packet
     * @return {undefined}
     * @ignore
     */
    async _onJoin (packet, port, address, data) {
      this.metrics.i[packet.type]++

      const pid = packet.packetId.toString('hex')
      if (packet.message.requesterPeerId === this.peerId) return
      if (this.gate.has(pid)) return
      if (!packet.clusterId) return

      this.lastUpdate = Date.now()

      const peerId = packet.message.requesterPeerId
      const rendezvousDeadline = packet.message.rendezvousDeadline
      const clusterId = packet.clusterId
      const subclusterId = packet.subclusterId
      const peerAddress = packet.message.address
      const peerPort = packet.message.port

      // a rendezvous isn't relevant if it's too old, just drop the packet
      if (rendezvousDeadline && rendezvousDeadline < Date.now()) return

      const cid = clusterId.toString('base64')
      const scid = subclusterId.toString('base64')

      debug(this.peerId, '<- JOIN (' +
        `peerId=${peerId.slice(0, 6)}, ` +
        `clock=${packet.clock}, ` +
        `hops=${packet.hops}, ` +
        `clusterId=${cid}, ` +
        `subclusterId=${scid}, ` +
        `address=${address}:${port})`
      )

      //
      // This packet represents a peer who wants to join the network and is a
      // member of our cluster. The packet was replicated though the network
      // and contains the details about where the peer can be reached, in this
      // case we want to ping that peer so we can be introduced to them.
      //
      if (rendezvousDeadline && !this.indexed && this.clusters[cid]) {
        if (!packet.message.rendezvousRequesterPeerId) {
          const pid = packet.packetId.toString('hex')
          this.gate.set(pid, 2)

          // TODO it would tighten up the transition time between dropped peers
          // if we check strategy from (packet.message.natType, this.natType) and
          // make introductions that create more mutually known peers.
          debug(this.peerId, `<- JOIN RENDEZVOUS START (to=${peerAddress}:${peerPort}, via=${packet.message.rendezvousAddress}:${packet.message.rendezvousPort})`)

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

      const filter = p => (
        p.connected && // you can't intro peers who aren't connected
        p.peerId !== packet.message.requesterPeerId &&
        p.peerId !== packet.message.rendezvousRequesterPeerId &&
        !p.indexed
      )

      let peers = this.getPeers(packet, this.peers, [{ port, address }], filter)

      //
      // A peer who belongs to the same cluster as the peer who's replicated
      // join was discovered, sent us a join that has a specification for who
      // they want to be introduced to.
      //
      if (packet.message.rendezvousRequesterPeerId && this.peerId === packet.message.rendezvousPeerId) {
        const peer = this.peers.find(p => p.peerId === packet.message.rendezvousRequesterPeerId)

        if (!peer) {
          debug(this.peerId, '<- INTRO FROM RENDEZVOUS FAILED', packet)
          return
        }

        // peer.natType = packet.message.rendezvousType
        peers = [peer]

        debug(this.peerId, `<- JOIN EXECUTING RENDEZVOUS (from=${packet.message.address}:${packet.message.port}, to=${peer.address}:${peer.port})`)
      }

      for (const peer of peers) {
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

        const opts = {
          hops: packet.hops + 1,
          clusterId,
          subclusterId
        }

        const intro1 = await Packet.encode(new PacketIntro({ ...opts, message: message1 }))
        const intro2 = await Packet.encode(new PacketIntro({ ...opts, message: message2 }))

        //
        // Send intro1 to the peer described in the message
        // Send intro2 to the peer in this loop
        //
        debug(this.peerId, `>> INTRO SEND (from=${peer.address}:${peer.port}, to=${packet.message.address}:${packet.message.port})`)
        debug(this.peerId, `>> INTRO SEND (from=${packet.message.address}:${packet.message.port}, to=${peer.address}:${peer.port})`)

        peer.lastRequest = Date.now()

        this.send(intro2, peer.port, peer.address)
        this.send(intro1, packet.message.port, packet.message.address)

        this.gate.set(Packet.decode(intro1).packetId.toString('hex'), 2)
        this.gate.set(Packet.decode(intro2).packetId.toString('hex'), 2)
      }

      this.gate.set(packet.packetId.toString('hex'), 2)

      if (packet.hops >= this.maxHops) return
      if (this.indexed && !packet.clusterId) return

      if (packet.hops === 1 && this.natType === NAT.UNRESTRICTED && !packet.message.rendezvousDeadline) {
        packet.message.rendezvousAddress = this.address
        packet.message.rendezvousPort = this.port
        packet.message.rendezvousType = this.natType
        packet.message.rendezvousPeerId = this.peerId
        packet.message.rendezvousDeadline = Date.now() + this.config.keepalive
      }

      debug(this.peerId, `-> JOIN RELAY (peerId=${peerId.slice(0, 6)}, from=${peerAddress}:${peerPort})`)
      this.mcast(packet, [{ port, address }, { port: peerPort, address: peerAddress }])

      if (packet.hops <= 1) {
        this._onConnection(packet, packet.message.requesterPeerId, port, address)
      }
    }

    /**
     * Received an Publish Packet
     * @return {undefined}
     * @ignore
     */
    async _onPublish (packet, port, address, data) {
      this.metrics.i[packet.type]++

      // only cache if this packet if i am part of this subclusterId
      // const cluster = this.clusters[packet.clusterId]
      // if (cluster && cluster[packet.subclusterId]) {

      const pid = packet.packetId.toString('hex')
      if (this.cache.has(pid)) {
        debug(this.peerId, `<- PUBLISH DUPE (packetId=${pid.slice(0, 8)}, from=${address}:${port})`)
        return
      }

      debug(this.peerId, `<- PUBLISH (packetId=${pid.slice(0, 8)}, from=${address}:${port}, is-sync=${packet.usr4.toString() === 'SYNC'})`)
      this.cache.insert(pid, packet)

      const ignorelist = [{ address, port }]
      const scid = packet.subclusterId.toString('base64')

      if (!this.indexed && this.encryption.has(scid)) {
        let p = packet.copy()
        if (p.index > -1) p = await this.cache.compose(p)
        if (p?.index === -1 && this.onPacket) this.onPacket(p, port, address)
      }

      if (packet.hops >= this.maxHops) return
      this.mcast(packet, ignorelist)

      // }
    }

    /**
     * Received an Stream Packet
     * @return {undefined}
     * @ignore
     */
    async _onStream (packet, port, address, data) {
      this.metrics.i[packet.type]++

      const pid = packet.packetId.toString('hex')
      if (this.gate.has(pid)) return
      this.gate.set(pid, 1)

      const streamTo = packet.usr3.toString('hex')
      const streamFrom = packet.usr4.toString('hex')

      // only help packets with a higher hop count if they are in our cluster
      // if (packet.hops > 2 && !this.clusters[packet.cluster]) return

      const peerFrom = this.peers.find(p => p.peerId.toString('hex') === streamFrom.toString('hex'))
      if (!peerFrom) return

      // stream message is for this peer
      if (streamTo.toString('hex') === this.peerId.toString('hex')) {
        const scid = packet.subclusterId.toString('base64')

        if (this.encryption.has(scid)) {
          let p = packet.copy() // clone the packet so it's not modified

          if (packet.index > -1) { // if it needs to be composed...
            p.timestamp = Date.now()
            this.streamBuffer.set(p.packetId.toString('hex'), p) // cache the partial

            p = await this.cache.compose(p, this.streamBuffer) // try to compose
            if (!p) return // could not compose

            if (p) { // if successful, delete the artifacts
              const previousId = p.index === 0 ? p.packetId : p.previousId
              const pid = previousId.toString('hex')

              this.streamBuffer.forEach((v, k) => {
                if (k === pid) this.streamBuffer.delete(k)
                if (v.previousId.toString('hex') === pid) this.streamBuffer.delete(k)
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

      if (packet.hops >= this.maxHops) {
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
      if (!packet || packet.version !== VERSION) return
      if (packet?.type !== 2) return

      const pid = packet.packetId.toString('hex')
      if (this.gate.has(pid)) return
      this.gate.set(pid, 1)

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

        // if we have previously sent an outbount message to this peer on the probe port
        // then our NAT will have a mapping for their IP, but not their IP+Port.
        if (!NAT.isFirewallDefined(this.nextNatType)) {
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
      if (!packet || packet.version !== VERSION) return

      const peer = this.peers.find(p => p.address === address && p.port === port)
      if (peer) peer.lastUpdate = Date.now()

      const cid = packet.clusterId.toString('base64')
      const scid = packet.subclusterId.toString('base64')

      // debug('<- PACKET', packet.type, port, address)
      const clusters = this.clusters[cid]
      const subcluster = clusters && clusters[scid]

      if (!this.config.limitExempt) {
        if (rateLimit(this.rates, packet.type, port, address, subcluster)) {
          debug(this.peerId, `XX RATE LIMIT HIT (from=${address}, type=${packet.type})`)
          this.metrics.i.REJECTED++
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
