import { Buffer } from 'socket:buffer.js'

const TYPE_BYTES = 1
const POSTAGE_BYTES = 1
const INDEX_BYTES = 4
const MESSAGE_ID_BYTES = 32
const TOPIC_ID_BYTES = 32
const PREVIOUS_ID_BYTES = 32
const NEXT_ID_BYTES = 32
const MESSAGE_BYTES = 1024
const CACHE_TTL = 60_000 * 60 * 6

// trims buffer/string of 0 bytes (\x00)
const trim = buf => buf.toString().split('\x00')[0]
// return '' if a buffer of all zeroes was given
const normalizeBuffer = buf => buf.find(b => b !== 0) ? Buffer.from(buf) : Buffer.from('')
// converts buffer/string to hex
const toHex = buf => normalizeBuffer(buf).toString('hex')

function createHashFunction () {
  const encoder = new TextEncoder()
  let crypto = null

  return async (seed) => {
    if (typeof seed === 'object' && seed.constructor.name !== 'Buffer') {
      seed = JSON.stringify(seed)
    }

    if (typeof globalThis.window !== 'undefined') {
      if (!crypto) {
        crypto = globalThis.crypto?.subtle
      }

      if (typeof seed === 'string') {
        seed = encoder.encode(seed)
      }

      return Buffer.from(await crypto.digest('SHA-256', seed)).toString('hex')
    } else {
      if (!crypto) { // eslint-disable-next-line
        crypto = await new Function('return import(\'crypto\')')()
      }

      if (seed) {
        return crypto.createHash('sha256').update(seed).digest('hex')
      }

      return crypto.randomBytes(32).toString('hex')
    }
  }
}

export const validate = (message, constraints) => {
  if (!message) throw new Error('Expected message object')
  const allowedKeys = Object.keys(constraints)
  const actualKeys = Object.keys(message)
  const unknown = actualKeys.filter(k => allowedKeys.indexOf(k) === -1)
  if (unknown.length) throw new Error(`Unexpected keys [${unknown}]`)

  for (const [key, con] of Object.entries(constraints)) {
    if (con.required && !message[key]) {
      throw new Error(`${key} is required`)
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
    postage: 16,
    index: -1,
    packetId: null,
    topicId: null,
    previousId: null,
    nextId: null,
    message: null,
    timestamp: Date.now() // so we know when to delete if we don't subscribe to this topic
  }

  // header
  o.type = Math.max(0, buf.readInt8(offset)); offset += TYPE_BYTES
  o.postage = Math.min(16, buf.readInt8(offset)); offset += POSTAGE_BYTES
  o.index = Math.max(-1, buf.readInt32BE(offset)); offset += INDEX_BYTES

  // identifiers
  o.packetId = toHex(buf.slice(offset, offset += MESSAGE_ID_BYTES))
  o.topicId = toHex(buf.slice(offset, offset += TOPIC_ID_BYTES))
  o.previousId = toHex(buf.slice(offset, offset += PREVIOUS_ID_BYTES))
  o.nextId = toHex(buf.slice(offset, offset += NEXT_ID_BYTES))

  // user message
  o.message = normalizeBuffer(buf.slice(offset, offset += MESSAGE_BYTES))

  if ((o.index <= 0) || (o.type !== 5)) { // internal packets are parsed as JSON
    try {
      o.message = JSON.parse(trim(o.message))
    } catch (err) {
    }
  }

  // TODO validate fields
  return o
}

export const applyTax = (buf) => {
  const offset = TYPE_BYTES
  const postage = buf.readInt8(offset)
  buf.writeInt8(postage - 1, offset)
  return buf
}

// const debug = Debug('packet')
export class Packet {
  packetId = null
  postage = 16

  static ttl = CACHE_TTL
  static maxLength = MESSAGE_BYTES

  #buf = null

  constructor ({ type, packetId, previousId, index, nextId, message, topicId }) {
    this.type = type || 0
    this.index = typeof index === 'undefined' ? -1 : index
    this.topicId = topicId || ''
    this.previousId = previousId || ''
    this.packetId = packetId || ''
    this.nextId = nextId || ''
    this.message = message || ''
  }

  static async encode (p) {
    p = { ...p }

    const buf = Buffer.alloc(
      TYPE_BYTES +
      POSTAGE_BYTES +
      INDEX_BYTES +
      MESSAGE_ID_BYTES +
      TOPIC_ID_BYTES +
      PREVIOUS_ID_BYTES +
      NEXT_ID_BYTES +
      MESSAGE_BYTES
    ) // buf length bust be < UDP MTU (usually ~1500)

    const isBuffer = p.message.constructor.name === 'Buffer'

    if (p.index <= 0 && typeof p.message === 'object' && !isBuffer) {
      p.message = JSON.stringify(p.message)
    }

    if (p.index <= 0 && typeof p !== 'string' && !isBuffer) {
      p.message = String(p.message)
    }

    if (p.message.length > Packet.MESSAGE_BYTES) throw new Error('ETOOBIG')

    p.packetId = p.packetId || await sha256(p.previousId + p.message + p.nextId)

    let offset = 0

    // header
    offset = buf.writeInt8(p.type, offset)
    offset = buf.writeInt8(p.postage, offset)
    offset = buf.writeInt32BE(p.index, offset)

    // identifiers
    Buffer.from(p.packetId, 'hex').copy(buf, offset); offset += MESSAGE_ID_BYTES
    Buffer.from(p.topicId, 'hex').copy(buf, offset); offset += TOPIC_ID_BYTES
    Buffer.from(p.previousId, 'hex').copy(buf, offset); offset += PREVIOUS_ID_BYTES
    Buffer.from(p.nextId, 'hex').copy(buf, offset); offset += NEXT_ID_BYTES

    // user message
    Buffer.from(p.message, 'utf8').copy(buf, offset); offset += MESSAGE_BYTES

    // debug('encode', p)
    return buf
  }

  static decode (buf) {
    return decode(buf)
  }
}

export class PacketPing extends Packet {
  static type = 1
  constructor (message) {
    super({ type: 1, message })

    validate(message, {
      peerId: { required: true, type: 'string' },
      pingId: { type: 'string' },
      natType: { type: 'string' },
      topics: { type: 'array' },
      isHeartbeat: { type: 'boolean' },
      data: { type: 'number' },
      isConnection: { type: 'boolean' },
      isReflection: { type: 'boolean' },
      testPort: { type: 'number' }
    })
  }
}

export class PacketPong extends Packet {
  static type = 2
  constructor (message) {
    super({ type: 2, message })

    validate(message, {
      peerId: { required: true, type: 'string' },
      topics: { type: 'array' },
      port: { type: 'number' },
      testPort: { type: 'number' },
      address: { required: true, type: 'string' },
      natType: { type: 'string' },
      isReflection: { type: 'boolean' },
      isConnection: { type: 'boolean' },
      pingId: { type: 'string' }
    })
  }
}

export class PacketIntro extends Packet {
  static type = 3
  constructor (message) {
    super({ type: 3, message })

    validate(message, {
      peerId: { required: true, type: 'string' },
      topics: { required: true, type: 'array' },
      natType: { required: true, type: 'string' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' }
    })
  }
}

export class PacketSubscribe extends Packet {
  static type = 4
  constructor (message) {
    super({ type: 4, message })

    validate(message, {
      peerId: { required: true, type: 'string' },
      topics: { required: true, type: 'array' },
      natType: { required: true, type: 'string' },
      initial: { type: 'boolean' },
      address: { required: true, type: 'string' },
      port: { required: true, type: 'number' }
    })
  }
}

export class PacketPublish extends Packet {
  static type = 5 // no need to validate, message is whatever you want
  constructor ({ message, packetId, topicId, nextId, previousId }) {
    super({ type: 5, message, packetId, topicId, nextId, previousId })
  }
}

export class PacketQuery extends Packet {
  static type = 6
  constructor ({ packetId, topicId, message = {} }) {
    super({ type: 6, packetId, topicId, message })

    validate(message, {
      tail: { type: 'boolean' },
      peerId: { type: 'string' }
    })
  }
}

export class PacketAnswer extends Packet {
  static type = 7
  constructor (args) { super({ type: 7, ...args }) }
}
