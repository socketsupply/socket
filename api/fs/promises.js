/**
 * @module FS.promises
 *
 *  * This module enables interacting with the file system in a way modeled on
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
 * import fs from 'socket:fs/promises'
 * ```
 */
import console from '../console.js'
import ipc from '../ipc.js'

import { Dir, Dirent, sortDirectoryEntries } from './dir.js'
import { DirectoryHandle, FileHandle } from './handle.js'
import { ReadStream, WriteStream } from './stream.js'
import * as constants from './constants.js'
import { Stats } from './stats.js'
import fds from './fds.js'

import * as exports from './promises.js'

// re-exports
export {
  constants,
  Dir,
  DirectoryHandle,
  Dirent,
  fds,
  FileHandle,
  ReadStream,
  Stats,
  WriteStream
}

/**
 * @typedef {import('../buffer.js').Buffer} Buffer
 * @typedef {import('.stats.js').Stats} Stats
 * @typedef {Uint8Array|Int8Array} TypedArray
 * @ignore
 */
async function visit (path, options, callback) {
  if (typeof options === 'function') {
    callback = options
    options = {}
  }

  const { flags, flag, mode } = options || {}

  const handle = await FileHandle.open(path, flags || flag, mode, options)

  // just visit `FileHandle`, without closing if given
  if (path instanceof FileHandle) {
    return await callback(handle)
  } else if (path?.fd) {
    return await callback(FileHandle.from(path.fd))
  }

  const value = await callback(handle)
  await handle.close(options)

  return value
}

/**
 * Asynchronously check access a file.
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesaccesspath-mode}
 * @param {string | Buffer | URL} path
 * @param {string?} [mode]
 * @param {object?} [options]
 */
export async function access (path, mode, options) {
  return await FileHandle.access(path, mode, options)
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fspromiseschmodpath-mode}
 * @param {string | Buffer | URL} path
 * @param {number} mode
 * @returns {Promise<void>}
 */
export async function chmod (path, mode) {
  if (typeof mode !== 'number') {
    throw new TypeError(`The argument 'mode' must be a 32-bit unsigned integer or an octal string. Received ${mode}`)
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError(`The value of 'mode" is out of range. It must be an integer. Received ${mode}`)
  }

  const result = await ipc.send('fs.chmod', { mode, path })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function chown (path, uid, gid) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (!Number.isInteger(uid)) {
    throw new TypeError('The argument \'uid\' must be an integer')
  }

  if (!Number.isInteger(gid)) {
    throw new TypeError('The argument \'gid\' must be an integer')
  }

  const result = await ipc.send('fs.chown', { path, uid, gid })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function copyFile (src, dest, flags) {
  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  if (!Number.isInteger(flags)) {
    throw new TypeError('The argument \'flags\' must be an integer')
  }

  const result = await ipc.send('fs.copyFile', { src, dest, flags })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function lchown (path, uid, gid) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (!Number.isInteger(uid)) {
    throw new TypeError('The argument \'uid\' must be an integer')
  }

  if (!Number.isInteger(gid)) {
    throw new TypeError('The argument \'gid\' must be an integer')
  }

  const result = await ipc.send('fs.lchown', { path, uid, gid })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function link (src, dest) {
  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
  }

  const result = await ipc.send('fs.link', { src, dest })

  if (result.err) {
    throw result.err
  }
}

/**
 * Asynchronously creates a directory.
 * @todo recursive option is not implemented yet.
 *
 * @param {String} path - The path to create
 * @param {Object} options - The optional options argument can be an integer specifying mode (permission and sticky bits), or an object with a mode property and a recursive property indicating whether parent directories should be created. Calling fs.mkdir() when path is a directory that exists results in an error only when recursive is false.
 * @return {Promise<any>} - Upon success, fulfills with undefined if recursive is false, or the first directory path created if recursive is true.
 */
export async function mkdir (path, options = {}) {
  const mode = options.mode ?? 0o777
  const recursive = options.recurisve === true

  if (typeof mode !== 'number') {
    throw new TypeError('mode must be a number.')
  }

  if (mode < 0 || !Number.isInteger(mode)) {
    throw new RangeError('mode must be a positive finite number.')
  }

  const result = await ipc.request('fs.mkdir', { mode, path, recursive })

  if (result.err) {
    throw result.err
  }
}

/**
 * Asynchronously open a file.
 * @see {@link https://nodejs.org/api/fs.html#fspromisesopenpath-flags-mode }
 *
 * @param {string | Buffer | URL} path
 * @param {string} flags - default: 'r'
 * @param {string} mode - default: 0o666
 * @return {Promise<FileHandle>}
 */
export async function open (path, flags, mode) {
  return await FileHandle.open(path, flags, mode)
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fspromisesopendirpath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @param {string?} [options.encoding = 'utf8']
 * @param {number?} [options.bufferSize = 32]
 * @return {Promise<Dir>}
 */
export async function opendir (path, options) {
  const handle = await DirectoryHandle.open(path, options)
  return new Dir(handle, options)
}

/**
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreaddirpath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} options
 * @param {string?} [options.encoding = 'utf8']
 * @param {boolean?} [options.withFileTypes = false]
 */
export async function readdir (path, options) {
  options = { entries: DirectoryHandle.MAX_ENTRIES, ...options }

  const entries = []
  const handle = await DirectoryHandle.open(path, options)
  const dir = new Dir(handle, options)

  for await (const entry of dir.entries(options)) {
    entries.push(entry)
  }

  if (!dir.closing && !dir.closed) {
    try {
      await dir.close()
    } catch (err) {
      if (!/not opened/i.test(err.message)) {
        console.warn(err)
      }
    }
  }

  return entries.sort(sortDirectoryEntries)
}

/**
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromisesreadfilepath-options}
 * @param {string} path
 * @param {object?} [options]
 * @param {(string|null)?} [options.encoding = null]
 * @param {string?} [options.flag = 'r']
 * @param {AbortSignal?} [options.signal]
 * @return {Promise<Buffer | string>}
 */
export async function readFile (path, options) {
  if (typeof options === 'string') {
    options = { encoding: options }
  }

  options = { flags: 'r', ...options }

  return await visit(path, options, async (handle) => {
    return await handle.readFile(options)
  })
}

/**
 * @ignore
 */
export async function readlink (path) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = await ipc.send('fs.readlink', { path })

  if (result.err) {
    throw result.err
  }

  return result.data.path
}

/**
 * @ignore
 */
export async function realpath (path) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = await ipc.send('fs.realpath', { path })

  if (result.err) {
    throw result.err
  }

  return result.data.path
}

/**
 * @ignore
 */
export async function rename (oldPath, newPath) {
  if (typeof oldPath !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  if (typeof newPath !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = await ipc.send('fs.rename', { oldPath, newPath })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function rmdir (path) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = await ipc.send('fs.rmdir', { path })

  if (result.err) {
    throw result.err
  }
}

/**
 * @see {@link https://nodejs.org/api/fs.html#fspromisesstatpath-options}
 * @param {string | Buffer | URL} path
 * @param {object?} [options]
 * @param {boolean?} [options.bigint = false]
 * @return {Promise<Stats>}
 */
export async function stat (path, options) {
  return await visit(path, {}, async (handle) => {
    return await handle.stat(options)
  })
}

/**
 * @ignore
 */
export async function symlink (src, dest, type = null) {
  let flags = 0

  if (typeof src !== 'string') {
    throw new TypeError('The argument \'src\' must be a string')
  }

  if (typeof dest !== 'string') {
    throw new TypeError('The argument \'dest\' must be a string')
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

  const result = await ipc.send('fs.symlink', { src, dest, flags })

  if (result.err) {
    throw result.err
  }
}

/**
 * @ignore
 */
export async function unlink (path) {
  if (typeof path !== 'string') {
    throw new TypeError('The argument \'path\' must be a string')
  }

  const result = await ipc.send('fs.unlink', { path })

  if (result.err) {
    throw result.err
  }
}

/**
 * @see {@link https://nodejs.org/dist/latest-v20.x/docs/api/fs.html#fspromiseswritefilefile-data-options}
 * @param {string | Buffer | URL | FileHandle} path - filename or FileHandle
 * @param {string|Buffer|Array|DataView|TypedArray} data
 * @param {object?} [options]
 * @param {string|null} [options.encoding = 'utf8']
 * @param {number} [options.mode = 0o666]
 * @param {string} [options.flag = 'w']
 * @param {AbortSignal?} [options.signal]
 * @return {Promise<void>}
 */
// FIXME: truncate file by default (support flags). Currently it fails if file exists
export async function writeFile (path, data, options) {
  if (typeof options === 'string') {
    options = { encoding: options }
  }

  options = { flag: 'w', mode: 0o666, ...options }

  return await visit(path, options, async (handle) => {
    return await handle.writeFile(data, options)
  })
}

export default exports
