import { randomBytes } from '../crypto.js'
import { isBufferLike } from '../util.js'
import { Buffer } from '../buffer.js'
import debug from './index.js'

// trims buffer of trailing null bytes (0x00)
export const trim = (/** @type {Buffer} */ buf) => {
  let pos = buf.length
  while (buf.at(pos - 1) === 0x00) pos--
  return buf.slice(0, pos)
}

/**
 * Hash function factory.
 * @return {function(object|Buffer|string): Promise<string>}
 */
function createHashFunction () {
  const encoder = new TextEncoder()
  let crypto = null

  if (!globalThis.process?.versions?.node) {
    if (!crypto) crypto = globalThis.crypto?.subtle

    return async (seed, opts = {}) => {
      const encoding = opts.encoding || 'hex'
      const bytes = opts.bytes

      if (typeof seed === 'object' && !isBufferLike(seed)) {
        seed = JSON.stringify(seed)
      }

      if (seed && typeof seed === 'string') {
        seed = encoder.encode(seed)
      }

      let value

      if (seed) {
        value = Buffer.from(await crypto.digest('SHA-256', seed))
        if (bytes) return value
        return value.toString(encoding)
      }

      value = Buffer.from(randomBytes(32))
      if (opts.bytes) return value
      return value.toString(encoding)
    }
  }

  return async (seed, opts = {}) => {
    const encoding = opts.encoding || 'hex'
    const bytes = opts.bytes

    if (typeof seed === 'object' && !isBufferLike(seed)) {
      seed = JSON.stringify(seed)
    }

    if (!crypto) { // eslint-disable-next-line
      crypto = await new Function('return import(\'crypto\')')()
    }

    let value

    if (seed) {
      value = crypto.createHash('sha256').update(seed)
      if (bytes) return Buffer.from(value.digest(encoding), encoding)
      return value.digest(encoding)
    }

    value = randomBytes(32)
    if (opts.bytes) return value
    return value.toString(encoding)
  }
}

/**
 * The magic bytes prefixing every packet. They are the
 * 2nd, 3rd, 5th, and 7th, prime numbers.
 * @type {number[]}
 */
export const MAGIC_BYTES_PREFIX = [0x03, 0x05, 0x0b, 0x11]

/**
 * The version of the protocol.
 */
export const VERSION = 4

/**
 * The size in bytes of the prefix magic bytes.
 */
export const MAGIC_BYTES = 4

/**
 * The size in bytes of the `type` field.
 */
export const TYPE_BYTES = 1

/**
 * The size in bytes of the `version` field.
 */
export const VERSION_BYTES = 1

/**
 * The size in bytes of the `gops` field.
 */
export const HOPS_BYTES = 1

/**
 * The size in bytes of the `clock` field.
 */
export const CLOCK_BYTES = 4

/**
 * The size in bytes of the `index` field.
 */
export const INDEX_BYTES = 4

/**
 * The size in bytes of the `message_id` field.
 */
export const MESSAGE_ID_BYTES = 32

/**
 * The size in bytes of the `clusterId` field.
 */
export const CLUSTER_ID_BYTES = 32

/**
 * The size in bytes of the `cluster_id` field.
 */
export const SUBCLUSTER_ID_BYTES = 32

/**
 * The size in bytes of the `previous_id` field.
 */
export const PREVIOUS_ID_BYTES = 32

/**
 * The size in bytes of the `next_id` field.
 */
export const NEXT_ID_BYTES = 32

/**
 * The size in bytes of the `usr1` field.
 */
export const USR1_BYTES = 32

/**
 * The size in bytes of the `usr2` field.
 */
export const USR2_BYTES = 32

/**
 * The size in bytes of the `ttl` field.
 */
export const TTL_BYTES = 4

/**
 * The size in bytes of the `stream_to` field.
 */
export const STREAM_TO_BYTES = 32

/**
 * The size in bytes of the `stream_from` field.
 */
export const STREAM_FROM_BYTES = 32

/**
 * The size in bytes of the `message_length` field.
 */
export const MESSAGE_LENGTH_BYTES = 2

/**
 * The size in bytes of the `message_length` field.
 */
export const MESSAGE_SIG_BYTES = 64

/**
 * The size in bytes of the `message` field.
 */
export const MESSAGE_BYTES = 1024

/**
 * The size in bytes of the total packet frame.
 */
export const FRAME_BYTES = MAGIC_BYTES + TYPE_BYTES + VERSION_BYTES + HOPS_BYTES +
  CLOCK_BYTES + INDEX_BYTES + MESSAGE_ID_BYTES + SUBCLUSTER_ID_BYTES + PREVIOUS_ID_BYTES +
  NEXT_ID_BYTES + CLUSTER_ID_BYTES + STREAM_TO_BYTES + STREAM_FROM_BYTES + USR1_BYTES +
  USR2_BYTES + TTL_BYTES + MESSAGE_SIG_BYTES + MESSAGE_LENGTH_BYTES

/**
 * The size in bytes of the total packet frame and message.
 */
export const PACKET_BYTES = FRAME_BYTES + MESSAGE_BYTES

/**
 * The cache TTL in milliseconds.
 */
export const CACHE_TTL = 60_000 * 60 * 12

/**
 * @param {object} message
 * @param {{ [key: string]: { required: boolean, type: string }}} constraints
 */
export const validatePacket = (o, constraints) => {
  if (!o) throw new Error('Expected object')
  const allowedKeys = Object.keys(constraints)
  const actualKeys = Object.keys(o)
  const unknown = actualKeys.filter(k => allowedKeys.indexOf(k) === -1)
  if (unknown.length) throw new Error(`Unexpected keys [${unknown}]`)

  for (const [key, con] of Object.entries(constraints)) {
    if (con.required && !o[key]) {
      debug(new Error(`${key} is required (${JSON.stringify(o, null, 2)})`), JSON.stringify(o))
    }

    const type = ({}).toString.call(o[key]).slice(8, -1).toLowerCase()

    if (o[key] && type !== con.type) {
      debug(`expected .${key} to be of type ${con.type}, got ${type} in packet.. ` + JSON.stringify(o))
    }
  }
}

/**
 * Computes a SHA-256 hash of input returning a hex encoded string.
 * @type {function(string|Buffer|Uint8Array): Promise<string>}
 */
export const sha256 = createHashFunction()

/**
 * Decodes `buf` into a `Packet`.
 * @param {Buffer} buf
 * @return {Packet}
 */
export const decode = buf => {
  let offset = 0
  const o = {
    type: 0,
    version: 0,
    hops: 0,
    clock: 0,
    index: -1,
    packetId: null,
    previousId: null,
    nextId: null,
    streamTo: null,
    streamFrom: null,
    usr1: null,
    usr2: null,
    ttl: 0,
    clusterId: null,
    subclusterId: null,
    message: null,
    sig: null,
    timestamp: Date.now() // so we know when to delete if we don't subscribe to this cluster
  }

  // @ts-ignore
  if (buf.length < FRAME_BYTES) return Packet.from(o) // > is handled by MTU

  buf = Buffer.from(buf)

  // magic bytes prefix check
  if (!Packet.isPacket(buf)) {
    return null
  }

  buf = buf.slice(MAGIC_BYTES)

  // header
  o.type = Math.max(0, buf.readUInt8(offset)); offset += TYPE_BYTES
  o.version = Math.max(VERSION, buf.readUInt8(offset)); offset += VERSION_BYTES
  o.hops = Math.max(0, buf.readUInt8(offset) || 0); offset += HOPS_BYTES
  o.clock = Math.max(0, buf.readUInt32BE(offset) || 0); offset += CLOCK_BYTES
  o.index = Math.max(-1, buf.readInt32BE(offset)); offset += INDEX_BYTES

  // identifiers
  o.packetId = trim(buf.slice(offset, offset += MESSAGE_ID_BYTES)).toString('hex')
  o.previousId = trim(buf.slice(offset, offset += PREVIOUS_ID_BYTES)).toString('hex')
  o.nextId = trim(buf.slice(offset, offset += NEXT_ID_BYTES)).toString('hex')
  o.streamTo = trim(buf.slice(offset, offset += STREAM_TO_BYTES)).toString('hex')
  o.streamFrom = trim(buf.slice(offset, offset += STREAM_FROM_BYTES)).toString('hex')

  // extract 32-byte public-key (if any) and encode as base64 string
  // @ts-ignore
  o.clusterId = trim(buf.slice(offset, offset += CLUSTER_ID_BYTES)).toString('base64')
  o.subclusterId = trim(buf.slice(offset, offset += SUBCLUSTER_ID_BYTES)).toString('base64')
  o.usr1 = trim(buf.slice(offset, offset += USR1_BYTES))
  o.usr2 = trim(buf.slice(offset, offset += USR2_BYTES))
  o.ttl = Math.max(-1, buf.readInt32BE(offset)); offset += TTL_BYTES

  // extract the user message length
  const messageLen = Math.min(buf.readUInt16BE(offset), MESSAGE_BYTES); offset += MESSAGE_LENGTH_BYTES

  // extract the user message
  o.message = trim(buf.slice(offset, offset + messageLen)); offset += MESSAGE_BYTES

  if ((o.index === 0) || o.type !== 5) {
    try { o.message = JSON.parse(o.message.toString()) } catch {}
  }

  o.sig = trim(buf.slice(offset, offset += MESSAGE_SIG_BYTES))

  return Packet.from(o)
}

export const addHops = (buf, offset = MAGIC_BYTES + TYPE_BYTES + VERSION_BYTES) => {
  const hops = buf.readUInt8(offset)
  if (hops + 1 > 255) return buf
  buf.writeUInt8(hops + 1, offset)
  return buf
}

export class Packet {
  type = 0
  version = VERSION
  clock = 0
  hops = 0
  index = -1
  previousId = ''
  packetId = ''
  nextId = ''
  clusterId = ''
  subclusterId = ''
  streamTo = ''
  streamFrom = ''
  usr1 = ''
  usr2 = ''
  ttl = 0
  message = ''

  static ttl = CACHE_TTL
  static maxLength = MESSAGE_BYTES

  #buf = null

  /**
   * Returns an empty `Packet` instance.
   * @return {Packet}
   */
  static empty () {
    return new this()
  }

  /**
   * @param {Packet|object} packet
   * @return {Packet}
   */
  static from (packet) {
    if (packet instanceof Packet) {
      return new this(packet)
    }

    return Object.assign(new this({}), packet)
  }

  /**
   * Determines if input is a packet.
   * @param {Buffer|Uint8Array|number[]|object|Packet} packet
   * @return {boolean}
   */
  static isPacket (packet) {
    if (isBufferLike(packet) || Array.isArray(packet)) {
      const prefix = Buffer.from(packet).slice(0, MAGIC_BYTES)
      const magic = Buffer.from(MAGIC_BYTES_PREFIX)
      return magic.compare(prefix) === 0
    } else if (packet && typeof packet === 'object') {
      const empty = Packet.empty()
      // check if every key on `Packet` exists in `packet`
      return Object.keys(empty).every(k => k in packet)
    }

    return false
  }

  /**
   * `Packet` class constructor.
   * @param {Packet|object?} options
   */
  constructor (options = {}) {
    this.type = options?.type || 0
    this.version = VERSION
    this.clock = options?.clock || 0
    this.hops = options?.hops || 0
    this.index = typeof options?.index === 'undefined' ? -1 : options.index
    this.clusterId = options?.clusterId || ''
    this.subclusterId = options?.subclusterId || ''
    this.previousId = options?.previousId || ''
    this.packetId = options?.packetId || ''
    this.nextId = options?.nextId || ''
    this.streamTo = options?.streamTo || ''
    this.streamFrom = options?.streamFrom || ''
    this.usr1 = options?.usr1 || ''
    this.usr2 = options?.usr2 || ''
    this.ttl = options?.ttl || 0
    this.message = options?.message || ''
    this.sig = options?.sig || ''
  }

  /**
  */
  static async encode (p) {
    p = { ...p }

    const buf = Buffer.alloc(PACKET_BYTES) // buf length bust be < UDP MTU (usually ~1500)
    if (!p.message) return buf

    const isBuffer = isBufferLike(p.message)
    const isObject = typeof p.message === 'object'

    if (p.index <= 0 && isObject && !isBuffer) {
      p.message = JSON.stringify(p.message)
    } else if (p.index <= 0 && typeof p !== 'string' && !isBuffer) {
      p.message = String(p.message)
    }

    if (p.message?.length > Packet.MESSAGE_BYTES) throw new Error('ETOOBIG')

    // we only have p.nextId when we know ahead of time, if it's empty that's fine.
    p.packetId = p.packetId || await sha256(p.previousId + p.message + p.nextId)

    let offset = 0

    if (p.clock === 2e9) p.clock = 0

    // magic bytes
    Buffer.from(MAGIC_BYTES_PREFIX).copy(buf, offset); offset += MAGIC_BYTES

    // header
    offset = buf.writeUInt8(p.type, offset)
    offset = buf.writeUInt8(p.version, offset)
    offset = buf.writeUInt8(p.hops, offset)
    offset = buf.writeUInt32BE(p.clock, offset)
    offset = buf.writeInt32BE(p.index, offset)

    // identifiers
    Buffer.from(p.packetId, 'hex').copy(buf, offset); offset += MESSAGE_ID_BYTES
    Buffer.from(p.previousId, 'hex').copy(buf, offset); offset += PREVIOUS_ID_BYTES
    Buffer.from(p.nextId, 'hex').copy(buf, offset); offset += NEXT_ID_BYTES
    Buffer.from(p.streamTo, 'hex').copy(buf, offset); offset += STREAM_TO_BYTES
    Buffer.from(p.streamFrom, 'hex').copy(buf, offset); offset += STREAM_FROM_BYTES

    Buffer.from(p.clusterId, 'base64').copy(buf, offset); offset += CLUSTER_ID_BYTES
    Buffer.from(p.subclusterId, 'base64').copy(buf, offset); offset += SUBCLUSTER_ID_BYTES
    Buffer.from(p.usr1).copy(buf, offset); offset += USR1_BYTES
    Buffer.from(p.usr2).copy(buf, offset); offset += USR2_BYTES
    offset = buf.writeInt32BE(p.ttl, offset)

    const msgBuf = Buffer.from(p.message)

    // encode message length into packet
    buf.writeUInt16BE(msgBuf.byteLength, offset); offset += MESSAGE_LENGTH_BYTES

    // encode message into packet
    msgBuf.copy(buf, offset); offset += MESSAGE_BYTES
    Buffer.from(p.sig).copy(buf, offset); offset += MESSAGE_SIG_BYTES

    return buf
  }

  static decode (buf) {
    return decode(buf)
  }
}

export class PacketPing extends Packet {
  static type = 1
  constructor ({ message }) {
    super({ type: PacketPing.type, message })

    validatePacket(message, {
      requesterPeerId: { required: true, type: 'string' },
      cacheSummaryHash: { type: 'string' },
      probeExternalPort: { type: 'number' },
      reflectionId: { type: 'string' },
      pingId: { type: 'string' },
      natType: { type: 'number' },
      uptime: { type: 'number' },
      isHeartbeat: { type: 'number' },
      subclusterId: { type: 'string' },
      cacheSize: { type: 'number' },
      isConnection: { type: 'boolean' },
      isReflection: { type: 'boolean' },
      isProbe: { type: 'boolean' },
      isDebug: { type: 'boolean' },
      timestamp: { type: 'number' }
    })
  }
}

export class PacketPong extends Packet {
  static type = 2
  constructor ({ message }) {
    super({ type: PacketPong.type, message })

    validatePacket(message, {
      requesterPeerId: { required: true, type: 'string' },
      responderPeerId: { required: true, type: 'string' },
      subclusterId: { type: 'string' },
      port: { type: 'number' },
      address: { required: true, type: 'string' },
      uptime: { type: 'number' },
      cacheSize: { type: 'number' },
      natType: { type: 'number' },
      isReflection: { type: 'boolean' },
      isHeartbeat: { type: 'number' },
      isConnection: { type: 'boolean' },
      reflectionId: { type: 'string' },
      pingId: { type: 'string' },
      isDebug: { type: 'boolean' },
      isProbe: { type: 'boolean' },
      rejected: { type: 'boolean' }
    })
  }
}

export class PacketIntro extends Packet {
  static type = 3
  constructor ({ clock, hops, clusterId, subclusterId, message }) {
    super({ type: PacketIntro.type, clock, hops, clusterId, subclusterId, message })

    validatePacket(message, {
      subclusterId: { type: 'string' },
      requesterPeerId: { required: true, type: 'string' },
      responderPeerId: { required: true, type: 'string' },
      isRendezvous: { type: 'boolean' },
      natType: { required: true, type: 'number' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' },
      timestamp: { required: true, type: 'number' }
    })
  }
}

export class PacketJoin extends Packet {
  static type = 4
  constructor ({ clock, hops, clusterId, subclusterId, message }) {
    super({ type: PacketJoin.type, clock, hops, clusterId, subclusterId, message })

    validatePacket(message, {
      rendezvousAddress: { type: 'string' },
      rendezvousPort: { type: 'number' },
      rendezvousType: { type: 'number' },
      rendezvousPeerId: { type: 'string' },
      rendezvousDeadline: { type: 'number' },
      rendezvousRequesterPeerId: { type: 'string' },
      subclusterId: { type: 'string' },
      requesterPeerId: { required: true, type: 'string' },
      natType: { required: true, type: 'number' },
      initial: { type: 'boolean' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' }
    })
  }
}

export class PacketPublish extends Packet {
  static type = 5 // no need to validatePacket, message is whatever you want
  constructor ({ message, sig, packetId, clusterId, subclusterId, nextId, clock, hops, to, usr1, usr2, ttl, previousId }) {
    super({ type: PacketPublish.type, message, sig, packetId, clusterId, subclusterId, nextId, clock, hops, usr1, usr2, ttl, previousId })
  }
}

export class PacketStream extends Packet {
  static type = 6
  constructor ({ message, sig, packetId, clusterId, subclusterId, nextId, clock, usr1, usr2, ttl, streamTo, streamFrom, previousId }) {
    super({ type: PacketStream.type, message, sig, packetId, clusterId, subclusterId, nextId, clock, usr1, usr2, ttl, streamTo, streamFrom, previousId })
  }
}

export class PacketSync extends Packet {
  static type = 7
  constructor ({ packetId, message = '' }) {
    super({ type: PacketSync.type, packetId, message })
  }
}

export class PacketQuery extends Packet {
  static type = 8
  constructor ({ packetId, subclusterId, message = {} }) {
    super({ type: PacketQuery.type, packetId, subclusterId, message })

    validatePacket(message, {
      history: { type: 'Array' }
    })
  }
}

export default Packet
