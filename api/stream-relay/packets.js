import { isBufferLike } from 'socket:util'
import { Buffer } from 'socket:buffer'
import { sodium, randomBytes } from 'socket:crypto'

const VERSION = 1
const TYPE_BYTES = 1
const VERSION_BYTES = 1
const HOPS_BYTES = 4
const CLOCK_BYTES = 4
const INDEX_BYTES = 4
const MESSAGE_ID_BYTES = 32
const CLUSTER_ID_BYTES = 32
const PREVIOUS_ID_BYTES = 32
const NEXT_ID_BYTES = 32
const TO_BYTES = 32
const USR1_BYTES = 32
const MESSAGE_LENGTH_BYTES = 2
const MESSAGE_BYTES = 1024
const CACHE_TTL = 60_000 * 60 * 24

const FRAME_SIZE = TYPE_BYTES + VERSION_BYTES + HOPS_BYTES +
  CLOCK_BYTES + INDEX_BYTES + MESSAGE_ID_BYTES + CLUSTER_ID_BYTES +
  PREVIOUS_ID_BYTES + NEXT_ID_BYTES + TO_BYTES + USR1_BYTES + MESSAGE_LENGTH_BYTES

export const PACKET_SIZE = FRAME_SIZE + MESSAGE_BYTES

// trims buffer/string of 0 bytes (\x00)
const trim = buf => buf.toString().split('\x00')[0]
// return '' if a buffer of all zeroes was given
const normalizeBuffer = buf => buf.find(b => b !== 0) ? Buffer.from(buf) : Buffer.from('')
// converts buffer/string to hex
const toHex = buf => normalizeBuffer(buf).toString('hex')

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

export const validatePacket = (message, constraints) => {
  if (!message) throw new Error('Expected message object')
  const allowedKeys = Object.keys(constraints)
  const actualKeys = Object.keys(message)
  const unknown = actualKeys.filter(k => allowedKeys.indexOf(k) === -1)
  if (unknown.length) throw new Error(`Unexpected keys [${unknown}]`)

  for (const [key, con] of Object.entries(constraints)) {
    if (con.required && !message[key]) {
      throw new Error(`${key} is required (${JSON.stringify(message, 2, 2)})`)
    }

    const type = ({}).toString.call(message[key]).slice(8, -1).toLowerCase()
    if (message[key] && type !== con.type) {
      throw new Error(`expected .${key} to be of type ${con.type}, got ${type}`)
    }
  }
}

export const sha256 = createHashFunction()

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

  if (buf.length < FRAME_SIZE) return o // > is handled by MTU

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
  o.to = sodium.to_base64(b, sodium.base64_variants.ORIGINAL)
  o.usr1 = trim(buf.slice(offset, offset += USR1_BYTES))

  // extract the user message length
  const messageLen = Math.min(buf.readUInt16BE(offset), MESSAGE_BYTES); offset += MESSAGE_LENGTH_BYTES

  // extract the user message
  o.message = normalizeBuffer(buf.slice(offset, offset + messageLen)); offset += MESSAGE_BYTES

  if ((o.index <= 0) || (o.type !== 5)) { // internal packets are parsed as JSON
    try { o.message = JSON.parse(trim(o.message)) } catch {}
  }

  return o
}

export const addHops = (buf, offset = TYPE_BYTES + VERSION_BYTES) => {
  const hops = buf.readUInt32BE(offset)
  buf.writeUInt32BE(hops + 1, offset)
  return buf
}

// const debug = Debug('packet')
export class Packet {
  packetId = null
  hops = 0

  static ttl = CACHE_TTL
  static maxLength = MESSAGE_BYTES

  #buf = null

  constructor ({ type, packetId, previousId, index, nextId, clock, to, usr1, message, clusterId }) {
    this.type = type || 0
    this.version = VERSION
    this.clock = clock || 0
    this.index = typeof index === 'undefined' ? -1 : index
    this.clusterId = clusterId || ''
    this.previousId = previousId || ''
    this.packetId = packetId || ''
    this.nextId = nextId || ''
    this.to = to || ''
    this.usr1 = usr1 || ''
    this.message = message || ''
  }

  static async encode (p) {
    p = { ...p }

    const buf = Buffer.alloc(PACKET_SIZE) // buf length bust be < UDP MTU (usually ~1500)

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
      geospatial: { type: 'number' },
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
      tail: { type: 'boolean' },
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
