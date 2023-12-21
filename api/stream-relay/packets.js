import { randomBytes } from '../crypto.js'
import { isBufferLike } from '../util.js'
import { Buffer } from '../buffer.js'
import debug from './index.js'

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

const getMethod = (type, bytes, isSigned) => {
  const bits = bytes << 3
  const isBigEndian = bits === 8 ? '' : 'BE'

  if (![8, 16, 32].includes(bits)) {
    throw new Error(`${bits} is invalid, expected 8, 16, or 32`)
  }

  return `${type}${isSigned ? '' : 'U'}Int${bits}${isBigEndian}`
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
export const VERSION = 6

/**
 * The size in bytes of the prefix magic bytes.
 */
export const MAGIC_BYTES = 4

/**
 * The maximum size of the user message.
 */
export const MESSAGE_BYTES = 1024

/**
 * The cache TTL in milliseconds.
 */
export const CACHE_TTL = 60_000 * 60 * 6

export const PACKET_SPEC = {
  type: { bytes: 1, encoding: 'number' },
  version: { bytes: 2, encoding: 'number', default: VERSION },
  clock: { bytes: 4, encoding: 'number', default: 1 },
  hops: { bytes: 4, encoding: 'number', default: 0 },
  index: { bytes: 4, encoding: 'number', default: -1, signed: true },
  ttl: { bytes: 4, encoding: 'number', default: CACHE_TTL },
  clusterId: { bytes: 32, encoding: 'base64', default: [0b0] },
  subclusterId: { bytes: 32, encoding: 'base64', default: [0b0] },
  previousId: { bytes: 32, encoding: 'hex', default: [0b0] },
  packetId: { bytes: 32, encoding: 'hex', default: [0b0] },
  nextId: { bytes: 32, encoding: 'hex', default: [0b0] },
  usr1: { bytes: 32, default: [0b0] },
  usr2: { bytes: 32, default: [0b0] },
  usr3: { bytes: 32, default: [0b0] },
  usr4: { bytes: 32, default: [0b0] },
  message: { bytes: 1024, default: [0b0] },
  sig: { bytes: 64, default: [0b0] }
}

let FRAME_BYTES = MAGIC_BYTES

for (const spec of Object.values(PACKET_SPEC)) {
  FRAME_BYTES += spec.bytes
}

/**
 * The size in bytes of the total packet frame and message.
 */
export const PACKET_BYTES = FRAME_BYTES + MESSAGE_BYTES

/**
 * The maximum distance that a packet can be replicated.
 */
export const MAX_HOPS = 16

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
 * Used to store the size of each field
 */
const SIZE = 2

const isEncodedAsJSON = ({ type, index }) => (
  type === PacketPing.type ||
  type === PacketPong.type ||
  type === PacketJoin.type ||
  type === PacketIntro.type ||
  type === PacketQuery.type ||
  index === 0
)

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
  if (!Packet.isPacket(buf)) return null

  buf = buf.slice(MAGIC_BYTES)

  const o = new Packet()
  let offset = 0

  for (const [k, spec] of Object.entries(PACKET_SPEC)) {
    o[k] = spec.default

    try {
      if (spec.encoding === 'number') {
        const method = getMethod('read', spec.bytes, spec.signed)
        o[k] = buf[method](offset)
        offset += spec.bytes
        continue
      }

      const size = buf.readUInt16BE(offset)
      offset += SIZE
      let value = buf.slice(offset, offset + size)
      offset += size

      if (spec.bytes && size > spec.bytes) return null

      if (spec.encoding === 'hex') value = Buffer.from(value, 'hex')
      if (spec.encoding === 'base64') value = Buffer.from(value, 'base64')
      if (spec.encoding === 'utf8') value = value.toString()

      if (k === 'message' && isEncodedAsJSON(o)) {
        try { value = JSON.parse(value.toString()) } catch {}
      }

      o[k] = value
    } catch (err) {
      return null // completely bail
    }
  }

  return o
}

export const getTypeFromBytes = (buf) => buf.byteLength > 4 ? buf.readUInt8(4) : 0

export class Packet {
  static ttl = CACHE_TTL
  static maxLength = MESSAGE_BYTES

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
    if (packet instanceof Packet) return packet
    return new Packet(packet)
  }

  /**
   * @param {Packet} packet
   * @return {Packet}
   */
  copy () {
    return Object.assign(new Packet({}), this)
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
      // check if every key on `Packet` exists in `packet`
      return Object.keys(PACKET_SPEC).every(k => k in packet)
    }

    return false
  }

  /**
   * `Packet` class constructor.
   * @param {Packet|object?} options
   */
  constructor (options = {}) {
    for (const [k, v] of Object.entries(PACKET_SPEC)) {
      this[k] = typeof options[k] === 'undefined'
        ? v.default
        : options[k]

      if (Array.isArray(this[k]) || ArrayBuffer.isView(this[k])) {
        this[k] = Buffer.from(this[k])
      }
    }

    // extras that might not come over the wire
    this.timestamp = options.timestamp || Date.now()
    this.isComposed = options.isComposed || false
    this.isReconciled = options.isReconciled || false
    this.meta = options.meta || {}
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
    if (p.packetId.length === 1 && p.packetId[0] === 0) {
      const bufs = [p.previousId, p.message, p.nextId].map(v => Buffer.from(v))
      p.packetId = await sha256(Buffer.concat(bufs), { bytes: true })
    }

    if (p.clock === 2e9) p.clock = 0

    const bufs = [Buffer.from(MAGIC_BYTES_PREFIX)]

    // an encoded packet has a fixed number of fields each with variable length
    // the order of bufs will be consistent regardless of the field order in p
    for (const [k, spec] of Object.entries(PACKET_SPEC)) {
      // if p[k] is larger than specified in the spec, throw

      const value = p[k] || spec.default

      if (spec.encoding === 'number') {
        const buf = Buffer.alloc(spec.bytes)
        const value = typeof p[k] !== 'undefined' ? p[k] : spec.default
        const bytesRequired = (32 - Math.clz32(value) >> 3) || 1

        if (bytesRequired > spec.bytes) {
          throw new Error(`key=${k}, value=${value} bytes=${bytesRequired}, max-bytes=${spec.bytes}, encoding=${spec.encoding}`)
        }

        const method = getMethod('write', spec.bytes, spec.signed)
        buf[method](value)
        bufs.push(buf)
        continue
      }

      // console.log(k, value, spec)
      const encoded = Buffer.from(value || spec.default, spec.encoding)

      if (value?.length && encoded.length > spec.bytes) {
        throw new Error(`${k} is invalid, ${value.length} is greater than ${spec.bytes} (encoding=${spec.encoding})`)
      }

      // create a buffer from the size of the field and the actual value of p[k]
      const bytes = value.length
      const buf = Buffer.alloc(SIZE + bytes)
      const offset = buf.writeUInt16BE(bytes)
      encoded.copy(buf, offset)
      bufs.push(buf)
    }

    return Buffer.concat(bufs)
  }

  static decode (buf) {
    return decode(buf)
  }
}

export class PacketPing extends Packet {
  static type = 1
  constructor ({ message, clusterId, subclusterId }) {
    super({ type: PacketPing.type, message, clusterId, subclusterId })

    validatePacket(message, {
      requesterPeerId: { required: true, type: 'string' },
      cacheSummaryHash: { type: 'string' },
      probeExternalPort: { type: 'number' },
      reflectionId: { type: 'string' },
      pingId: { type: 'string' },
      natType: { type: 'number' },
      uptime: { type: 'number' },
      isHeartbeat: { type: 'number' },
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
  constructor ({ message, clusterId, subclusterId }) {
    super({ type: PacketPong.type, message, clusterId, subclusterId })

    validatePacket(message, {
      requesterPeerId: { required: true, type: 'string' },
      responderPeerId: { required: true, type: 'string' },
      cacheSummaryHash: { type: 'string' },
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
      requesterPeerId: { required: true, type: 'string' },
      natType: { required: true, type: 'number' },
      initial: { type: 'boolean' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' },
      isConnection: { type: 'boolean' }
    })
  }
}

export class PacketPublish extends Packet {
  static type = 5 // no need to validatePacket, message is whatever you want
  constructor ({ message, sig, packetId, clusterId, subclusterId, nextId, clock, hops, usr1, usr2, ttl, previousId }) {
    super({ type: PacketPublish.type, message, sig, packetId, clusterId, subclusterId, nextId, clock, hops, usr1, usr2, ttl, previousId })
  }
}

export class PacketStream extends Packet {
  static type = 6
  constructor ({ message, sig, packetId, clusterId, subclusterId, nextId, clock, ttl, usr1, usr2, usr3, usr4, previousId }) {
    super({ type: PacketStream.type, message, sig, packetId, clusterId, subclusterId, nextId, clock, ttl, usr1, usr2, usr3, usr4, previousId })
  }
}

export class PacketSync extends Packet {
  static type = 7
  constructor ({ packetId, message = Buffer.from([0b0]) }) {
    super({ type: PacketSync.type, packetId, message })
  }
}

export class PacketQuery extends Packet {
  static type = 8
  constructor ({ packetId, previousId, subclusterId, usr1, usr2, usr3, usr4, message = {} }) {
    super({ type: PacketQuery.type, packetId, previousId, subclusterId, usr1, usr2, usr3, usr4, message })
  }
}

export default Packet
