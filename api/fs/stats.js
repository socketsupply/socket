import constants from './constants.js'
import os from '../os.js'

import * as exports from './stats.js'

const isWindows = /win/i.test(os.type())
const isAndroid = /android/i.test(os.type())

/**
 * @ignore
 * @param {number|bigint} mode
 * @param {number|bigint} property
 * @return {boolean}
 */
function checkMode (mode, property) {
  if (isWindows) {
    if (
      property === constants.S_IFIFO ||
      property === constants.S_IFBLK ||
      property === constants.S_IFSOCK
    ) {
      return false
    }
  }

  if (typeof mode === 'bigint') {
    return (mode & BigInt(constants.S_IFMT)) === BigInt(property)
  }

  return (mode & constants.S_IFMT) === property
}

/**
 * A container for various stats about a file or directory.
 */
export class Stats {
  /**
   * Creates a `Stats` instance from input, optionally with `BigInt` data types
   * @param {object|Stats} [stat]
   * @param {fromBigInt=} [fromBigInt = false]
   * @return {Stats}
   */
  static from (stat, fromBigInt = false) {
    if (fromBigInt) {
      return new this({
        dev: BigInt(stat.st_dev ?? 0n),
        ino: BigInt(stat.st_ino ?? 0n),
        mode: BigInt(stat.st_mode ?? 0n),
        nlink: BigInt(stat.st_nlink ?? 0n),
        uid: BigInt(stat.st_uid ?? 0n),
        gid: BigInt(stat.st_gid ?? 0n),
        rdev: BigInt(stat.st_rdev ?? 0n),
        size: BigInt(stat.st_size ?? 0n),
        blksize: BigInt(stat.st_blksize ?? 0n),
        blocks: BigInt(stat.st_blocks ?? 0n),
        atimeMs: BigInt(stat.st_atim?.tv_sec ?? 0n) * 1000n + BigInt(stat.st_atim?.tv_nsec ?? 0n) / 1000_000n,
        mtimeMs: BigInt(stat.st_mtim?.tv_sec ?? 0n) * 1000n + BigInt(stat.st_mtim?.tv_nsec ?? 0n) / 1000_000n,
        ctimeMs: BigInt(stat.st_ctim?.tv_sec ?? 0n) * 1000n + BigInt(stat.st_ctim?.tv_nsec ?? 0n) / 1000_000n,
        atimNs: BigInt(stat.st_atim?.tv_sec ?? 0n) * 1000_000_000n + BigInt(stat.st_atim?.tv_nsec ?? 0n),
        mtimNs: BigInt(stat.st_mtim?.tv_sec ?? 0n) * 1000_000_000n + BigInt(stat.st_mtim?.tv_nsec ?? 0n),
        ctimNs: BigInt(stat.st_ctim?.tv_sec ?? 0n) * 1000_000_000n + BigInt(stat.st_ctim?.tv_nsec ?? 0n),

        birthtimeMs: BigInt(stat.st_birthtim?.tv_sec ?? 0n) * 1000n + BigInt(stat.st_birthtim?.tv_nsec ?? 0n) / 1000_000n,
        birthtimeNs: BigInt(stat.st_birthtim?.tv_sec ?? 0n) * 1000_000_000n + BigInt(stat.st_birthtim?.tv_nsec ?? 0n)
      })
    }

    return new this({
      dev: Number(stat.st_dev ?? 0),
      ino: Number(stat.st_ino ?? 0),
      mode: Number(stat.st_mode ?? 0),
      nlink: Number(stat.st_nlink ?? 0),
      uid: Number(stat.st_uid ?? 0),
      gid: Number(stat.st_gid ?? 0),
      rdev: Number(stat.st_rdev ?? 0),
      size: Number(stat.st_size ?? 0),
      blksize: Number(stat.st_blksize ?? 0),
      blocks: Number(stat.st_blocks ?? 0),
      atimeMs: Number(stat.st_atim?.tv_sec ?? 0) * 1000 + Number(stat.st_atim?.tv_nsec ?? 0) / 1000_000,
      mtimeMs: Number(stat.st_mtim?.tv_sec ?? 0) * 1000 + Number(stat.st_mtim?.tv_nsec ?? 0) / 1000_000,
      ctimeMs: Number(stat.st_ctim?.tv_sec ?? 0) * 1000 + Number(stat.st_ctim?.tv_nsec ?? 0) / 1000_000,
      birthtimeMs: Number(stat.st_birthtim?.tv_sec ?? 0) * 1000 + Number(stat.st_birthtim?.tv_nsec ?? 0) / 1000_000
    })
  }

  /**
   * `Stats` class constructor.
   * @param {object|Stats} stat
   */
  constructor (stat) {
    this.dev = stat.dev
    this.ino = stat.ino
    this.mode = stat.mode
    this.nlink = stat.nlink
    this.uid = stat.uid
    this.gid = stat.gid
    this.rdev = stat.rdev
    this.size = stat.size
    this.blksize = stat.blksize
    this.blocks = stat.blocks
    this.atimeMs = stat.atimeMs
    this.mtimeMs = stat.mtimeMs
    this.ctimeMs = stat.ctimeMs
    this.birthtimeMs = stat.birthtimeMs

    this.atime = new Date(this.atimeMs)
    this.mtime = new Date(this.mtimeMs)
    this.ctime = new Date(this.ctimeMs)
    this.birthtime = new Date(this.birthtimeMs)

    // hack to make stats assume file is regular file because it likely
    // came from the content resolver
    if (isAndroid && !this.mode) {
      this.mode = constants.S_IFREG | constants.S_IRUSR
    }

    Object.defineProperty(this, 'handle', {
      configurable: true,
      enumerable: false,
      writable: true,
      value: null
    })
  }

  /**
   * Returns `true` if stats represents a directory.
   * @return {Boolean}
   */
  isDirectory () {
    return checkMode(this.mode, constants.S_IFDIR)
  }

  /**
   * Returns `true` if stats represents a file.
   * @return {Boolean}
   */
  isFile () {
    return checkMode(this.mode, constants.S_IFREG)
  }

  /**
   * Returns `true` if stats represents a block device.
   * @return {Boolean}
   */
  isBlockDevice () {
    return checkMode(this.mode, constants.S_IFBLK)
  }

  /**
   * Returns `true` if stats represents a character device.
   * @return {Boolean}
   */
  isCharacterDevice () {
    return checkMode(this.mode, constants.S_IFCHR)
  }

  /**
   * Returns `true` if stats represents a symbolic link.
   * @return {Boolean}
   */
  isSymbolicLink () {
    return checkMode(this.mode, constants.S_IFLNK)
  }

  /**
   * Returns `true` if stats represents a FIFO.
   * @return {Boolean}
   */
  isFIFO () {
    return checkMode(this.mode, constants.S_IFIFO)
  }

  /**
   * Returns `true` if stats represents a socket.
   * @return {Boolean}
   */
  isSocket () {
    return checkMode(this.mode, constants.S_IFSOCK)
  }
}

export default exports
