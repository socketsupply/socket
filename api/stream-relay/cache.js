import { isBufferLike, toBuffer } from '../util.js'
import { Buffer } from '../buffer.js'

import { Packet, PacketPublish } from './packets.js'

/**
 * Tries to convert input to a `Buffer`, if possible, otherwise `null`.
 * @ignore
 * @param {(string|Buffer|Uint8Array)?} m
 * @return {Buffer?}
 */
function toBufferMaybe (m) {
  return isBufferLike(m) || typeof m === 'string'
    ? toBuffer(m)
    : m && typeof m === 'object'
      ? toBuffer(JSON.stringify(m))
      : null
}

/**
 * Default max size of a `Cache` instance.
 */
export const DEFAULT_MAX_SIZE = Math.ceil((16 * 1024 * 1024) / 1158)

/**
 * @typedef {Packet} CacheEntry
 * @typedef {function(CacheEntry, CacheEntry): number} CacheEntrySiblingResolver
 */

/**
 * Default cache sibling resolver that computes a delta between
 * two entries clocks.
 * @param {CacheEntry} a
 * @param {CacheEntry} b
 * @return {number}
 */
export function defaultSiblingResolver (a, b) {
  return a.clock - b.clock
}

/**
 * Internal mapping of packet IDs to packet data used by `Cache`.
 */
export class CacheData extends Map {}

/**
 * A class for storing a cache of packets by ID.
 */
export class Cache {
  data = new CacheData()
  maxSize = Math.ceil((16 * 1024 * 1024) / 1158)

  /**
   * `Cache` class constructor.
   * @param {CacheData?} [data]
   */
  constructor (
    data = new CacheData(),
    siblingResolver = defaultSiblingResolver
  ) {
    if (data instanceof Map) {
      this.data = data
    } else if (Array.isArray(data)) {
      this.data = new CacheData(data)
    }

    if (typeof siblingResolver === 'function') {
      this.siblingResolver = siblingResolver
    } else {
      this.siblingResolver = defaultSiblingResolver
    }
  }

  /**
   * Readonly count of the number of cache entries.
   * @type {number}
   */
  get size () {
    return this.data.size
  }

  toJSON () {
    return [...this.data.entries()]
  }

  /**
   * Inserts a `CacheEntry` value `v` into the cache at key `k`.
   * @param {string} k
   * @param {CacheEntry} v
   * @return {Promise<boolean>}
   */
  async insert (k, v) {
    if (v.type !== PacketPublish.type) return false
    if (this.has(k)) return true

    if (this.data.size === this.maxSize) {
      const oldest = [...this.data.values()].sort(this.siblingResolver).pop()
      this.data.delete(oldest.packetId)
    }

    this.data.set(k, v)
    return true
  }

  /**
   * Gets a `CacheEntry` value at key `k`.
   * @param {string} k
   * @return {Promise<CacheEntry?>}
   */
  async get (k) {
    return this.data.get(k)
  }

  /**
   * @param {string} k
   * @return {Promise<boolean>}
   */
  async delete (k) {
    if (!this.has(k)) return false
    this.data.delete(k)
    return true
  }

  /**
   * Predicate to determine if cache contains an entry at key `k`.
   * @param {string} k
   * @return {boolean}
   */
  has (k) {
    return this.data.has(k)
  }

  /**
   * Composes an indexed packet into a new `Packet`
   * @param {Packet} packet
   */
  async compose (packet) {
    if (packet.previousId && packet?.index > 0) packet = await this.get(packet.previousId)
    if (!packet) return null

    const { meta, size, indexes } = packet.message
    const { packetId } = packet

    // follow the chain to get the buffers in order
    const bufs = this
      .map(p => p, { packetId, inclusive: true })
      .sort((a, b) => a.index - b.index)

    if (!indexes || bufs.length < indexes) {
      return null
    }

    // sort by index, concat and then hash, the original should match
    const messages = bufs.map(p => p.message)
    const buffers = messages.map(toBufferMaybe).filter(Boolean)
    const message = Buffer.concat(buffers, size)

    return Packet.from({
      ...packet,
      message,
      isComposed: true,
      index: -1,
      meta
    })
  }

  /**
   * Visits each head entry in the cache calling function `fn` on each. Heads
   * are entries that contain a previous packet ID that can be found in the
   * cache.
   * @param {function(CacheEntry, number): any} fn
   */
  heads (fn) {
    this.each(fn, { onlyHeads: true })
  }

  /**
   * Visits each tail entry in the cache calling function `fn` on each. Tails
   * are entries that contain a previous packet ID that is not found in the cache
   * @param {function(CacheEntry, number): any} fn
   */
  tails (fn) {
    let index = 0
    this.data.forEach((packet) => {
      if (packet.previousId && !this.has(packet.previousId)) fn(packet, index++)
    })
  }

  /**
   * Visits each entry in the cache calling function `fn` on each visited entry.
   * @param {function(CacheEntry, number): any} fn
   * @param {{ onlyHeads?: boolean?, packetId?: string, inclusive?: boolean }} [options]
   */
  each (fn, options = {}) {
    if (typeof fn !== 'function') {
      throw new TypeError('Expecting function')
    }

    const { onlyHeads, packetId = '', inclusive = false } = options || {}
    const packets = [...this.data.values()]
    const tails = packets.filter(packetId ? p => p.packetId === packetId : p => !p.previousId)
    let index = 0

    const next = (/** @type {CacheEntry[]} */ tails) => {
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

  /**
   * Visits each entry in the cache calling function `fn` on each visited entry
   * returning mapped results into a new array.
   * @param {function(CacheEntry, number): any} fn
   * @param {{ onlyHeads?: boolean?, packetId?: string, inclusive?: boolean }} [options]
   */
  map (fn, options) {
    const mapped = []
    this.each((...args) => mapped.push(fn(...args)), options)
    return mapped
  }
}

export default Cache
