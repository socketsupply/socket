import { isBufferLike, toBuffer } from '../util.js'
import { Buffer } from '../buffer.js'

import { PacketPublish } from './packets.js'

export class Cache {
  data = new Map()
  maxSize = Math.ceil((16 * 1024 * 1024) / 1158)

  constructor (data, siblingResolver) {
    if (data) this.data = data
    if (siblingResolver) this.siblingResolver = siblingResolver
    if (!siblingResolver) siblingResolver = (a, b) => a.clock - b.clock
  }

  get size () {
    return this.data.size
  }

  async insert (k, v) {
    if (v.type !== PacketPublish.type) return false
    if (this.data.has(k)) return true

    if (this.data.size === this.maxSize) {
      const oldest = [...this.data.values()].sort(this.siblingResolver).pop()
      this.data.delete(oldest.packetId)
    }

    this.data.set(k, v)
    return true
  }

  async get (k) {
    return this.data.get(k)
  }

  delete (k) {
    this.data.delete(k)
  }

  has (k) {
    return this.data.has(k)
  }

  async compose (packet, peer) {
    if (packet.index > 0) packet = this.data.get(packet.previousId)
    if (!packet) return null

    let bufs = [] // follow the chain to get the buffers in order
    this.each(p => bufs.push(p), { packetId: packet.packetId, inclusive: true })

    if (!packet.message.indexes || bufs.length < packet.message.indexes) {
      return null
    }

    // sort by index, concat and then hash, the original should match
    bufs.sort((a, b) => a.index - b.index)

    bufs = bufs
      .map(p => p.message)
      .map(m => isBufferLike(m) || typeof m === 'string'
        ? toBuffer(m)
        : m && typeof m === 'object' ? toBuffer(JSON.stringify(m)) : null
      )
      .filter(Boolean)

    const message = Buffer.concat(bufs, packet.message.size)

    return { ...packet, message, index: -1, isComposed: true, meta: packet.message.meta }
  }

  heads (fn) {
    this.each(fn, { onlyHeads: true })
  }

  tails (fn) {
    let index = 0
    this.data.forEach((packet, packetId) => {
      if (packet.previousId && !this.data.has(packet.previousId)) fn(packet, index++)
    })
  }

  each (fn, { onlyHeads, packetId = '', inclusive = false } = {}) {
    const packets = [...this.data.values()]
    const tails = packets.filter(packetId ? p => p.packetId === packetId : p => !p.previousId)
    let index = 0

    const next = async tails => {
      for (let tail of tails) {
        const heads = packets.filter(p => p.previousId === tail.packetId)
        const siblings = heads.length > 1 && heads.every(p => p.previousId === tail.packetId)
        if (siblings) tail = heads.pop()

        next(heads.sort(this.siblingResolver))

        if (inclusive || tail.packetId !== packetId) {
          onlyHeads ? ((heads.length === 0) && fn(tail, index)) : fn(tail, index)
        }

        index++
      }
    }

    next(tails.sort(this.siblingResolver))
  }
}

export default Cache
