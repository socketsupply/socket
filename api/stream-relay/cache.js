import { isBufferLike, toBuffer } from '../util.js'
import { Buffer } from '../buffer.js'
import { createDigest } from '../crypto.js'
import { Packet, PacketPublish } from './packets.js'

export const trim = (/** @type {Buffer} */ buf) => {
  return buf.toString().split('~')[0].split('\x00')[0]
}

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

  static HASH_SIZE_BYTES = 20

  /**
   * `Cache` class constructor.
   * @param {CacheData?} [data]
   */
  constructor (data = new CacheData(), siblingResolver = defaultSiblingResolver) {
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

    v.timestamp = Date.now()

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

    // follow the chain to get the buffers in order
    const bufs = [...this.data.values()]
      .filter(p => p.previousId === packet.packetId)
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

  async sha1 (value, toHex) {
    const buf = await createDigest('SHA-1', isBufferLike(value) ? value : Buffer.from(value))
    if (!toHex) return buf
    return buf.toString('hex')
  }

  /**
   * summarize returns a terse yet comparable summary of the cache contents.
   *
   * thinking of the cache as a trie of hex characters, the summary returns
   * a checksum for the current level of the trie and for its 16 children.
   *
   * this is similar to a merkel tree as equal subtrees can easily be detected
   * without the need for further recursion. When the subtree checksums are
   * inequivalent then further negotiation at lower levels may be required, this
   * process continues until the two trees become synchonized.
   *
   * when the prefix is empty, the summary will return an array of 16 checksums
   * these checksums provide a way of comparing that subtree with other peers.
   *
   * when a variable-length hexidecimal prefix is provided, then only cache
   * member hashes sharing this prefix will be considered.
   *
   * for each hex character provided in the prefix, the trie will decend by one
   * level, each level divides the 2^128 address space by 16.
   *
   * example:
   *
   * level  0   1   2
   * --------------------------
   * 2b00
   * aa0e  ━┓  ━┓
   * aa1b   ┃   ┃
   * aae3   ┃   ┃  ━┓
   * aaea   ┃   ┃   ┃
   * aaeb   ┃  ━┛  ━┛
   * ab00   ┃  ━┓
   * ab1e   ┃   ┃
   * ab2a   ┃   ┃
   * abef   ┃   ┃
   * abf0  ━┛  ━┛
   * bff9
   *
   * @param {string} prefix - a string of lowercased hexidecimal characters
   * @return {Object}
   */
  async summarize (prefix = '') {
    // each level has 16 children (0x0-0xf)
    const children = new Array(16).fill(null).map(_ => [])

    // partition the cache into children
    for (const key of this.data.keys()) {
      if (prefix.length && !key.startsWith(prefix)) continue
      const hex = key.slice(prefix.length, prefix.length + 1)
      children[parseInt(hex, 16)].push(key)
    }

    // compute a checksum for all child members (deterministically)
    // if the bucket is empty, return null
    const buckets = await Promise.all(children.map(child => {
      return child.length ? this.sha1(child.sort().join(''), true) : Promise.resolve(null)
    }))

    // compute a summary hash (checksum of all other hashes)
    const hash = await this.sha1(buckets.join(''), true)

    return { prefix, hash, buckets }
  }

  /**
   * encodeSummary provide a compact binary encoding of the output of summary()
   *
   * @param {Object} summary - the output of calling summary()
   * @return {Buffer}
  **/
  static encodeSummary (summary) {
    // prefix is variable-length hex string encoded with the first byte indicating the length
    const prefixBin = Buffer.alloc(1 + Math.ceil(summary.prefix.length / 2))
    prefixBin.writeUInt8(summary.prefix.length, 0)
    const prefixHex = summary.prefix.length % 2 ? '0' + summary.prefix : summary.prefix
    Buffer.from(prefixHex, 'hex').copy(prefixBin, 1)

    // hash is the summary hash (checksum of all other hashes)
    const hashBin = Buffer.from(summary.hash, 'hex')

    // buckets are rows of { offset, sum } where the sum is not null
    const bucketBin = Buffer.concat(summary.buckets.map((sum, offset) => {
      // empty buckets are omitted from the encoding
      if (!sum) return Buffer.alloc(0)

      // write the child offset as a uint8
      const offsetBin = Buffer.alloc(1)
      offsetBin.writeUInt8(offset, 0)

      return Buffer.concat([offsetBin, Buffer.from(sum, 'hex')])
    }))

    return Buffer.concat([prefixBin, hashBin, bucketBin])
  }

  /**
   * decodeSummary decodes the output of encodeSummary()
   *
   * @param {Buffer} bin - the output of calling encodeSummary()
   * @return {Object} summary
  **/
  static decodeSummary (bin) {
    let o = 0 // byte offset

    // prefix is variable-length hex string encoded with the first byte indicating the length
    const plen = bin.readUint8(o++)
    const prefix = bin.slice(o, o += Math.ceil(plen / 2)).toString('hex').slice(-plen)

    // hash is the summary hash (checksum of all other hashes)
    const hash = bin.slice(o, o += Cache.HASH_SIZE_BYTES).toString('hex')

    // buckets are rows of { offset, sum } where the sum is not null
    const buckets = new Array(16).fill(null)
    while (o < bin.length) {
      buckets[bin.readUint8(o++)] = bin.slice(o, o += Cache.HASH_SIZE_BYTES).toString('hex')
    }

    return { prefix, hash, buckets }
  }
}

export default Cache
