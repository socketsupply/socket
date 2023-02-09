import constants from './constants.js'
import os from '../os.js'

import * as exports from './stats.js'

const isWindows = /win/i.test(os.type())

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
 * @TODO
 */
export class Stats {
  /**
   * @TODO
   */
  static from (stat, fromBigInt) {
    if (fromBigInt) {
      return new this({
        dev: BigInt(stat.st_dev),
        ino: BigInt(stat.st_ino),
        mode: BigInt(stat.st_mode),
        nlink: BigInt(stat.st_nlink),
        uid: BigInt(stat.st_uid),
        gid: BigInt(stat.st_gid),
        rdev: BigInt(stat.st_rdev),
        size: BigInt(stat.st_size),
        blksize: BigInt(stat.st_blksize),
        blocks: BigInt(stat.st_blocks),
        atimeMs: BigInt(stat.st_atim.tv_sec) * 1000n + BigInt(stat.st_atim.tv_nsec) / 1000_000n,
        mtimeMs: BigInt(stat.st_mtim.tv_sec) * 1000n + BigInt(stat.st_mtim.tv_nsec) / 1000_000n,
        ctimeMs: BigInt(stat.st_ctim.tv_sec) * 1000n + BigInt(stat.st_ctim.tv_nsec) / 1000_000n,
        birthtimeMs: BigInt(stat.st_birthtim.tv_sec) * 1000n + BigInt(stat.st_birthtim.tv_nsec) / 1000_000n,
        atimNs: BigInt(stat.st_atim.tv_sec) * 1000_000_000n + BigInt(stat.st_atim.tv_nsec),
        mtimNs: BigInt(stat.st_mtim.tv_sec) * 1000_000_000n + BigInt(stat.st_mtim.tv_nsec),
        ctimNs: BigInt(stat.st_ctim.tv_sec) * 1000_000_000n + BigInt(stat.st_ctim.tv_nsec),
        birthtimNs: BigInt(stat.st_birthtim.tv_sec) * 1000_000_000n + BigInt(stat.st_birthtim.tv_nsec)
      })
    }

    return new this({
      dev: Number(stat.st_dev),
      ino: Number(stat.st_ino),
      mode: Number(stat.st_mode),
      nlink: Number(stat.st_nlink),
      uid: Number(stat.st_uid),
      gid: Number(stat.st_gid),
      rdev: Number(stat.st_rdev),
      size: Number(stat.st_size),
      blksize: Number(stat.st_blksize),
      blocks: Number(stat.st_blocks),
      atimeMs: Number(stat.st_atim.tv_sec) * 1000 + Number(stat.st_atim.tv_nsec) / 1000_000,
      mtimeMs: Number(stat.st_mtim.tv_sec) * 1000 + Number(stat.st_mtim.tv_nsec) / 1000_000,
      ctimeMs: Number(stat.st_ctim.tv_sec) * 1000 + Number(stat.st_ctim.tv_nsec) / 1000_000,
      birthtimeMs: Number(stat.st_birthtim.tv_sec) * 1000 + Number(stat.st_birthtim.tv_nsec) / 1000_000
    })
  }

  /**
   * `Stats` class constructor.
   * @param {object} stat
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

    Object.defineProperty(this, 'handle', {
      configurable: true,
      enumerable: false,
      writable: true,
      value: null
    })
  }

  isDirectory () {
    return checkMode(this.mode, constants.S_IFDIR)
  }

  isFile () {
    return checkMode(this.mode, constants.S_IFREG)
  }

  isBlockDevice () {
    return checkMode(this.mode, constants.S_IFBLK)
  }

  isCharacterDevice () {
    return checkMode(this.mode, constants.S_IFCHR)
  }

  isSymbolicLink () {
    return checkMode(this.mode, constants.S_IFLNK)
  }

  isFIFO () {
    return checkMode(this.mode, constants.S_IFIFO)
  }

  isSocket () {
    return checkMode(this.mode, constants.S_IFSOCK)
  }
}

export default exports
