import {
  InvertedPromise,
  isBufferLike,
  isTypedArray,
  isEmptyObject,
  splitBuffer,
  clamp
} from '../util.js'

import { rand64 } from '../crypto.js'

import { ReadStream, WriteStream } from './stream.js'
import { normalizeFlags } from './flags.js'
import { EventEmitter } from '../events.js'
import { AbortError } from '../errors.js'
import diagnostics from '../diagnostics.js'
import { Buffer } from '../buffer.js'
import { Stats } from './stats.js'
import { F_OK } from './constants.js'
import console from '../console.js'
import fds from './fds.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

import * as exports from './handle.js'

const dc = diagnostics.channels.group('fs', [
  'handle',
  'handle.open',
  'handle.read',
  'handle.write',
  'handle.close'
])

export const kOpening = Symbol.for('fs.FileHandle.opening')
export const kClosing = Symbol.for('fs.FileHandle.closing')
export const kClosed = Symbol.for('fs.FileHandle.closed')

/**
 * A container for a descriptor tracked in `fds` and opened in the native layer.
 * This class implements the Node.js `FileHandle` interface
 * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#class-filehandle}
 */
export class FileHandle extends EventEmitter {
  static get DEFAULT_ACCESS_MODE () { return F_OK }
  static get DEFAULT_OPEN_FLAGS () { return 'r' }
  static get DEFAULT_OPEN_MODE () { return 0o666 }

  /**
   * Creates a `FileHandle` from a given `id` or `fd`
   * @param {string|number|FileHandle|object} id
   * @return {FileHandle}
   */
  static from (id) {
    if (id?.id) {
      return this.from(id.id)
    } else if (id?.fd) {
      return this.from(id.fd)
    }

    let fd = fds.get(id)

    // `id` could actually be an `fd`
    if (!fd) {
      id = fds.id(id)
      fd = fds.get(id)
    }

    if (!fd || !id) {
      throw new Error('Invalid file descriptor.')
    }

    return new this({ fd, id })
  }

  // TODO(trevnorris): The way the comment says to use mode doesn't match
  // how it's currently being used in tests. Instead we're passing values
  // from fs.constants.
  /**
   * Determines if access to `path` for `mode` is possible.
   * @param {string} path
   * @param {number} [mode = 0o666]
   * @param {object=} [options]
   * @return {boolean}
   */
  static async access (path, mode, options) {
    if (mode !== null && typeof mode === 'object') {
      options = mode
      mode = undefined
    }

    if (mode === undefined) {
      mode = FileHandle.DEFAULT_ACCESS_MODE
    }

    const result = await ipc.request('fs.access', { mode, path }, options)

    if (result.err) {
      throw result.err
    }

    // F_OK means access in any way
    return mode === F_OK ? true : (result.data?.mode && mode) > 0
  }

  /**
   * Asynchronously open a file.
   * @see {https://nodejs.org/dist/latest-v16.x/docs/api/fs.html#fspromisesopenpath-flags-mode}
   * @param {string | Buffer | URL} path
   * @param {string=} [flags = 'r']
   * @param {string=} [mode = 0o666]
   * @param {object=} [options]
   */
  static async open (path, flags, mode, options) {
    if (flags === undefined) {
      flags = FileHandle.DEFAULT_OPEN_FLAGS
    }

    if (mode === undefined) {
      mode = FileHandle.DEFAULT_OPEN_MODE
    }

    const handle = new this({ path, flags, mode })

    if (typeof handle.path !== 'string') {
      throw new TypeError('Expecting path to be a string, Buffer, or URL.')
    }

    await handle.open(options)

    return handle
  }

  /**
   * `FileHandle` class constructor
   * @private
   * @param {object} options
   */
  constructor (options) {
    super()

    // String | Buffer | URL | { toString(): String }
    if (options?.path && typeof options.path.toString === 'function') {
      options.path = options.path.toString()
    }

    this[kOpening] = null
    this[kClosing] = null
    this[kClosed] = false

    this.flags = normalizeFlags(options?.flags)
    this.path = options?.path || null
    this.mode = options?.mode || FileHandle.DEFAULT_OPEN_MODE

    // this id will be used to identify the file handle that is a
    // reference stored in the native side
    this.id = String(options?.id || rand64())
    this.fd = options?.fd || null // internal file descriptor

    gc.ref(this, options)
    dc.channel('handle').publish({ handle: this })
  }

  /**
   * `true` if the `FileHandle` instance has been opened.
   * @type {boolean}
   */
  get opened () {
    return this.fd !== null && this.fd === fds.get(this.id)
  }

  /**
   * `true` if the `FileHandle` is opening.
   * @type {boolean}
   */
  get opening () {
    const opening = this[kOpening]
    return opening?.value !== true
  }

  /**
   * `true` if the `FileHandle` is closing.
   * @type {boolean}
   */
  get closing () {
    const closing = this[kClosing]
    return Boolean(closing && closing?.value !== true)
  }

  /**
   * `true` if the `FileHandle` is closed.
   */
  get closed () {
    return this[kClosed]
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, options],
      async handle (id) {
        if (fds.has(id)) {
          console.warn('Closing FileHandle on garbage collection')
          await ipc.send('fs.close', { id }, options)
          fds.release(id, false)
        }
      }
    }
  }

  /**
   * Appends to a file, if handle was opened with `O_APPEND`, otherwise this
   * method is just an alias to `FileHandle#writeFile()`.
   * @param {string|Buffer|TypedArray|Array} data
   * @param {object=} [options]
   * @param {string=} [options.encoding = 'utf8']
   * @param {object=} [options.signal]
   */
  async appendFile (data, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    return await this.writeFile(data, options)
  }

  /**
   * Change permissions of file handle.
   * @param {number} mode
   * @param {object=} [options]
   */
  async chmod (mode, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * Change ownership of file handle.
   * @param {number} uid
   * @param {number} gid
   * @param {object=} [options]
   */
  async chown (uid, gid, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * Close underlying file handle
   * @param {object=} [options]
   */
  async close (options) {
    // wait for opening to finish before proceeding to close
    if (this[kOpening]) {
      await this[kOpening]
    }

    if (this[kClosing]) {
      return await this[kClosing]
    }

    if (!fds.get(this.id)) {
      throw new Error('FileHandle is not opened')
    }

    this[kClosing] = new InvertedPromise()

    const result = await ipc.send('fs.close', { id: this.id }, options)

    if (result.err) {
      return this[kClosing].reject(result.err)
    }

    fds.release(this.id, false)
    gc.unref(this)

    this.fd = null

    this[kClosing].resolve(true)

    this[kOpening] = null
    this[kClosing] = null
    this[kClosed] = true

    this.emit('close')

    dc.channel('handle.close').publish({ handle: this })

    return true
  }

  /**
   * Creates a `ReadStream` for the underlying file.
   * @param {object=} [options] - An options object
   */
  createReadStream (options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const stream = new ReadStream({
      autoClose: options?.autoClose === true,
      ...options,
      handle: this
    })

    stream.once('end', async () => {
      if (options?.autoClose === true) {
        try {
          await this.close()
        } catch (err) {
          stream.emit('error', err)
        }
      }
    })

    return stream
  }

  /**
   * Creates a `WriteStream` for the underlying file.
   * @param {object=} [options] - An options object
   */
  createWriteStream (options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const stream = new WriteStream({
      autoClose: options?.autoClose === true,
      ...options,
      handle: this
    })

    stream.once('finish', async () => {
      if (options?.autoClose === true) {
        try {
          await this.close()
        } catch (err) {
          stream.emit('error', err)
        }
      }
    })

    return stream
  }

  /**
   * @param {object=} [options]
   */
  async datasync () {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * Opens the underlying descriptor for the file handle.
   * @param {object=} [options]
   */
  async open (options) {
    if (this.closing) {
      throw new Error('FileHandle is closing')
    }

    if (this.closed) {
      throw new Error('FileHandle is closed')
    }

    if (this.opened) {
      return true
    }

    if (this[kOpening]) {
      return await this[kOpening]
    }

    const { flags, mode, path, id } = this

    if (options?.signal?.aborted) {
      throw new AbortError(options.signal)
    }

    this[kOpening] = new InvertedPromise()

    const result = await ipc.request('fs.open', {
      id,
      mode,
      path,
      flags
    }, options)

    if (result.err) {
      return this[kOpening].reject(result.err)
    }

    this.fd = result.data.fd

    fds.set(this.id, this.fd, 'file')

    this[kOpening].resolve(true)

    this.emit('open', this.fd)

    dc.channel('handle.open').publish({ handle: this, mode, path, flags })

    return true
  }

  /**
   * Reads `length` bytes starting from `position` into `buffer` at
   * `offset`.
   * @param {Buffer|object} buffer
   * @param {number} offset
   * @param {number} length
   * @param {number} position
   * @param {object=} [options]
   */
  async read (buffer, offset, length, position, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const { id } = this

    let bytesRead = 0
    let timeout = options?.timeout || null
    let signal = options?.signal || null

    if (typeof buffer === 'object' && !isBufferLike(buffer)) {
      offset = buffer.offset ?? 0
      length = buffer.length ?? buffer.byteLength ?? 0
      position = buffer.position ?? 0
      signal = buffer.signal ?? signal
      timeout = buffer.timeout ?? timeout
      buffer = buffer.buffer ?? buffer
    }

    if (signal?.aborted) {
      throw new AbortError(signal)
    }

    if (!isBufferLike(buffer)) {
      throw new TypeError('Expecting buffer to be a Buffer or TypedArray.')
    }

    if (offset === undefined) {
      offset = 0
    }

    if (length === undefined) {
      length = buffer.byteLength - offset
    }

    if (position === null) {
      position = -1
    }

    if (typeof position !== 'number') {
      position = 0
    }

    if (typeof offset !== 'number') {
      throw new TypeError(`Expecting offset to be a number. Got ${typeof offset}`)
    }

    if (typeof length !== 'number') {
      throw new TypeError(`Expecting length to be a number. Got ${typeof length}`)
    }

    if (offset < 0) {
      throw new RangeError(
        `Expecting offset to be greater than or equal to 0: Got ${offset}`
      )
    }

    if (offset + length > buffer.length) {
      throw new RangeError('Offset + length cannot be larger than buffer length.')
    }

    if (length < 0) {
      throw new RangeError(
        `Expecting length to be greater than or equal to 0: Got ${length}`
      )
    }

    if (isTypedArray(buffer)) {
      buffer = Buffer.from(buffer.buffer) // from ArrayBuffer
    }

    if (length > buffer.byteLength - offset) {
      throw new RangeError(
        `Expecting length to be less than or equal to ${buffer.byteLength - offset}: ` +
        `Got ${length}`
      )
    }

    const result = await ipc.request('fs.read', {
      id,
      size: length,
      offset: position
    }, { signal, timeout, responseType: 'arraybuffer' })

    if (result.err) {
      throw result.err
    }

    const contentType = result.headers?.get('content-type')

    if (contentType && contentType !== 'application/octet-stream') {
      throw new TypeError(
        `Invalid response content type from 'fs.read'. Received: ${contentType}`
      )
    }

    if (isTypedArray(result.data) || result.data instanceof ArrayBuffer) {
      bytesRead = result.data.byteLength
      Buffer.from(result.data).copy(Buffer.from(buffer), 0, offset)
      dc.channel('handle.read').publish({ handle: this, bytesRead })
    } else if (isEmptyObject(result.data)) {
      // an empty response from mac returns an empty object sometimes
      bytesRead = 0
    } else {
      throw new TypeError(
        `Invalid response buffer from 'fs.read' Received: ${typeof result.data}`
      )
    }

    return { bytesRead, buffer }
  }

  /**
   * Reads the entire contents of a file and returns it as a buffer or a string
   * specified of a given encoding specified at `options.encoding`.
   * @param {object=} [options]
   * @param {string=} [options.encoding = 'utf8']
   * @param {object=} [options.signal]
   */
  async readFile (options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const buffers = []
    const signal = options?.signal
    const stream = this.createReadStream(options)

    if (signal instanceof AbortSignal) {
      if (signal.aborted) {
        throw new AbortError(signal)
      }

      signal.addEventListener('abort', () => {
        if (!stream.destroyed && !stream.destroying) {
          stream.destroy(new AbortError(signal))
        }
      })
    }

    // collect
    await new Promise((resolve, reject) => {
      stream.on('data', (buffer) => buffers.push(buffer))
      stream.once('end', resolve)
      stream.once('error', reject)
    })

    const buffer = Buffer.concat(buffers)

    if (typeof options?.encoding === 'string') {
      return buffer.toString(options.encoding)
    }

    return buffer
  }

  /**
   * @param {object=} [options]
   */
  async readv (buffers, position) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * Returns the stats of the underlying file.
   * @param {object=} [options]
   */
  async stat (options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const result = await ipc.request('fs.fstat', { ...options, id: this.id })

    if (result.err) {
      throw result.err
    }

    const stats = Stats.from(result.data, Boolean(options?.bigint))
    stats.handle = this
    return stats
  }

  /**
   * @param {object=} [options]
   */
  async sync () {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * @param {object=} [options]
   */
  async truncate (length) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * @param {object=} [options]
   */
  async utimes (atime, mtime) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }
  }

  /**
   * Writes `length` bytes at `offset` in `buffer` to the underlying file
   * at `position`.
   * @param {Buffer|object} buffer
   * @param {number} offset
   * @param {number} length
   * @param {number} position
   * @param {object=} [options]
   */
  async write (buffer, offset, length, position, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    let timeout = options?.timeout || null
    let signal = options?.signal || null

    if (typeof buffer === 'object' && !isBufferLike(buffer)) {
      offset = buffer.offset
      length = buffer.length
      position = buffer.position
      signal = buffer.signal || signal
      timeout = buffer.timeout || timeout
      buffer = buffer.buffer
    }

    if (signal?.aborted) {
      throw new AbortError(signal)
    }

    if (typeof buffer !== 'string' && !isBufferLike(buffer)) {
      throw new TypeError('Expecting buffer to be a string or Buffer.')
    }

    if (offset === undefined) {
      offset = 0
    }

    if (length === undefined) {
      length = buffer.length
    }

    if (position === null) {
      position = -1
    }

    if (typeof position !== 'number') {
      position = 0
    }

    if (length > buffer.length) {
      throw new RangeError('Length cannot be larger than buffer length.')
    }

    if (offset > buffer.length) {
      throw new RangeError('Offset cannot be larger than buffer length.')
    }

    if (offset + length > buffer.length) {
      throw new RangeError('Offset + length cannot be larger than buffer length.')
    }

    buffer = Buffer.from(buffer).subarray(offset, offset + length)

    const params = { id: this.id, offset: position }
    const result = await ipc.write('fs.write', params, buffer, {
      timeout,
      signal
    })

    if (result.err) {
      throw result.err
    }

    const bytesWritten = parseInt(result.data.result) || 0

    dc.channel('handle.write').publish({ handle: this, bytesWritten })

    return {
      buffer,
      bytesWritten
    }
  }

  /**
   * Writes `data` to file.
   * @param {string|Buffer|TypedArray|Array} data
   * @param {object=} [options]
   * @param {string=} [options.encoding = 'utf8']
   * @param {object=} [options.signal]
   */
  async writeFile (data, options) {
    if (this.closing || this.closed) {
      throw new Error('FileHandle is not opened')
    }

    const signal = options?.signal
    const stream = this.createWriteStream(options)
    const buffer = Buffer.from(data, options?.encoding ?? 'utf8')
    const buffers = splitBuffer(buffer, stream.highWaterMark)

    if (signal instanceof AbortSignal) {
      if (signal.aborted) {
        throw new AbortError(signal)
      }

      signal.addEventListener('abort', () => {
        if (!stream.destroyed && !stream.destroying) {
          stream.destroy(new AbortError(signal))
        }
      })
    }

    queueMicrotask(async () => {
      while (buffers.length) {
        const buffer = buffers.shift()
        if (!stream.write(buffer)) {
          // block until drain
          await new Promise((resolve) => stream.once('drain', resolve))
        }
      }

      stream.end(null)
    })

    await new Promise((resolve, reject) => {
      stream.once('finish', resolve)
      stream.once('close', resolve)
      stream.once('error', reject)
    })
  }

  /**
   * @param {object=} [options]
   */
  async writev (buffers, position) {
  }
}

/**
 * A container for a directory handle tracked in `fds` and opened in the
 * native layer.
 */
export class DirectoryHandle extends EventEmitter {
  /**
   * The max number of entries that can be bufferd with the `bufferSize`
   * option.
   */
  static get MAX_BUFFER_SIZE () { return 256 }
  static get MAX_ENTRIES () { return this.MAX_BUFFER_SIZE }

  /**
   * The default number of entries `Dirent` that are buffered
   * for each read request.
   */
  static get DEFAULT_BUFFER_SIZE () { return 32 }

  /**
   * Creates a `FileHandle` from a given `id` or `fd`
   * @param {string|number|DirectoryHandle|object} id
   * @return {DirectoryHandle}
   */
  static from (id) {
    if (id?.id) {
      return this.from(id.id)
    } else if (id?.fd) {
      return this.from(id.fd)
    }

    if (fds.get(id) !== id) {
      throw new Error('Invalid file descriptor for directory handle.')
    }

    return new this({ id })
  }

  /**
   * Asynchronously open a directory.
   * @param {string | Buffer | URL} path
   * @param {object=} [options]
   */
  static async open (path, options) {
    const handle = new this({ path })

    if (typeof handle.path !== 'string') {
      throw new TypeError('Expecting path to be a string, Buffer, or URL.')
    }

    await handle.open(options)

    return handle
  }

  /**
   * `DirectoryHandle` class constructor
   * @private
   * @param {object} options
   */
  constructor (options) {
    super()

    // String | Buffer | URL | { toString(): String }
    if (options?.path && typeof options.path.toString === 'function') {
      options.path = options.path.toString()
    }

    this[kOpening] = null
    this[kClosing] = null
    this[kClosed] = false

    // this id will be used to identify the file handle that is a
    // reference stored in the native side
    this.id = String(options?.id || rand64())
    this.path = options?.path || null

    // @TODO(jwerle): implement usage of this internally
    this.bufferSize = Math.min(
      DirectoryHandle.MAX_BUFFER_SIZE,
      (typeof options?.bufferSize === 'number' && options.bufferSize) ||
      DirectoryHandle.DEFAULT_BUFFER_SIZE
    )

    gc.ref(this, options)

    dc.channel('handle').publish({ handle: this })
  }

  /**
   * DirectoryHandle file descriptor id
   */
  get fd () {
    return this.id
  }

  /**
   * `true` if the `DirectoryHandle` instance has been opened.
   * @type {boolean}
   */
  get opened () {
    return this.id !== null && this.id === fds.get(this.id)
  }

  /**
   * `true` if the `DirectoryHandle` is opening.
   * @type {boolean}
   */
  get opening () {
    const opening = this[kOpening]
    return opening?.value !== true
  }

  /**
   * `true` if the `DirectoryHandle` is closing.
   * @type {boolean}
   */
  get closing () {
    const closing = this[kClosing]
    return Boolean(closing && closing?.value !== true)
  }

  /**
   * `true` if `DirectoryHandle` is closed.
   */
  get closed () {
    return this[kClosed]
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, options],
      async handle (id) {
        if (fds.has(id)) {
          console.warn('Closing DirectoryHandle on garbage collection')
          await ipc.send('fs.closedir', { id }, options)
          fds.release(id, false)
        }
      }
    }
  }

  /**
   * Opens the underlying handle for a directory.
   * @param {object=} options
   */
  async open (options) {
    if (this.opened) {
      return true
    }

    if (this[kOpening]) {
      return await this[kOpening]
    }

    const { path, id } = this

    if (options?.signal?.aborted) {
      throw new AbortError(options.signal)
    }

    this[kOpening] = new InvertedPromise()

    const result = await ipc.request('fs.opendir', { id, path }, options)

    if (result.err) {
      return this[kOpening].reject(result.err)
    }

    // directory file descriptors are not accessible because
    // `dirfd` is not portable on the native side
    fds.set(this.id, this.id, 'directory')

    this[kOpening].resolve(true)

    this.emit('open', this.fd)

    dc.channel('handle.open').publish({ handle: this, path })

    return true
  }

  /**
   * Close underlying directory handle
   * @param {object=} [options]
   */
  async close (options) {
    // wait for opening to finish before proceeding to close
    if (this[kOpening]) {
      await this[kOpening]
    }

    if (this[kClosing]) {
      return await this[kClosing]
    }

    if (!fds.get(this.id)) {
      throw new Error('DirectoryHandle is not opened')
    }

    const { id } = this

    if (options?.signal?.aborted) {
      throw new AbortError(options.signal)
    }

    this[kClosing] = new InvertedPromise()

    const result = await ipc.send('fs.closedir', { id }, options)

    if (result.err) {
      return this[kClosing].reject(result.err)
    }

    fds.release(this.id, false)
    gc.unref(this)

    this[kClosing].resolve(true)

    this[kOpening] = null
    this[kClosing] = null
    this[kClosed] = true

    this.emit('close')
    dc.channel('handle.close').publish({ handle: this })

    return true
  }

  /**
   * Reads
   * @param {object=} [options]
   */
  async read (options) {
    if (this[kOpening]) {
      await this[kOpening]
    }

    if (this.closing || this.closed) {
      throw new Error('DirectoryHandle is not opened')
    }

    if (options?.signal?.aborted) {
      throw new AbortError(options.signal)
    }

    const entries = clamp(options.entries, 1, DirectoryHandle.MAX_ENTRIES)
    const { id } = this

    const result = await ipc.request('fs.readdir', {
      id,
      entries
    }, options)

    if (result.err) {
      throw result.err
    }

    return result.data
  }
}

export default exports
