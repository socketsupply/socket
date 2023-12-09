/**
 * @module FS
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
import console from '../console.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

import { Dir, Dirent, sortDirectoryEntries } from './dir.js'
import { DirectoryHandle, FileHandle } from './handle.js'
import { ReadStream, WriteStream } from './stream.js'
import * as constants from './constants.js'
import * as promises from './promises.js'
import { Watcher } from './watcher.js'
import { Stats } from './stats.js'
import fds from './fds.js'

import * as exports from './index.js'

/**
 * @typedef {import('../buffer.js').Buffer} Buffer
 * @typedef {Uint8Array|Int8Array} TypedArray
 * @ignore
 */
function defaultCallback (err) {
  if (err) throw err
}

async function visit (path, options = null, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

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
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {string?|function(Error?)?} [mode = F_OK(0)]
 * @param {function(Error?)?} [callback]
 */
export function access (path, mode, callback) {
  if (typeof mode === 'function') {
    callback = mode
    mode = FileHandle.DEFAULT_ACCESS_MODE
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  FileHandle
    .access(path, mode)
    .then((mode) => callback(null, mode))
    .catch((err) => callback(err))
}

/**
 * @ignore
 */
export function appendFile (path, data, options, callback) {
}

/**
 *
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

  ipc.request('fs.chmod', { mode, path }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  })
}

/**
 * Changes ownership of file or directory at `path` with `uid` and `gid`.
 * @param {string} path
 * @param {number} uid
 * @param {number} gid
 * @param {function} callback
 */
export function chown (path, uid, gid, callback) {
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

  ipc.request('fs.chown', { path, uid, gid }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * Asynchronously close a file descriptor calling `callback` upon success or error.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsclosefd-callback}
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
 * Asynchronously copies `src` to `dest` calling `callback` upon success or error.
 * @param {string} src - The source file path.
 * @param {string} dest - The destination file path.
 * @param {number} flags - Modifiers for copy operation.
 * @param {function(Error=)=} [callback] - The function to call after completion.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscopyfilesrc-dest-mode-callback}
 */
export function copyFile (src, dest, flags, callback) {
  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (!Number.isInteger(flags)) {
    throw new TypeError('The argument \'flags\' must be an integer')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.copyFile', { src, dest, flags }).then((result) => {
    result?.err ? callback(result.err) : callback(null)
  }).catch(callback)
}

/**
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @returns {ReadStream}
 */
export function createReadStream (path, options) {
  if (path?.fd) {
    options = path
    path = options?.path || null
  }

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
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fscreatewritestreampath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @returns {WriteStream}
 */
export function createWriteStream (path, options) {
  if (path?.fd) {
    options = path
    path = options?.path || null
  }

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
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsfstatfd-options-callback}
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
  if ((typeof options === 'undefined') || (typeof options === 'function')) {
    throw new TypeError('options must be an object.')
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
 * Asynchronously open a file calling `callback` upon success or error.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsopenpath-flags-mode-callback}
 * @param {string | Buffer | URL} path
 * @param {string?} [flags = 'r']
 * @param {string?} [mode = 0o666]
 * @param {object?|function?} [options]
 * @param {function(Error?, number?)?} [callback]
 */
export function open (path, flags = 'r', mode = 0o666, options = null, callback) {
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

  FileHandle
    .open(path, flags, mode, options)
    .then((handle) => {
      gc.unref(handle)
      callback(null, handle.fd)
    })
    .catch((err) => callback(err))
}

/**
 * Asynchronously open a directory calling `callback` upon success or error.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback}
 * @param {string | Buffer | URL} path
 * @param {object?|function(Error?, Dir?)} [options]
 * @param {string?} [options.encoding = 'utf8']
 * @param {boolean?} [options.withFileTypes = false]
 * @param {function(Error?, Dir?)?} callback
 */
export function opendir (path, options = {}, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  DirectoryHandle
    .open(path, options)
    .then((handle) => callback(null, new Dir(handle, options)))
    .catch((err) => callback(err))
}

/**
 * Asynchronously read from an open file descriptor.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreadfd-buffer-offset-length-position-callback}
 * @param {number} fd
 * @param {object | Buffer | TypedArray} buffer - The buffer that the data will be written to.
 * @param {number} offset - The position in buffer to write the data to.
 * @param {number} length - The number of bytes to read.
 * @param {number | BigInt | null} position - Specifies where to begin reading from in the file. If position is null or -1 , data will be read from the current file position, and the file position will be updated. If position is an integer, the file position will be unchanged.
 * @param {function(Error?, number?, Buffer?)} callback
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
 * Asynchronously read all entries in a directory.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fsreaddirpath-options-callback}
 * @param {string | Buffer | URL } path
 * @param {object?|function(Error?, object[])} [options]
 * @param {string?} [options.encoding ? 'utf8']
 * @param {boolean?} [options.withFileTypes ? false]
 * @param {function(Error?, object[])} callback
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
 * @param {string | Buffer | URL | number } path
 * @param {object?|function(Error?, Buffer?)} [options]
 * @param {string?} [options.encoding ? 'utf8']
 * @param {string?} [options.flag ? 'r']
 * @param {AbortSignal?} [options.signal]
 * @param {function(Error?, Buffer?)} callback
 */
export function readFile (path, options = {}, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof options === 'string') {
    options = { encoding: options }
  }

  options = {
    flags: 'r',
    ...options
  }

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
 * Reads link at `path`
 * @param {string} path
 * @param {function(err, string)} callback
 */
export function readlink (path, callback) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
  }

  ipc.request('fs.readlink', { path }).then((result) => {
    result?.err ? callback(result.err) : callback(result.data.path)
  }).catch(callback)
}

/**
 * Computes real path for `path`
 * @param {string} path
 * @param {function(err, string)} callback
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
 * Renames file or directory at `src` to `dest`.
 * @param {string} src
 * @param {string} dest
 * @param {function} callback
 */
export function rename (src, dest, callback) {
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
 * Removes directory at `path`.
 * @param {string} path
 * @param {function} callback
 */
export function rmdir (path, callback) {
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
 *
 * @param {string | Buffer | URL | number } path - filename or file descriptor
 * @param {object?} options
 * @param {string?} [options.encoding ? 'utf8']
 * @param {string?} [options.flag ? 'r']
 * @param {AbortSignal?} [options.signal]
 * @param {function(Error?, Stats?)} callback
 */
export function stat (path, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  if (typeof callback !== 'function') {
    throw new TypeError('callback must be a function.')
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
 * Creates a symlink of `src` at `dest`.
 * @param {string} src
 * @param {string} dest
 */
export function symlink (src, dest, type = null, callback) {
  let flags = 0

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
 * @param {function} callback
 */
export function unlink (path, callback) {
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
 * @see {@url https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fswritefilefile-data-options-callback}
 * @param {string | Buffer | URL | number } path - filename or file descriptor
 * @param {string | Buffer | TypedArray | DataView | object } data
 * @param {object?} options
 * @param {string?} [options.encoding ? 'utf8']
 * @param {string?} [options.mode ? 0o666]
 * @param {string?} [options.flag ? 'w']
 * @param {AbortSignal?} [options.signal]
 * @param {function(Error?)} callback
 */
export function writeFile (path, data, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = { encoding: 'utf8' }
  }

  if (typeof options === 'string') {
    options = { encoding: options }
  }

  options = {
    mode: 0o666,
    flag: 'w',
    ...options
  }

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
 * Watch for changes at `path` calling `callback`
 * @param {string}
 * @param {function|object=} [options]
 * @param {string=} [options.encoding = 'utf8']
 * @param {?function} [callback]
 * @return {Watcher}
 */
export function watch (path, options, callback = null) {
  if (typeof options === 'function') {
    callback = options
  }

  const watcher = new Watcher(path, options)
  watcher.on('change', callback)
  return watcher
}

// re-exports
export {
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
  }
}
