import { sodium, randomBytes } from '../crypto.js'
import { isBufferLike } from '../util.js'
import { Buffer } from '../buffer.js'

// trims buffer/string of 0 bytes (\x00)
const trim = (/** @type {Buffer} */ buf) => buf.toString().split('\x00')[0]
// converts buffer/string to hex
const toHex = (/** @type {Buffer} */ b) => normalizeBuffer(b).toString('hex')

/**
 * return empty buffer if input buffer contains all zeroes was given
 * @param {number[]|Buffer|Uint8Array} buf
 * @return {Buffer}
 */
function normalizeBuffer (buf) {
  return /** @type {number[]} */ (buf).find((b) => b !== 0)
    ? Buffer.from(buf)
    : Buffer.from('')
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

    return async (seed) => {
      if (typeof seed === 'object' && !isBufferLike(seed)) {
        seed = JSON.stringify(seed)
      }

      if (seed && typeof seed === 'string') {
        seed = encoder.encode(seed)
      }

      if (seed) {
        return Buffer.from(await crypto.digest('SHA-256', seed)).toString('hex')
      }

      return Buffer.from(randomBytes(32)).toString('hex')
    }
  }

  return async (seed) => {
    if (typeof seed === 'object' && !isBufferLike(seed)) {
      seed = JSON.stringify(seed)
    }

    if (!crypto) { // eslint-disable-next-line
      crypto = await new Function('return import(\'crypto\')')()
    }

    if (seed) {
      return crypto.createHash('sha256').update(seed).digest('hex')
    }

    return randomBytes(32).toString('hex')
  }
}

/**
 * The version of the protocol.
 */
export const VERSION = 1

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
export const HOPS_BYTES = 4

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
 * The size in bytes of the `cluster_id` field.
 */
export const CLUSTER_ID_BYTES = 32

/**
 * The size in bytes of the `previous_id` field.
 */
export const PREVIOUS_ID_BYTES = 32

/**
 * The size in bytes of the `next_id` field.
 */
export const NEXT_ID_BYTES = 32

/**
 * The size in bytes of the `to` field.
 */
export const TO_BYTES = 32

/**
 * The size in bytes of the `usr1` field.
 */
export const USR1_BYTES = 32

/**
 * The size in bytes of the `message_length` field.
 */
export const MESSAGE_LENGTH_BYTES = 2

/**
 * The size in bytes of the `message` field.
 */
export const MESSAGE_BYTES = 1024

/**
 * The size in bytes of the total packet frame.
 */
export const FRAME_BYTES = TYPE_BYTES + VERSION_BYTES + HOPS_BYTES +
  CLOCK_BYTES + INDEX_BYTES + MESSAGE_ID_BYTES + CLUSTER_ID_BYTES +
  PREVIOUS_ID_BYTES + NEXT_ID_BYTES + TO_BYTES + USR1_BYTES + MESSAGE_LENGTH_BYTES

/**
 * The size in bytes of the total packet frame and message.
 */
export const PACKET_BYTES = FRAME_BYTES + MESSAGE_BYTES

/**
 * The cache TTL in milliseconds.
 */
export const CACHE_TTL = 60_000 * 60 * 24

/**
 * @param {object} message
 * @param {{ [key: string]: { required: boolean, type: string }}} constraints
 */
export const validatePacket = (message, constraints) => {
  if (!message) throw new Error('Expected message object')
  const allowedKeys = Object.keys(constraints)
  const actualKeys = Object.keys(message)
  const unknown = actualKeys.filter(k => allowedKeys.indexOf(k) === -1)
  if (unknown.length) throw new Error(`Unexpected keys [${unknown}]`)

  for (const [key, con] of Object.entries(constraints)) {
    if (con.required && !message[key]) {
      throw new Error(`${key} is required (${JSON.stringify(message, null, 2)})`)
    }

    const type = ({}).toString.call(message[key]).slice(8, -1).toLowerCase()
    if (message[key] && type !== con.type) {
      throw new Error(`expected .${key} to be of type ${con.type}, got ${type}`)
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
    clusterId: null,
    previousId: null,
    nextId: null,
    to: null,
    usr1: null,
    message: null,
    timestamp: Date.now() // so we know when to delete if we don't subscribe to this cluster
  }

  // @ts-ignore
  if (buf.length < FRAME_BYTES) return Packet.from(o) // > is handled by MTU

  buf = Buffer.from(buf)

  // header
  o.type = Math.max(0, buf.readInt8(offset)); offset += TYPE_BYTES
  o.version = Math.max(VERSION, buf.readInt8(offset)); offset += VERSION_BYTES
  o.hops = Math.max(0, buf.readUInt32BE(offset)); offset += HOPS_BYTES
  o.clock = Math.max(0, buf.readUInt32BE(offset)); offset += CLOCK_BYTES
  o.index = Math.max(-1, buf.readInt32BE(offset)); offset += INDEX_BYTES

  // identifiers
  o.packetId = toHex(buf.slice(offset, offset += MESSAGE_ID_BYTES))
  o.clusterId = toHex(buf.slice(offset, offset += CLUSTER_ID_BYTES))
  o.previousId = toHex(buf.slice(offset, offset += PREVIOUS_ID_BYTES))
  o.nextId = toHex(buf.slice(offset, offset += NEXT_ID_BYTES))

  // extract 32-byte public-key (if any) and encode as base64 string
  const b = buf.slice(offset, offset += TO_BYTES)
  // @ts-ignore
  o.to = sodium.to_base64(b, sodium.base64_variants.ORIGINAL)
  o.usr1 = trim(buf.slice(offset, offset += USR1_BYTES))

  // extract the user message length
  const messageLen = Math.min(buf.readUInt16BE(offset), MESSAGE_BYTES); offset += MESSAGE_LENGTH_BYTES

  // extract the user message
  o.message = normalizeBuffer(buf.slice(offset, offset + messageLen)); offset += MESSAGE_BYTES

  if ((o.index <= 0) || (o.type !== 5)) { // internal packets are parsed as JSON
    try { o.message = JSON.parse(trim(o.message)) } catch {}
  }

  return Packet.from(o)
}

export const addHops = (buf, offset = TYPE_BYTES + VERSION_BYTES) => {
  const hops = buf.readUInt32BE(offset)
  buf.writeUInt32BE(hops + 1, offset)
  return buf
}

// const debug = Debug('packet')
export class Packet {
  type = 0
  version = VERSION
  clock = 0
  index = -1
  clusterId = ''
  previousId = ''
  packetId = ''
  nextId = ''
  to = ''
  usr1 = ''
  message = ''

  static ttl = CACHE_TTL
  static maxLength = MESSAGE_BYTES

  #buf = null

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
   * `Packet` class constructor.
   * @param {Packet|object?} options
   */
  constructor (options = {}) {
    this.type = options?.type || 0
    this.version = VERSION
    this.clock = options?.clock || 0
    this.index = typeof options?.index === 'undefined' ? -1 : options.index
    this.clusterId = options?.clusterId || ''
    this.previousId = options?.previousId || ''
    this.packetId = options?.packetId || ''
    this.nextId = options?.nextId || ''
    this.to = options?.to || ''
    this.usr1 = options?.usr1 || ''
    this.message = options?.message || ''
  }

  /**
   */
  static async encode (p) {
    p = { ...p }

    const buf = Buffer.alloc(PACKET_BYTES) // buf length bust be < UDP MTU (usually ~1500)

    const isBuffer = isBufferLike(p.message)

    if (p.index <= 0 && typeof p.message === 'object' && !isBuffer) {
      p.message = JSON.stringify(p.message)
    }

    if (p.index <= 0 && typeof p !== 'string' && !isBuffer) {
      p.message = String(p.message)
    }

    if (p.message.length > Packet.MESSAGE_BYTES) throw new Error('ETOOBIG')

    // we only have p.nextId when we know ahead of time, if it's empty that's fine.
    p.packetId = p.packetId || await sha256(p.previousId + p.message + p.nextId)

    let offset = 0

    if (p.clock === 2e9) p.clock = 0

    // header
    offset = buf.writeInt8(p.type, offset)
    offset = buf.writeInt8(p.version, offset)
    offset = buf.writeInt32BE(p.hops, offset)
    offset = buf.writeInt32BE(p.clock, offset)
    offset = buf.writeInt32BE(p.index, offset)

    // identifiers
    Buffer.from(p.packetId, 'hex').copy(buf, offset); offset += MESSAGE_ID_BYTES
    Buffer.from(p.clusterId, 'hex').copy(buf, offset); offset += CLUSTER_ID_BYTES
    Buffer.from(p.previousId, 'hex').copy(buf, offset); offset += PREVIOUS_ID_BYTES
    Buffer.from(p.nextId, 'hex').copy(buf, offset); offset += NEXT_ID_BYTES

    // base64-encoded public-key string, for the `to` field?
    if (typeof p.to === 'string' && p.to.length === 44) {
      // copy the underlying 32 bytes into the packet
      const b = sodium.from_base64(p.to, sodium.base64_variants.ORIGINAL).buffer
      Buffer.from(b).copy(buf, offset)
      offset += TO_BYTES
    } else {
      // set the `to` field's bytes to all null-bytes
      Buffer.from(Array.from({ length: TO_BYTES }).fill(0x00)).copy(buf, offset)
      offset += TO_BYTES
    }

    p.usr1 = Buffer.from(p.usr1).copy(buf, offset); offset += USR1_BYTES

    // user message
    if (typeof p.message === 'number') p.message = String(p.message)

    // encoding encrypted 'publish' packet?
    const msgBuf = (p.type === PacketPublish.type && p.message?.buffer)
      ? Buffer.from(p.message.buffer) // copy raw bytes as-is
      : Buffer.from(p.message, 'utf8') // copy message as string

    // encode message length into packet
    buf.writeUInt16BE(msgBuf.byteLength, offset); offset += MESSAGE_LENGTH_BYTES

    // encode message into packet
    msgBuf.copy(buf, offset); offset += MESSAGE_BYTES

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
      peerId: { required: true, type: 'string' },
      pingId: { type: 'string' },
      natType: { type: 'string' },
      isHeartbeat: { type: 'boolean' },
      clusterId: { type: 'string' },
      cacheSize: { type: 'number' },
      isConnection: { type: 'boolean' },
      isReflection: { type: 'boolean' },
      testPort: { type: 'number' }
    })
  }
}

export class PacketPong extends Packet {
  static type = 2
  constructor ({ message }) {
    super({ type: PacketPong.type, message })

    validatePacket(message, {
      clusterId: { type: 'string' },
      peerId: { required: true, type: 'string' },
      port: { type: 'number' },
      testPort: { type: 'number' },
      address: { required: true, type: 'string' },
      cacheSize: { type: 'number' },
      natType: { type: 'string' },
      isReflection: { type: 'boolean' },
      isConnection: { type: 'boolean' },
      pingId: { type: 'string' }
    })
  }
}

export class PacketIntro extends Packet {
  static type = 3
  constructor ({ message }) {
    super({ type: PacketIntro.type, message })

    validatePacket(message, {
      clusterId: { type: 'string' },
      peerId: { required: true, type: 'string' },
      natType: { required: true, type: 'string' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' }
    })
  }
}

export class PacketJoin extends Packet {
  static type = 4
  constructor ({ message }) {
    super({ type: PacketJoin.type, message })

    validatePacket(message, {
      clusterId: { type: 'string' },
      peerId: { required: true, type: 'string' },
      natType: { required: true, type: 'string' },
      initial: { type: 'boolean' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' }
    })
  }
}

export class PacketPublish extends Packet {
  static type = 5 // no need to validatePacket, message is whatever you want
  constructor ({ message, packetId, clusterId, nextId, clock, to, usr1, previousId }) {
    super({ type: PacketPublish.type, message, packetId, clusterId, nextId, clock, to, usr1, previousId })
  }
}

export class PacketAsk extends Packet {
  static type = 6
  constructor ({ packetId, clusterId, message = {} }) {
    super({ type: PacketAsk.type, packetId, clusterId, message })

    validatePacket(message, {
      previousId: { type: 'string' },
      packetId: { type: 'string' },
      peerId: { type: 'string' }
    })
  }
}

export class PacketAnswer extends Packet {
  static type = 7
  constructor (args) { super({ type: PacketAnswer.type, ...args }) }
}

export class PacketQuery extends Packet {
  static type = 8
  constructor ({ packetId, clusterId, message = {} }) {
    super({ type: PacketAsk.type, packetId, clusterId, message })

    validatePacket(message, {
      history: { type: 'Array' }
    })
  }
}

export default Packet
