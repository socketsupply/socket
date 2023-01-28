import { Buffer } from 'socket:buffer'
import { PacketPublish } from './packets.js'

class Cache {
  data = new Map()
  maxSize = Math.ceil((16 * 1024 * 1024) / 1158)

  constructor (data) {
    if (data) this.data = data
  }

  get size () {
    return this.data.size
  }

  insert (k, v) {
    if (v.type !== PacketPublish.type || this.data.has(k)) return false
    if (this.data.size === this.maxSize) {
      const byTimestamp = (a, b) => a.timestamp - b.timestamp
      const oldest = [...this.data.values()].sort(byTimestamp).pop()
      this.data.delete(oldest.packetId)
    }
    this.data.set(k, v)
    return true
  }

  delete (k) {
    this.data.delete(k)
  }

  has (k) {
    return this.data.has(k)
  }

  get (k) {
    return this.data.get(k)
  }

  async compose (packet, sha256) {
    if (packet.index > 0) packet = this.data.get(packet.previousId)
    if (!packet) return null

    let bufs = [] // follow the chain to get the buffers in order
    this.each(p => bufs.push(p), { from: p => p.packetId === packet.packetId })

    if (!packet.message.indexes || bufs.length < packet.message.indexes) return null

    // sort by index, concat and then hash, the original should match
    bufs.sort((a, b) => a.index - b.index)

    bufs = bufs.map(c => Buffer.from(c.message.toString()))
    const message = Buffer.concat(bufs, packet.message.size)

    if (await sha256(message) !== packet.message.hash) return null

    return { ...packet, message, isComposed: true, meta: packet.message.meta }
  }

  heads (fn) {
    this.each(fn, { onlyHeads: true })
  }

  tails (fn) {
    let index = 0
    this.data.forEach((packet, packetId) => {
      index++
      if (packet.previousId && this.data.has(packet.previousId)) return
      fn(packet, index)
    })
  }

  lookback (fn, parent) {
    while (parent) {
      parent = this.data.get(parent.previousId)
      if (parent) fn(parent)
    }
  }

  each (fn, { onlyHeads, from = p => !p.previousId } = {}) {
    const sort = (a, b) => a.timestamp + b.timestamp
    const packets = [...this.data.values()]
    const tails = packets.filter(from)
    let index = 0

    const next = tails => {
      for (let tail of tails) {
        const heads = packets.filter(p => p.previousId === tail.packetId)
        const siblings = heads.length > 1 && heads.every(p => p.previousId === tail.packetId)
        if (siblings) tail = heads.pop()

        next(heads.sort(sort))
        onlyHeads ? ((heads.length === 0) && fn(tail, index)) : fn(tail, index)
        index++
      }
    }

    next(tails.sort(sort))
  }
}

export { Cache }
