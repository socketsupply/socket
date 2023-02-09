import { DirectoryHandle } from './handle.js'
import { Buffer } from '../buffer.js'
import {
  UV_DIRENT_UNKNOWN,
  UV_DIRENT_FILE,
  UV_DIRENT_DIR,
  UV_DIRENT_LINK,
  UV_DIRENT_FIFO,
  UV_DIRENT_SOCKET,
  UV_DIRENT_CHAR,
  UV_DIRENT_BLOCK
} from './constants.js'

import * as exports from './dir.js'

export const kType = Symbol.for('fs.Dirent.type')

function checkDirentType (dirent, property) {
  return dirent.type === property
}

/**
 * Sorts directory entries
 * @param {string|Dirent} a
 * @param {string|Dirent} b
 * @return {number}
 */
export function sortDirectoryEntries (a, b) {
  if (a instanceof Dirent) {
    return sortDirectoryEntries(a.name, b.name)
  }

  return a < b ? -1 : a > b ? 1 : 0
}

/**
 * A containerr for a directory and its entries. This class supports scanning
 * a directory entry by entry with a `read()` method. The `Symbol.asyncIterator`
 * interface is exposed along with an AsyncGenerator `entries()` method.
 * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-fsdir}
 */
export class Dir {
  static from (fdOrHandle, options) {
    if (fdOrHandle instanceof DirectoryHandle) {
      return new this(fdOrHandle, options)
    }

    return new this(DirectoryHandle.from(fdOrHandle, options), options)
  }

  /**
   * `Dir` class constructor.
   * @param {DirectoryHandle} handle
   * @param {object=} options
   */
  constructor (handle, options) {
    this.path = handle?.path ?? null
    this.handle = handle
    this.encoding = options?.encoding || 'utf8'
    this.withFileTypes = options?.withFileTypes !== false
  }

  /**
   * Closes container and underlying handle.
   * @param {object|function} options
   * @param {function=} callback
   */
  async close (options, callback) {
    if (typeof options === 'function') {
      callback = options
      options = {}
    }

    if (typeof callback === 'function') {
      try {
        await this.handle?.close(options)
      } catch (err) {
        return callback(err)
      }

      return callback(null)
    }

    return await this.handle?.close(options)
  }

  /**
   * Reads and returns directory entry.
   * @param {object|function} options
   * @param {function=} callback
   * @return {Dirent|string}
   */
  async read (options, callback) {
    if (typeof options === 'function') {
      callback = options
      options = {}
    }

    let results = []

    try {
      results = await this.handle?.read(options)
    } catch (err) {
      if (typeof callback === 'function') {
        callback(err)
        return
      }

      throw err
    }

    results = results.map((result) => {
      if (this.withFileTypes) {
        result = Dirent.from(result)

        if (this.encoding === 'buffer') {
          result.name = Buffer.from(result.name)
        } else {
          result.name = Buffer.from(result.name).toString(this.encoding)
        }
      } else {
        result = result.name

        if (this.encoding === 'buffer') {
          result = Buffer.from(result)
        } else {
          result = Buffer.from(result).toString(this.encoding)
        }
      }

      return result
    })

    if (results.length <= 1) {
      results = results[0] || null
    }

    if (typeof callback === 'function') {
      callback(null, results)
      return
    }

    return results
  }

  /**
   * AsyncGenerator which yields directory entries.
   * @param {object=} options
   */
  async * entries (options) {
    try {
      while (true) {
        const results = await this.read(options)

        if (results === null) {
          break
        }

        if (Array.isArray(results)) {
          for (let i = 0; i < results.length; ++i) {
            yield results[i]
          }
        } else {
          yield results
        }
      }
    } finally {
      await this.close()
    }
  }

  /**
   * `for await (...)` AsyncGenerator support.
   */
  get [Symbol.asyncIterator] () {
    return this.entries
  }
}

/**
 * A container for a directory entry.
 * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-fsdirent}
 */
export class Dirent {
  static get UNKNOWN () { return UV_DIRENT_UNKNOWN }
  static get FILE () { return UV_DIRENT_FILE }
  static get DIR () { return UV_DIRENT_DIR }
  static get LINK () { return UV_DIRENT_LINK }
  static get FIFO () { return UV_DIRENT_FIFO }
  static get SOCKET () { return UV_DIRENT_SOCKET }
  static get CHAR () { return UV_DIRENT_CHAR }
  static get BLOCK () { return UV_DIRENT_BLOCK }

  /**
   * Creates `Dirent` instance from input.
   * @param {object|string} name
   * @param {(string|number)=} type
   */
  static from (name, type) {
    if (typeof name === 'object') {
      return new this(name?.name, name?.type)
    }

    return new this(name, type ?? Dirent.UNKNOWN)
  }

  /**
   * `Dirent` class constructor.
   * @param {string} name
   * @param {string|number} type
   */
  constructor (name, type) {
    this.name = name ?? null
    this[kType] = parseInt(type ?? Dirent.UNKNOWN)
  }

  /**
   * Read only type.
   */
  get type () {
    return this[kType]
  }

  /**
   * `true` if `Dirent` instance is a directory.
   */
  isDirectory () {
    return checkDirentType(this, Dirent.DIR)
  }

  /**
   * `true` if `Dirent` instance is a file.
   */
  isFile () {
    return checkDirentType(this, Dirent.FILE)
  }

  /**
   * `true` if `Dirent` instance is a block device.
   */
  isBlockDevice () {
    return checkDirentType(this, Dirent.BLOCK)
  }

  /**
   * `true` if `Dirent` instance is a character device.
   */
  isCharacterDevice () {
    return checkDirentType(this, Dirent.CHAR)
  }

  /**
   * `true` if `Dirent` instance is a symbolic link.
   */
  isSymbolicLink () {
    return checkDirentType(this, Dirent.LINK)
  }

  /**
   * `true` if `Dirent` instance is a FIFO.
   */
  isFIFO () {
    return checkDirentType(this, Dirent.FIFO)
  }

  /**
   * `true` if `Dirent` instance is a socket.
   */
  isSocket () {
    return checkDirentType(this, Dirent.SOCKET)
  }
}

export default exports
