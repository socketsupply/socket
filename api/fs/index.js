/**
 * @module fs
 *
 * This module enables interacting with the file system in a way modeled on
 * standard POSIX functions.
 *
 * The Application Sandbox restricts access to the file system.
 *
 * iOS Application Sandboxing has a set of rules that limits access to the file
 * system. Apps can only access files in their own sandboxed home directory.
 *
 * | Directory | Description |
 * | --- | --- |
 * | `Documents` | The app’s sandboxed documents directory. The contents of this directory are backed up by iTunes and may be set as accessible to the user via iTunes when `UIFileSharingEnabled` is set to `true` in the application's `info.plist`. |
 * | `Library` | The app’s sandboxed library directory. The contents of this directory are synchronized via iTunes (except the `Library/Caches` subdirectory, see below), but never exposed to the user. |
 * | `Library/Caches` | The app’s sandboxed caches directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. It's a good place to store data which provides a good offline-first experience for the user. |
 * | `Library/Preferences` | The app’s sandboxed preferences directory. The contents of this directory are synchronized via iTunes. Its purpose is to be used by the Settings app. Avoid creating your own files in this directory. |
 * | `tmp` | The app’s sandboxed temporary directory. The contents of this directory are not synchronized via iTunes and may be deleted by the system at any time. Although, it's recommended that you delete data that is not necessary anymore manually to minimize the space your app takes up on the file system. Use this directory to store data that is only useful during the app runtime. |
 *
 * Example usage:
 * ```js
 * import * as fs from 'socket:fs';
 * ```
 */

import { isBufferLike, isFunction, noop } from '../util.js'
import { rand64 } from '../crypto.js'
import { Buffer } from '../buffer.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

import { Dir, Dirent, sortDirectoryEntries } from './dir.js'
import { DirectoryHandle, FileHandle } from './handle.js'
import { ReadStream, WriteStream } from './stream.js'
import { normalizeFlags } from './flags.js'
import * as constants from './constants.js'
import * as promises from './promises.js'
import { Watcher } from './watcher.js'
import { Stats } from './stats.js'
import bookmarks from './bookmarks.js'
import fds from './fds.js'

import * as exports from './index.js'

const kFileDescriptor = Symbol.for('socket.runtime.fs.web.FileDescriptor')

/**
 * @typedef {import('../buffer.js').Buffer} Buffer
 * @typedef {Uint8Array|Int8Array} TypedArray
 * @ignore
 */
function defaultCallback (err) {
  if (err) throw err
}

function normalizePath (path) {
  if (path instanceof URL) {
    if (path.origin === globalThis.location.origin) {
      return normalizePath(path.href)
    }

    return null
  }

  if (URL.canParse(path)) {
    const url = new URL(path)
    if (url.origin === globalThis.location.origin) {
      path = `./${url.pathname.slice(1)}`
    }
  }

  return path
}

async function visit (path, options = null, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  path = normalizePath(path)

  const { flags, flag, mode } = options || {}

  let handle = null
  try {
    handle = await FileHandle.open(path, flags || flag, mode, options)
  } catch (err) {
    return callback(err)
  }

  if (handle) {
    await callback(null, handle)

    try {
      await handle.close(options)
    } catch (err) {
      console.warn(err.message || err)
    }
  }
}

/**
 * Asynchronously check access a file for a given mode calling `callback`
 * upon success or error.
 * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {number?|function(Error|null):any?} [mode = F_OK(0)]
 * @param {function(Error|null):any?} [callback]
 */
export function access (path, mode, callback) {
  if (typeof mode === 'function') {
    callback = mode
    mode = FileHandle.DEFAULT_ACCESS_MODE
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    return queueMicrotask(() => callback(null, mode))
  }

  path = normalizePath(path)

  FileHandle
    .access(path, mode)
    .then((mode) => callback(null, mode))
    .catch((err) => callback(err))
}

/**
 * Synchronously check access a file for a given mode calling `callback`
 * upon success or error.
 * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {string?} [mode = F_OK(0)]
 */
export function accessSync (path, mode = constants.F_OK) {
  path = normalizePath(path)
  const result = ipc.sendSync('fs.access', { path, mode })

  if (result.err) {
    throw result.err
  }

  // F_OK means access in any way
  return mode === constants.F_OK ? true : (result.data?.mode && mode) > 0
}

/**
 * Checks if a path exists
 * @param {string | Buffer | URL} path
 * @param {function(Boolean)?} [callback]
 */
export function exists (path, callback) {
  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  path = normalizePath(path)
  access(path, (err) => {
    // eslint-disable-next-line
    callback(err !== null)
  })
}

/**
 * Checks if a path exists
 * @param {string | Buffer | URL} path
 * @param {function(Boolean)?} [callback]
 */
export function existsSync (path) {
  path = normalizePath(path)
  try {
    accessSync(path)
    return true
  } catch {
    return false
  }
}

/**
 * Asynchronously changes the permissions of a file.
 * No arguments other than a possible exception are given to the completion callback
 *
 * @see {@link https://nodejs.org/api/fs.html#fschmodpath-mode-callback}
 *
 * @param {string | Buffer | URL} path
 * @param {number} mode
 * @param {function(Error?)} callback
 */
export function chmod (path, mode, callback) {
  if (typeof mode !== 'number') {
    throw new TypeError(`The argument 'mode' must be a 32-bit unsigned integer or an octal string. Received ${mode}`)
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError(`The value of "mode" is out of range. It must be an integer. Received ${mode}`)
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    return new TypeError(
      'FileSystemHandle is not writable'
    )
  }

  path = normalizePath(path)
  ipc.request('fs.chmod', { mode, path }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  })
}

/**
 * Synchronously changes the permissions of a file.
 *
 * @see {@link https://nodejs.org/api/fs.html#fschmodpath-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {number} mode
 */
export function chmodSync (path, mode) {
  if (typeof mode !== 'number') {
    throw new TypeError(`The argument 'mode' must be a 32-bit unsigned integer or an octal string. Received ${mode}`)
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError(`The value of "mode" is out of range. It must be an integer. Received ${mode}`)
  }

  path = normalizePath(path)
  const result = ipc.sendSync('fs.chmod', { mode, path })

  if (result.err) {
    throw result.err
  }
}

/**
 * Changes ownership of file or directory at `path` with `uid` and `gid`.
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 * @param {function} callback
 */
export function chown (path, uid, gid, callback) {
  path = normalizePath(path)
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (!Number.isInteger(uid)) {
    throw new TypeError('The argument \'uid\' must be an integer')
  }

  if (!Number.isInteger(gid)) {
    throw new TypeError('The argument \'gid\' must be an integer')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    return new TypeError(
      'FileSystemHandle is not writable'
    )
  }

  ipc.request('fs.chown', { path, uid, gid }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Changes ownership of file or directory at `path` with `uid` and `gid`.
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 */
export function chownSync (path, uid, gid) {
  path = normalizePath(path)
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (!Number.isInteger(uid)) {
    throw new TypeError('The argument \'uid\' must be an integer')
  }

  if (!Number.isInteger(gid)) {
    throw new TypeError('The argument \'gid\' must be an integer')
  }

  const result = ipc.sendSync('fs.chown', { path, uid, gid })

  if (result.err) {
    throw result.err
  }
}

/**
 * Asynchronously close a file descriptor calling `callback` upon success or error.
 * @see {@link https://nodejs.org/api/fs.html#fsclosefd-callback}
 * @param {number} fd
 * @param {function(Error?)?} [callback]
 */
export function close (fd, callback) {
  if (typeof callback !== 'function') {
    callback = defaultCallback
  }

  try {
    FileHandle
      .from(fd)
      .close()
      .then(() => callback(null))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Synchronously close a file descriptor.
 * @param {number} fd  - fd
 */
export function closeSync (fd) {
  const id = fds.get(fd) || fd
  const result = ipc.sendSync('fs.close', { id })
  if (result.err) {
    throw result.err
  }
  fds.release(id)
}

/**
 * Asynchronously copies `src` to `dest` calling `callback` upon success or error.
 * @param {string} src - The source file path.
 * @param {string} dest - The destination file path.
 * @param {number} flags - Modifiers for copy operation.
 * @param {function(Error=)=} [callback] - The function to call after completion.
 * @see {@link https://nodejs.org/api/fs.html#fscopyfilesrc-dest-mode-callback}
 */
export function copyFile (src, dest, flags = 0, callback) {
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (flags && !Number.isInteger(flags)) {
    throw new TypeError('The argument \'flags\' must be an integer')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (src instanceof globalThis.FileSystemFileHandle) {
    if (src.getFile()[kFileDescriptor]) {
      src = src.getFile()[kFileDescriptor].path
    } else {
      src.getFile().arrayBuffer.then((arrayBuffer) => {
        writeFile(dest, arrayBuffer, { flags }, callback)
      })
      return
    }
  }

  ipc.request('fs.copyFile', { src, dest, flags }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Synchronously copies `src` to `dest` calling `callback` upon success or error.
 * @param {string} src - The source file path.
 * @param {string} dest - The destination file path.
 * @param {number} flags - Modifiers for copy operation.
 * @see {@link https://nodejs.org/api/fs.html#fscopyfilesrc-dest-mode-callback}
 */
export function copyFileSync (src, dest, flags = 0) {
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (!Number.isInteger(flags)) {
    throw new TypeError('The argument \'flags\' must be an integer')
  }

  const result = ipc.sendSync('fs.copyFile', { src, dest, flags })

  if (result.err) {
    throw result.err
  }
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fscreatewritestreampath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @returns {ReadStream}
 */
export function createReadStream (path, options) {
  if (path?.fd) {
    options = path
    path = options?.path || null
  }

  path = normalizePath(path)

  let handle = null
  const stream = new ReadStream({
    autoClose: typeof options?.fd !== 'number',
    ...options
  })

  if (options?.fd) {
    handle = FileHandle.from(options.fd)
  } else {
    // @ts-ignore
    handle = new FileHandle({ flags: 'r', path, ...options })
    // @ts-ignore
    handle.open(options).catch((err) => stream.emit('error', err))
  }

  stream.once('end', async () => {
    if (options?.autoClose !== false) {
      try {
        await handle.close(options)
      } catch (err) {
        // @ts-ignore
        stream.emit('error', err)
      }
    }
  })

  stream.setHandle(handle)

  return stream
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fscreatewritestreampath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @returns {WriteStream}
 */
export function createWriteStream (path, options) {
  if (path?.fd) {
    options = path
    path = options?.path || null
  }

  if (path instanceof globalThis.FileSystemHandle) {
    return new TypeError(
      'FileSystemHandle is not writable'
    )
  }

  path = normalizePath(path)

  let handle = null
  const stream = new WriteStream({
    autoClose: typeof options?.fd !== 'number',
    ...options
  })

  if (typeof options?.fd === 'number') {
    handle = FileHandle.from(options.fd)
  } else {
    handle = new FileHandle({ flags: 'w', path, ...options })
    // @ts-ignore
    handle.open(options).catch((err) => stream.emit('error', err))
  }

  stream.once('finish', async () => {
    if (options?.autoClose !== false) {
      try {
        await handle.close(options)
      } catch (err) {
        // @ts-ignore
        stream.emit('error', err)
      }
    }
  })

  stream.setHandle(handle)

  return stream
}

/**
 * Invokes the callback with the <fs.Stats> for the file descriptor. See
 * the POSIX fstat(2) documentation for more detail.
 *
 * @see {@link https://nodejs.org/api/fs.html#fsfstatfd-options-callback}
 *
 * @param {number} fd - A file descriptor.
 * @param {object?|function?} [options] - An options object.
 * @param {function?} callback - The function to call after completion.
 */
export function fstat (fd, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  try {
    FileHandle
      .from(fd)
      .stat(options)
      .then(() => callback(null))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Request that all data for the open file descriptor is flushed
 * to the storage device.
 * @param {number} fd - A file descriptor.
 * @param {function} callback - The function to call after completion.
 */
export function fsync (fd, callback) {
  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  try {
    FileHandle
      .from(fd)
      .sync()
      .then(() => callback(null))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Truncates the file up to `offset` bytes.
 * @param {number} fd - A file descriptor.
 * @param {number=|function} [offset = 0]
 * @param {function?} callback - The function to call after completion.
 */
export function ftruncate (fd, offset, callback) {
  if (typeof offset === 'function') {
    callback = offset
    offset = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  try {
    FileHandle
      .from(fd)
      .truncate(offset)
      .then(() => callback(null))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Chages ownership of link at `path` with `uid` and `gid.
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 * @param {function} callback
 */
export function lchown (path, uid, gid, callback) {
  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    return new TypeError(
      'FileSystemHandle is not writable'
    )
  }

  path = normalizePath(path)

  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (!Number.isInteger(uid)) {
    throw new TypeError('The argument \'uid\' must be an integer')
  }

  if (!Number.isInteger(gid)) {
    throw new TypeError('The argument \'gid\' must be an integer')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.lchown', { path, uid, gid }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Creates a link to `dest` from `src`.
 * @param {string} src
 * @param {string} dest
 * @param {function}
 */
export function link (src, dest, callback) {
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.link', { src, dest }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * @ignore
 */
export function mkdir (path, options, callback) {
  path = normalizePath(path)

  if (typeof options === 'function') {
    callback = options
    options = null
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  const mode = options.mode || 0o777
  const recursive = Boolean(options.recursive) // default to false

  if (typeof mode !== 'number') {
    throw new TypeError('mode must be a number.')
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError('mode must be a positive finite number.')
  }

  ipc
    .request('fs.mkdir', { mode, path, recursive })
    .then(result => result?.err ? callback(result.err) : callback(null))
    .catch(err => callback(err))
}

/**
 * @ignore
 * @param {string|URL} path
 * @param {object=} [options]
 */
export function mkdirSync (path, options = null) {
  path = normalizePath(path)

  const mode = options?.mode || 0o777
  const recursive = Boolean(options?.recursive) // default to false

  if (typeof mode !== 'number') {
    throw new TypeError('mode must be a number.')
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError('mode must be a positive finite number.')
  }

  const result = ipc.sendSync('fs.mkdir', { mode, path, recursive })

  if (result.err) {
    throw result.err
  }
}

/**
 * Asynchronously open a file calling `callback` upon success or error.
 * @see {@link https://nodejs.org/api/fs.html#fsopenpath-flags-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {string=} [flags = 'r']
 * @param {number=} [mode = 0o666]
 * @param {(object|function(Error|null, number|undefined):any)=} [options]
 * @param {(function(Error|null, number|undefined):any)|null} [callback]
 */
export function open (path, flags = 'r', mode = 0o666, options = null, callback = null) {
  if (typeof flags === 'object') {
    callback = mode
    options = flags
    flags = FileHandle.DEFAULT_OPEN_FLAGS
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  if (typeof mode === 'object') {
    callback = options
    options = mode
    flags = FileHandle.DEFAULT_OPEN_FLAGS
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof flags === 'function') {
    callback = flags
    flags = FileHandle.DEFAULT_OPEN_FLAGS
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  if (typeof mode === 'function') {
    callback = mode
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  path = normalizePath(path)

  FileHandle
    .open(path, flags, mode, options)
    .then((handle) => {
      gc.unref(handle)
      callback(null, handle.fd)
    })
    .catch((err) => callback(err))
}

/**
 * Synchronously open a file.
 * @param {string|Buffer|URL} path
 * @param {string=} [flags = 'r']
 * @param {string=} [mode = 0o666]
 * @param {object=} [options]
 */
export function openSync (path, flags = 'r', mode = 0o666, options = null) {
  if (typeof flags === 'object') {
    options = flags
    flags = FileHandle.DEFAULT_OPEN_FLAGS
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  if (typeof mode === 'object') {
    options = mode
    flags = FileHandle.DEFAULT_OPEN_FLAGS
    mode = FileHandle.DEFAULT_OPEN_MODE
  }

  path = normalizePath(path)

  const id = String(options?.id || rand64())
  const result = ipc.sendSync('fs.open', {
    id,
    mode,
    path,
    flags
  }, {
    ...options
  })

  if (result.err) {
    throw result.err
  }

  if (result.data?.fd) {
    fds.set(id, result.data.fd, 'file')
  } else {
    fds.set(id, id, 'file')
  }

  return result.data?.fd || id
}

/**
 * Asynchronously open a directory calling `callback` upon success or error.
 * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
 * @param {string | Buffer | URL} path
 * @param {(object|function(Error|Null, Dir|undefined):any)=} [options]
 * @param {string=} [options.encoding = 'utf8']
 * @param {boolean=} [options.withFileTypes = false]
 * @param {function(Error|null, Dir|undefined):any)} callback
 */
export function opendir (path, options = {}, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  path = normalizePath(path)

  DirectoryHandle
    .open(path, options)
    .then((handle) => callback(null, new Dir(handle, options)))
    .catch((err) => callback(err))
}

/**
 * Synchronously open a directory.
 * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
 * @param {string|Buffer|URL} path
 * @param {objec} [options]
 * @param {string=} [options.encoding = 'utf8']
 * @param {boolean=} [options.withFileTypes = false]
 * @return {Dir}
 */
export function opendirSync (path, options = {}) {
  path = normalizePath(path)
  // @ts-ignore
  const id = String(options?.id || rand64())
  const result = ipc.sendSync('fs.opendir', { id, path }, options)

  if (result.err) {
    throw result.err
  }

  fds.set(id, id, 'directory')

  // @ts-ignore
  const handle = new DirectoryHandle({ id, path })
  return new Dir(handle, options)
}

/**
 * Asynchronously read from an open file descriptor.
 * @see {@link https://nodejs.org/api/fs.html#fsreadfd-buffer-offset-length-position-callback}
 * @param {number} fd
 * @param {object|Buffer|Uint8Array} buffer - The buffer that the data will be written to.
 * @param {number} offset - The position in buffer to write the data to.
 * @param {number} length - The number of bytes to read.
 * @param {number|BigInt|null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
 * @param {function(Error|null, number|undefined, Buffer|undefined):any} callback
 */
export function read (fd, buffer, offset, length, position, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof buffer === 'object' && !isBufferLike(buffer)) {
    options = buffer
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  try {
    FileHandle
      .from(fd)
      .read({ ...options, buffer, offset, length, position })
      .then(({ bytesRead, buffer }) => callback(null, bytesRead, buffer))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Asynchronously write to an open file descriptor.
 * @see {@link https://nodejs.org/api/fs.html#fswritefd-buffer-offset-length-position-callback}
 * @param {number} fd
 * @param {object|Buffer|Uint8Array} buffer - The buffer that the data will be written to.
 * @param {number} offset - The position in buffer to write the data to.
 * @param {number} length - The number of bytes to read.
 * @param {number|BigInt|null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
 * @param {function(Error|null, number|undefined, Buffer|undefined):any} callback
 */
export function write (fd, buffer, offset, length, position, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof buffer === 'object' && !isBufferLike(buffer)) {
    options = buffer
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  try {
    FileHandle
      .from(fd)
      .write({ ...options, buffer, offset, length, position })
      .then(({ bytesWritten, buffer }) => callback(null, bytesWritten, buffer))
      .catch((err) => callback(err))
  } catch (err) {
    callback(err)
  }
}

/**
 * Asynchronously read all entries in a directory.
 * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
 * @param {string|Buffer|URL} path
 * @param {object|function(Error|null, (Dirent|string)[]|undefined):any} [options]
 * @param {string=} [options.encoding = 'utf8']
 * @param {boolean=} [options.withFileTypes = false]
 * @param {function(Error|null, (Dirent|string)[]):any} callback
 */
export function readdir (path, options = {}, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (!options || typeof options !== 'object') {
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  path = normalizePath(path)
  options = {
    entries: DirectoryHandle.MAX_ENTRIES,
    withFileTypes: false,
    ...options
  }

  DirectoryHandle
    .open(path, options)
    .then(async (handle) => {
      const entries = []
      const dir = new Dir(handle, options)

      try {
        for await (const entry of dir.entries(options)) {
          entries.push(entry)
        }
      } catch (err) {
        return callback(err)
      } finally {
        if (!dir.closing && !dir.closed) {
          dir.close().catch(noop)
        }
      }

      callback(null, entries.sort(sortDirectoryEntries))
    })
    .catch((err) => callback(err))
}

/**
 * Synchronously read all entries in a directory.
 * @see {@link https://nodejs.org/api/fs.html#fsreaddirpath-options-callback}
 * @param {string|Buffer | URL } path
 * @param {object=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {boolean=} [options.withFileTypes ? false]
 * @return {(Dirent|string)[]}
 */
export function readdirSync (path, options = {}) {
  options = {
    entries: DirectoryHandle.MAX_ENTRIES,
    withFileTypes: false,
    ...options
  }
  const dir = opendirSync(path, options)
  const entries = []

  do {
    const entry = dir.readSync(options)
    if (!entry || entry.length === 0) {
      break
    }

    entries.push(...[].concat(entry))
  } while (true)

  dir.closeSync()
  return entries
}

/**
 * @param {string|Buffer|URL|number} path
 * @param {object|function(Error|null, Buffer|string|undefined):any} options
 * @param {string=} [options.encoding = 'utf8']
 * @param {string=} [options.flag = 'r']
 * @param {AbortSignal|undefined} [options.signal]
 * @param {function(Error|null, Buffer|string|undefined):any} callback
 */
export function readFile (path, options = {}, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof options === 'string') {
    options = { encoding: options }
  }

  path = normalizePath(path)
  options = { flags: 'r', ...options }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  visit(path, options, async (err, handle) => {
    let buffer = null

    if (err) {
      callback(err)
      return
    }

    try {
      buffer = await handle.readFile(options)
    } catch (err) {
      callback(err)
      return
    }

    callback(null, buffer)
  })
}

/**
 * @param {string|Buffer|URL|number} path
 * @param {{ encoding?: string = 'utf8', flags?: string = 'r'}} [options]
 * @param {object|function(Error|null, Buffer|undefined):any} [options]
 * @param {AbortSignal|undefined} [options.signal]
 * @return {string|Buffer}
 */
export function readFileSync (path, options = null) {
  if (typeof options === 'string') {
    options = { encoding: options }
  }

  path = normalizePath(path)
  options = {
    flags: 'r',
    encoding: options?.encoding,
    ...options
  }

  let result = null

  const stats = statSync(path)
  const flags = normalizeFlags(options.flags)
  const mode = FileHandle.DEFAULT_OPEN_MODE
  // @ts-ignore
  const id = String(options?.id || rand64())

  result = ipc.sendSync('fs.open', {
    mode,
    flags,
    id,
    path
  }, options)

  if (result.err) {
    throw result.err
  }

  result = ipc.sendSync('fs.read', {
    id,
    size: stats.size,
    offset: 0
  }, { responseType: 'arraybuffer' })

  if (result.err) {
    throw result.err
  }

  const data = result.data

  result = ipc.sendSync('fs.close', { id }, options)

  if (result.err) {
    throw result.err
  }

  const buffer = data ? Buffer.from(data) : Buffer.alloc(0)

  if (typeof options?.encoding === 'string') {
    return buffer.toString(options.encoding)
  }

  return buffer
}

/**
 * Reads link at `path`
 * @param {string} path
 * @param {function(Error|null, string|undefined):any} callback
 */
export function readlink (path, callback) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  path = normalizePath(path)
  ipc.request('fs.readlink', { path }).then((result) => {
    result?.err ? callback(result.err) : callback(null, result.data.path)
  }).catch(callback)
}

/**
 * Computes real path for `path`
 * @param {string} path
 * @param {function(Error|null, string|undefined):any} callback
 */
export function realpath (path, callback) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.realpath', { path }).then((result) => {
    result?.err ? callback(result.err) : callback(result.data.path)
  }).catch(callback)
}

/**
 * Computes real path for `path`
 * @param {string} path
 * @return {string}
 */
export function realpathSync (path) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = ipc.sendSync('fs.realpath', { path })

  if (result.err) {
    throw result.err
  }

  return result.data
}

/**
 * Renames file or directory at `src` to `dest`.
 * @param {string} src
 * @param {string} dest
 * @param {function(Error|null):any} callback
 */
export function rename (src, dest, callback) {
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.rename', { src, dest }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  })
}

/**
 * Renames file or directory at `src` to `dest`, synchronously.
 * @param {string} src
 * @param {string} dest
 */
export function renameSync (src, dest) {
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  const result = ipc.sendSync('fs.rename', { src, dest })

  if (result.err) {
    throw result.err
  }
}

/**
 * Removes directory at `path`.
 * @param {string} path
 * @param {function(Error|null):any} callback
 */
export function rmdir (path, callback) {
  path = normalizePath(path)

  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.rmdir', { path }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  })
}

/**
 * Removes directory at `path`, synchronously.
 * @param {string} path
 */
export function rmdirSync (path) {
  path = normalizePath(path)

  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = ipc.sendSync('fs.rmdir', { path })

  if (result.err) {
    throw result.err
  }
}

/**
 * Synchronously get the stats of a file
 * @param {string} path - filename or file descriptor
 * @param {object=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {string=} [options.flag ? 'r']
 */
export function statSync (path, options = null) {
  path = normalizePath(path)
  const result = ipc.sendSync('fs.stat', { path })

  if (result.err) {
    throw result.err
  }

  return Stats.from(result.data, Boolean(options?.bigint))
}

/**
 * Get the stats of a file
 * @param {string|Buffer|URL|number} path - filename or file descriptor
 * @param {(object|function(Error|null, Stats|undefined):any)=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {string=} [options.flag ? 'r']
 * @param {AbortSignal|undefined} [options.signal]
 * @param {function(Error|null, Stats|undefined):any} callback
 */
export function stat (path, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    if (path.getFile()[kFileDescriptor]) {
      try {
        path.getFile()[kFileDescriptor].stat(options).then(
          (stats) => callback(null, stats),
          (err) => callback(err)
        )
      } catch (err) {
        callback(err)
      }

      return
    } else {
      queueMicrotask(() => {
        const info = {
          st_mode: path instanceof globalThis.FileSystemDirectoryHandle
            ? constants.S_IFDIR
            : constants.S_IFREG,
          st_size: path instanceof globalThis.FileSystemFileHandle
            ? path.size
            : 0
        }

        const stats = Stats.from(info, Boolean(options?.bigint))
        callback(null, stats)
      })

      return
    }
  }

  visit(path, {}, async (err, handle) => {
    let stats = null

    if (err) {
      callback(err)
      return
    }

    try {
      stats = await handle.stat(options)
    } catch (err) {
      callback(err)
      return
    }

    callback(null, stats)
  })
}

/**
 * Get the stats of a symbolic link
 * @param {string|Buffer|URL|number} path - filename or file descriptor
 * @param {(object|function(Error|null, Stats|undefined):any)=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {string=} [options.flag ? 'r']
 * @param {AbortSignal|undefined} [options.signal]
 * @param {function(Error|null, Stats|undefined):any} callback
 */
export function lstat (path, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  if (
    path instanceof globalThis.FileSystemFileHandle ||
    path instanceof globalThis.FileSystemDirectoryHandle
  ) {
    return stat(path, options, callback)
  }

  visit(path, {}, async (err, handle) => {
    let stats = null

    if (err) {
      callback(err)
      return
    }

    try {
      stats = await handle.lstat(options)
    } catch (err) {
      callback(err)
      return
    }

    callback(null, stats)
  })
}

/**
 * Creates a symlink of `src` at `dest`.
 * @param {string} src
 * @param {string} dest
 * @param {function(Error|null):any} callback
 */
export function symlink (src, dest, type = null, callback) {
  let flags = 0
  src = normalizePath(src)
  dest = normalizePath(dest)

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (typeof type === 'function') {
    callback = type
  }

  if (!type) {
    type = 'file'
  }

  if (type === 'file') {
    flags = 0
  } else if (type === 'dir') {
    flags = constants.UV_FS_SYMLINK_DIR
  } else if (type === 'junction') {
    flags = constants.UV_FS_SYMLINK_JUNCTION
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.symlink', { src, dest, flags }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Unlinks (removes) file at `path`.
 * @param {string} path
 * @param {function(Error|null):any} callback
 */
export function unlink (path, callback) {
  path = normalizePath(path)

  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.unlink', { path }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Unlinks (removes) file at `path`, synchronously.
 * @param {string} path
 */
export function unlinkSync (path) {
  path = normalizePath(path)

  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = ipc.sendSync('fs.unlink', { path })

  if (result.err) {
    throw result.err
  }
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fswritefilefile-data-options-callback}
 * @param {string|Buffer|URL|number} path - filename or file descriptor
 * @param {string|Buffer|TypedArray|DataView|object} data
 * @param {(object|function(Error|null):any)=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {string=} [options.mode ? 0o666]
 * @param {string=} [options.flag ? 'w']
 * @param {AbortSignal|undefined} [options.signal]
 * @param {function(Error|null):any} callback
 */
export function writeFile (path, data, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = { encoding: 'utf8' }
  }

  if (typeof options === 'string') {
    options = { encoding: options }
  }

  path = normalizePath(path)
  options = { mode: 0o666, flag: 'w', ...options }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  visit(path, options, async (err, handle) => {
    if (err) {
      callback(err)
      return
    }

    try {
      await handle.writeFile(data, options)
    } catch (err) {
      callback(err)
      return
    }

    callback(null)
  })
}

/**
 * Writes data to a file synchronously.
 * @param {string|Buffer|URL|number} path - filename or file descriptor
 * @param {string|Buffer|TypedArray|DataView|object} data
 * @param {object=} [options]
 * @param {string=} [options.encoding ? 'utf8']
 * @param {string=} [options.mode ? 0o666]
 * @param {string=} [options.flag ? 'w']
 * @param {AbortSignal|undefined} [options.signal]
 * @see {@link https://nodejs.org/api/fs.html#fswritefilesyncfile-data-options}
 */
export function writeFileSync (path, data, options) {
  const id = String(options?.id || rand64())

  let result = ipc.sendSync('fs.open', {
    id,
    mode: options?.mode || 0o666,
    path,
    flags: options?.flags ? normalizeFlags(options.flags) : 'w'
  }, options)

  if (result.err) {
    throw result.err
  }

  result = ipc.sendSync('fs.write', { id, offset: 0 }, null, data)

  if (result.err) {
    throw result.err
  }

  result = ipc.sendSync('fs.close', { id }, options)

  if (result.err) {
    throw result.err
  }
}

/**
 * Watch for changes at `path` calling `callback`
 * @param {string}
 * @param {function|object=} [options]
 * @param {string=} [options.encoding = 'utf8']
 * @param {function=} [callback]
 * @return {Watcher}
 */
export function watch (path, options, callback = null) {
  if (typeof options === 'function') {
    callback = options
  }

  path = normalizePath(path)
  const watcher = new Watcher(path, options)
  watcher.on('change', callback)
  return watcher
}

// re-exports
export {
  bookmarks,
  constants,
  Dir,
  DirectoryHandle,
  Dirent,
  fds,
  FileHandle,
  promises,
  ReadStream,
  Stats,
  Watcher,
  WriteStream
}

export default exports

for (const key in exports) {
  const value = exports[key]
  if (key in promises && isFunction(value) && isFunction(promises[key])) {
    value[Symbol.for('nodejs.util.promisify.custom')] = promises[key]
    value[Symbol.for('socket.runtime.util.promisify.custom')] = promises[key]
  }
}
