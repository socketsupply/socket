// import Debug from 'debug'
import { Buffer } from 'socket:buffer'

import {
  Packet,
  PacketPing,
  PacketPong,
  PacketIntro,
  PacketQuery,
  PacketAnswer,
  PacketPublish,
  PacketSubscribe,
  applyTax,
  sha256
} from './packets.js'

import { getRandomPort, getNRandom } from './util.js'
import { Cache } from './cache.js'

export { sha256, Cache }
export const PING_RETRY = 500
export const DEFAULT_PORT = 9777
export const DEFAULT_TEST_PORT = 9778
export const KEEP_ALIVE = 29_000

export class RemotePeer {
  peerId = null
  address = null
  port = 0

  natType = null
  topics = []
  pingId = null
  introducer = false
  writes = 0
  lastUpdate = 0
  constructor (o) {
    if (o.introducer) this.natType = 'static'

    if (!o.port) throw new Error('expected .port')
    if (!o.address) throw new Error('expected .address')
    if (!o.peerId) throw new Error('expected .peerId')

    Object.assign(this, o)
  }
}

// const debug = Debug('peer')

const getN = (n, peers) => {
  peers = getNRandom(n, peers)
  return [...peers].sort((a, b) => {
    if (a.natType === 'easy') return -1
    if (a.natType === 'static' && b.natType !== 'easy') return -1
    return 0
  })
}

export const createPeer = udp => {
  class Peer {
    port = null
    address = null
    natType = null
    reflectionId = null
    peerId = ''
    ctime = Date.now()
    lastUpdate = 0
    closing = false
    listening = false
    unpublished = {}
    cache = null
    limits = {}
    topics = ['09ca7e4eaa6e8ae9c7d261167129184883644d07dfba7cbfbc4c8a2e08360d5b']
    bdpCache = []

    peers = [
      new RemotePeer({
        address: '18.205.9.88',
        port: DEFAULT_PORT,
        peerId: 'aaecb3746ecec8f9b72eef221ccdd55da8c6fdccd54ba9a9839e8927a8750861',
        introducer: true
      }),
      new RemotePeer({
        address: '34.236.219.221',
        port: DEFAULT_PORT,
        peerId: 'bb40cd15d7266be9248ae8c8f10de00260f970b7dae18cafdfa753f6cc1d58ff',
        introducer: true
      })
    ]

    constructor (config, persistedState) {
      if (!config.peerId) throw new Error('config.peerId required')

      this.config = {
        port: DEFAULT_PORT,
        keepalive: KEEP_ALIVE,
        testPort: DEFAULT_TEST_PORT,
        ...config
      }

      let cacheData

      if (persistedState?.data?.length > 0) {
        cacheData = new Map(persistedState.data)
      }

      this.cache = new Cache(cacheData)
      this.cache.peer = this

      this.unpublished = persistedState?.unpublished || {}

      Object.assign(this, config)
      this.introducer = !!config.introducer
      this.natType = config.introducer ? 'static' : null

      this.subscribeAll = this.onReflection

      this.socket = udp.createSocket('udp4', this)
      this.testSocket = udp.createSocket('udp4', this)
    }

    init () {
      this.socket.on('listening', () => {
        this.listening = true
        if (this.onListening) this.onListening()
        this.requestReflection()
      })

      this.socket.on('message', (...args) => this.onPacket(...args))
      this.socket.on('error', (...args) => this.onError(...args))
      this.testSocket.on('message', (...args) => this.onPacket(...args))
      this.testSocket.on('error', (...args) => this.onError(...args))

      this.socket.bind(this.config.port)
      this.testSocket.bind(this.config.testPort)

      globalThis.window?.addEventListener('online', async () => {
        this.subscribeAll()

        setTimeout(() => {
          for (const [packetId, topicId] of Object.entries(this.unpublished)) {
            const data = this.cache.get(packetId)
            if (data) this.broadcast(data, packetId, topicId)
          }
        }, 1024)
      })

      const cb = async ts => {
        if (this.closing) return false // cancel timer

        const offline = globalThis.navigator && !globalThis.navigator.onLine
        if (!this.config.introducer && !offline) this.requestReflection()

        for (const [k, packet] of [...this.cache.data]) {
          const TTL = Packet.ttl

          if (packet.timestamp + TTL < ts) {
            this.cache.delete(k)
            if (this.onDelete) this.onDelete(packet)
          }
        }

        // debug('PING HART', this.peerId, this.peers)
        for (const [i, peer] of Object.entries(this.peers)) {
          if (peer.introducer) continue
          const then = Date.now() - peer.lastUpdate

          if (then >= this.config.keepalive) {
            this.peers.splice(i, 1)
            continue
          }

          if (this.limits[peer.address]) {
            this.limits[peer.address] = this.limits[peer.address] / 1.05
          }

          this.ping(peer, {
            peerId: this.peerId,
            natType: this.natType,
            data: this.cache.size,
            isHeartbeat: true
          })
        }
      }

      this.timer(this.config.keepalive, this.config.keepalive, cb)
    }

    send (data, ...args) {
      try {
        this.socket.send(data, ...args)
      } catch (err) {
        if (this.onError) this.onError(new Error('ESEND'), ...args)
        return false
      }
      return true
    }

    getState () {
      return {
        data: [...this.cache.data],
        unpublished: this.unpublished
      }
    }

    // Packets MAY be addressed directly to a peer using IP address and port,
    // but should never assume that any particular peer will be available.
    // Instead, packets should be broadcast to the network.
    async broadcast (data, packetId, topicId, isTaxed, filter = x => x) {
      const peers = getN(3, this.peers.filter(filter))

      let stagger = 0
      for (const peer of peers) {
        await new Promise(resolve => {
          this.timer((stagger++) * 2, 0, async ts => {
            if (this.send(isTaxed ? applyTax(data) : data, peer.port, peer.address)) {
              delete this.unpublished[packetId]
            }
            resolve()
          })
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
      if (this.closing || this.config.introducer) return

      // get two random easy peers from the known peers and send pings, we can
      // learn this peer's nat type if we have at least two Pongs from peers
      // that are outside of this peer's NAT.
      const peers = this.peers.filter(p => p.natType !== 'static' || p.natType !== 'easy')

      peers.sort((a, b) => {
        if (a.natType === 'static' && b.natType !== 'static') return -1
        return 0
      })

      const [peer1, peer2] = peers

      const peerId = this.peerId
      const pingId = Math.random().toString(16).slice(2)
      const topics = this.topics
      const testPort = this.config.testPort

      // debug(`  requestReflection peerId=${peerId}, pingId=${pingId}`)

      this.ping(peer1, { peerId, topics, pingId, isReflection: true })
      this.ping(peer2, { peerId, topics, pingId, isReflection: true, testPort })
    }

    async ping (peer, props = {}) {
      if (!peer) return

      const packet = new PacketPing({ peerId: peer.peerId, ...props })
      const data = await Packet.encode(packet)

      const retry = () => {
        if (this.closing) return false

        const p = this.peers.find(p => p.peerId === peer.peerId)
        if (p && p.pingId === packet.message.pingId) return false
        this.send(data, peer.port, peer.address)
      }

      retry()

      this.timer(PING_RETRY, 0, retry)
      this.timer(PING_RETRY * 8, 0, retry)
      this.timer(PING_RETRY * 16, 0, retry)

      return packet
    }

    setPeer (packet, port, address) {
      if (this.address === address && this.port === port) return this
      const { peerId, natType, topics } = packet.message

      const existingPeer = this.peers.find(p => p.peerId === peerId)

      const newPeer = {
        peerId,
        port,
        address,
        lastUpdate: Date.now()
      }

      if (natType) newPeer.natType = natType
      if (topics) newPeer.topics = topics

      if (!existingPeer) {
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

      Object.assign(existingPeer, newPeer)
      return existingPeer
    }

    //
    // users call this at least once, when their app starts. It will broadcast
    // this peer, their latest topics and starts querying toe network for peers.
    //
    async subscribe (topicId) {
      if (!this.natType || !this.port || this.config.introducer) return

      const topicIndex = this.topics.indexOf(topicId)
      if (topicIndex === -1) this.topics.push(topicId)

      const data = await Packet.encode(new PacketSubscribe({
        peerId: this.peerId,
        topics: this.topics,
        natType: this.natType,
        address: this.address,
        port: this.port
      }))

      const peers = getN(3, this.peers)

      for (const peer of peers) {
        this.send(data, peer.port, peer.address)
      }
    }

    async publish (args) {
      const { topicId, previousId, nextId, message, meta = {} } = args

      let messages = [message]
      let len = message.length

      if (typeof message === 'object' && message.constructor.name !== 'Buffer') {
        try { len = Buffer.from(JSON.stringify(message)).length } catch {}
      }

      // Split packets that have messages bigger than Packet.maxLength
      if (len > 1024) {
        messages = [{
          meta,
          hash: await sha256(message),
          size: message.length,
          indexes: Math.ceil(message.length / 1024)
        }]
        let pos = 0
        while (pos < message.length) messages.push(message.slice(pos, pos += 1024))
      }

      // turn each message into an actual packet
      const packets = messages.map(message => new PacketPublish({
        peerId: this.peerId,
        topicId,
        message
      }))

      if (previousId) packets[0].previousId = previousId
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
        const data = await Packet.encode(packet)
        const inserted = this.cache.insert(packet.packetId, packet)

        if (inserted && this.onData) this.onData(packet, this.port, this.address)
        if (this.onPublish) this.onPublish(packet)

        if (globalThis.navigator && !globalThis.navigator.onLine) {
          this.unpublished[packet.packetId] = topicId
          continue
        }

        await this.broadcast(data, packet.packetId, topicId)
      }
    }

    negotiateCache (peer, packet, port, address) {
      if (this.config.introducer) return

      if (peer.writes === 0) {
        return this.cache.each(async (p, i) => {
          this.send(await Packet.encode(p), port, address) && peer.writes++
        })
      }

      const tails = [] // don't send tails if they are also heads

      // a tail is a packet with a .previousId that we don't have, the purpose
      // of sending it is to request packets that came before these.
      this.cache.tails((p, i) => {
        tails.push(p.packetId)
        this.timer(i * 1.5, 0, async () => {
          const q = new PacketQuery({
            packetId: p.packetId,
            topicId: p.topicId,
            message: { tail: true, peerId: this.peerId }
          })
          this.send(await Packet.encode(q), port, address)
        })
      })

      // a head is the last packet we know about in the chain, the purpose
      // of sending it is to ask for packets that might come after these.
      this.cache.heads((p, i) => {
        if (tails.includes(p.packetId)) return
        this.timer(i * 1.5, 0, async () => {
          const q = new PacketQuery({
            packetId: p.packetId,
            topicId: p.topicId,
            message: { peerId: this.peerId }
          })
          this.send(await Packet.encode(q), port, address)
        })
      })
    }

    async onQuery (packet, port, address, data) {
      const { packetId, topicId, message } = packet

      if (!this.cache.has(packetId)) { // we dont have it
        return this.broadcast(data, packetId, topicId)
      }

      const reply = async p => {
        p.type = PacketAnswer.type // convert to PacketAnswer
        this.send(await Packet.encode(p), port, address)
      }

      if (message.tail) { // move backward from the specified packetId
        this.cache.lookback(reply, packet)
      } else { // move forward from the specified packetId
        this.cache.each(reply, p => p.packetId === packetId)
      }
    }

    onAnswer (packet, port, address) {
      // if (this.cache.has(packet.packetId)) return
      packet.type = PacketPublish.type // convert from PacketAnswer
      if (this.cache.insert(packet.packetId, packet)) {
        if (this.onData) this.onData(packet, port, address)
      }
    }

    close () {
      this.closing = true
    }

    //
    // Events
    //
    async onReflection () {
      if (this.closing || this.config.introducer) return
      for (const topicId of this.topics) this.subscribe(topicId)
    }

    onConnection (peer, packet, port, address) {
      this.negotiateCache(peer, packet, port, address)
      if (this.closing) return
      if (this.onConnect) this.onConnect(peer, packet, port, address)
    }

    async onPing (packet, port, address) {
      this.lastUpdate = Date.now()
      const { pingId, isReflection, isConnection, isHeartbeat, testPort } = packet.message

      const peer = this.setPeer(packet, port, address)
      if (!peer || isHeartbeat) return

      const opts = {
        peerId: this.peerId,
        port,
        address,
        isConnection
      }

      if (!this.config.introducer) {
        opts.topics = this.topics
      }

      if (pingId) opts.pingId = pingId

      if (isReflection) {
        opts.isReflection = true
      } else {
        opts.natType = this.natType
      }

      if (isConnection) this.onConnection(peer, packet, port, address)

      const packetPong = new PacketPong(opts)
      const buf = await Packet.encode(packetPong)

      this.send(buf, port, address)

      if (testPort) { // also send to the test port
        opts.testPort = testPort
        const packetPong = new PacketPong(opts)
        const buf = await Packet.encode(packetPong)
        this.send(buf, testPort, address)
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
        this.onConnection(peer, packet, port, address)
        return
      }

      if (isReflection) {
        // this isn't a connection, it's just verifying that this is or is not a static peer
        if (packet.message.address === this.address && testPort === this.config.testPort) {
          this.reflectionId = null
          this.natType = 'static'
          this.address = packet.message.address
          this.port = packet.message.port
          // debug(`  response ${this.peerId} nat=${this.natType}`)
          if (this.onNat && oldType !== this.natType) this.onNat(this.natType)
          this.onReflection()
          return
        }

        if (!testPort && pingId && this.reflectionId === null) {
          this.reflectionId = pingId
        } else if (!testPort && pingId && pingId === this.reflectionId) {
          if (packet.message.address === this.address && packet.message.port === this.port) {
            this.natType = 'easy'
          } else if (packet.message.address === this.address && packet.message.port !== this.port) {
            this.natType = 'hard'
          }

          this.reflectionId = null

          if (oldType !== this.natType) {
            // debug(`  response ${this.peerId} nat=${this.natType}`)
            if (this.onNat) this.onNat(this.natType)
            this.onReflection() // this peer can let the user know we now know our nat type
          }

          this.ping(this, {
            peerId: this.peerId,
            natType: this.natType,
            data: this.cache.size,
            isHeartbeat: true
          })
        }

        this.address = packet.message.address
        this.port = packet.message.port
      }

      const peer = this.setPeer(packet, port, address)
      if (!peer) return
      this.onConnection(peer, packet, port, address)
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
      const rp = getRandomPort(DEFAULT_PORT, DEFAULT_TEST_PORT)
      const pingId = Math.random().toString(16).slice(2)

      const encoded = await Packet.encode(new PacketPing({
        peerId: this.peerId,
        topics: this.topics,
        natType: this.natType,
        isConnection: true,
        pingId
      }))

      if (localEasy && remoteHard) {
        // debug(`onintro (${this.peerId} easy, ${packet.message.peerId} hard)`)
        peer.connecting = true

        let i = 0
        this.timer(0, 10, (_ts) => {
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

          this.send(encoded, rp(portCache), packet.message.address)
        })

        return
      }

      if (localHard && remoteEasy) {
        // debug(`onintro (${this.peerId} hard, ${packet.message.peerId} easy)`)
        if (!this.bdpCache.length) {
          this.bdpCache = Array.from({ length: 256 }, () => rp(portCache))
        }

        for (const port of this.bdpCache) {
          this.send(encoded, port, packet.message.address)
        }
        return
      }

      // try to introduce them to another peer that is not hard (if we know about one)
      if (localHard && remoteHard) {
        // debug(`onintro (${this.peerId} hard, ${packet.message.peerId} hard)`)
        const peers = getN(3, this.peers.filter(p => p.natType !== 'hard'))

        for (const peer of peers) {
          if (peer.peerId === this.peerId) continue
          if (!peer.natType) continue
          if (packet.message.peerId === this.peerId) continue

          const p = await Packet.encode(new PacketIntro({
            peerId: peer.peerId,
            topics: peer.topics,
            natType: peer.natType,
            address: peer.address,
            port: peer.port
          }))

          this.send(p, packet.message.port, packet.message.address)
        }
        return
      }

      this.onConnection(peer, packet, port, address)

      // debug(`onintro (${this.peerId} easy/static, ${packet.message.peerId} easy/static)`)

      if (this.natType) {
        this.ping(peer, { peerId: this.peerId, natType: this.natType, isConnection: true, pingId: packet.message.pingId })
      }
    }

    //
    // A peer is letting us know they want to subscribe to a topic
    //  - update the peer in this.peers to include the topic
    //  - tell the peer what heads we have with the specified topicId
    //  - tell the peer what tails we have to see if they can help with missing packets
    //  - introduce this peer to some random peers in this.peers (introductions
    //    are only made to live peers, we do not track dead peers -- our TTL is
    //    30s so this would create a huge amount of useless data sent over the network.
    async onSub (packet, port, address) {
      this.lastUpdate = Date.now()
      const peer = this.setPeer(packet, port, address)

      this.negotiateCache(peer, packet, port, address)

      let peers
      // first only get peers who subscribe to this topic.
      peers = this.peers.filter(p => p.topics.includes(packet.topicId))
      // if our peer is hard, filter out any other hard peers.
      if (this.natType === 'hard') peers = peers.filter(p => p.natType !== 'hard')
      // if we still don't have 3 peers, take anyone
      if (peers.length < 3) peers = this.peers

      for (const peer of getN(3, peers)) { // getN will sort {easy,static}
        if (this.peerId === peer.peerId) continue
        if (this.peerId === packet.message.peerId) continue
        if (peer.peerId === packet.message.peerId) continue
        if (!peer.port || !peer.natType) continue // not known yet
        if (peer.introducer) continue // TODO possibly allow this

        // dont try to intro two hard peers
        if (peer.natType === 'hard' && this.natType === 'hard') continue

        // tell the caller to ping a peer from the list
        const intro1 = await Packet.encode(new PacketIntro({
          peerId: peer.peerId,
          natType: peer.natType,
          topics: peer.topics,
          address: peer.address,
          port: peer.port
        }))

        // tell the peer from the list to ping the caller
        const intro2 = await Packet.encode(new PacketIntro({
          peerId: packet.message.peerId,
          topics: packet.message.topics,
          natType: packet.message.natType,
          address: packet.message.address,
          port: packet.message.port
        }))

        await new Promise(resolve => {
          this.timer(Math.random() * 2, 0, () => {
            // debug(`onSub -> ${peer.address}:${peer.port}`)
            this.send(intro2, peer.port, peer.address)

            // debug(`onSub -> ${packet.message.address}:${packet.message.port}`)
            this.send(intro1, packet.message.port, packet.message.address)
            resolve()
          })
        })
      }
    }

    //
    // Event is fired when a user calls .publish and a peer receives the
    // packet, or when a peer receives a subscribe packet.
    //
    async onPub (packet, port, address, data) {
      // A peer can not send more than N messages in sequence. As messages
      // from other peers are received, their limits are reduced.
      if (!this.limits[address]) this.limits[address] = 0
      if (this.limits[address] >= 1024) return (this.limit && this.limit(address))

      // 1Mb max allowance per peer (or until other peers get a chance)
      this.limits[address]++

      // find another peer to credit
      const otherAddress = Object.entries(this.limits)
        .sort((a, b) => b[1] - a[1])
        .filter(limits => limits[0] !== address)
        .map(limits => limits[0])
        .pop()

      if (this.limits[otherAddress]) {
        this.limits[otherAddress] -= 1

        if (this.limits[otherAddress] <= 0) {
          delete this.limits[otherAddress]
        }
      }

      const inserted = this.cache.insert(packet.packetId, packet)

      if (packet.index > -1) {
        const p = await this.cache.compose(packet, sha256)
        if (p && this.onData) this.onData(p, port, address)
      } else if (inserted && this.onData) {
        this.onData(packet, port, address)
      }

      if (packet.postage < 1) return

      const filter = p => p.address !== address && p.port !== port
      this.broadcast(data, packet.packetId, packet.topicId, true, filter)
    }

    //
    // When a packet is received it is decoded, the packet contains the type
    // of the message. Based on the message type it is routed to a function.
    //
    onPacket (data, { port, address }) {
      const packet = Packet.decode(data)
      // console.log(`${packet.type} from ${address}:${port}`)
      const args = [packet, port, address, data]
      if (this.firewall) if (!this.firewall(...args)) return

      switch (packet.type) {
        case PacketPing.type: return this.onPing(...args)
        case PacketPong.type: return this.onPong(...args)
        case PacketIntro.type: return this.onIntro(...args)
        case PacketSubscribe.type: return this.onSub(...args)
        case PacketPublish.type: return this.onPub(...args)
        case PacketQuery.type: return this.onQuery(...args)
        case PacketAnswer.type: return this.onAnswer(...args)
      }
    }
  }

  return Peer
}

export default createPeer
