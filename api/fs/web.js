/* global ReadableStream, WritableStream, Blob */
import { DEFAULT_STREAM_HIGH_WATER_MARK } from './stream.js'
import { isBufferLike, toBuffer } from '../util.js'
import { NotAllowedError } from '../errors.js'
import mime from '../mime.js'
import path from '../path.js'
import fs from './promises.js'

const kFileSystemHandleFullName = Symbol('kFileSystemHandleFullName')
const kFileFullName = Symbol('kFileFullName')

// @ts-ignore
export const File = globalThis.File ??
  class File {
    get lastModifiedDate () { return new Date(0) }
    get lastModified () { return 0 }
    get name () { return null }
    get size () { return 0 }
    get type () { return '' }

    slice () {}

    async arrayBuffer () {}
    async text () {}
    stream () {}
  }

// @ts-ignore
export const FileSystemHandle = globalThis.FileSystemHandle ??
  class FileSystemHandle {
    get name () { return null }
    get kind () { return null }
  }

// @ts-ignore
export const FileSystemFileHandle = globalThis.FileSystemFileHandle ??
  class FileSystemFileHandle extends FileSystemHandle {
    getFile () {}
    async createWritable (options = null) {}
    async createSyncAccessHandle () {}
  }

// @ts-ignore
export const FileSystemDirectoryHandle = globalThis.FileSystemDirectoryHandle ??
  class FileSystemDirectoryHandle extends FileSystemHandle {
    async * entries () {}
    async * values () {}
    async * keys () {}
    async resolve (possibleDescendant) {}
    async removeEntry (name, options = null) {}
    async getDirectoryHandle (name, options = null) {}
    async getFileHandle (name, options = null) {}
  }

// @ts-ignore
export const FileSystemWritableFileStream = globalThis.FileSystemWritableFileStream ??
  class FileSystemWritableFileStream extends WritableStream {
    async seek (position) {}
    async truncate (size) {}
    async write (data) {}
  }

/**
 * Helper for creating an implementation adapters for various platform APIs.
 * @ignore
 * @param {new () => object} Adapter
 * @return {object}
 */
function adapter (Adapter) {
  return Object.getOwnPropertyDescriptors(
    typeof Adapter === 'object'
      ? Adapter
      : Adapter.prototype
  )
}

/**
 * Helper function for creating a non-construable native class with adapter
 * implementation
 * @ignore
 * @param {new () => object|object} Super
 * @param {new () => object} Adapter
 * @return {object}
 */
function create (Super, Adapter) {
  return Object.create(
    typeof Super === 'object' ? Super : Super.prototype,
    adapter(Adapter)
  )
}

/**
 * Creates a new `File` instance from `filename`.
 * @param {string} filename
 * @param {{ fd: fs.FileHandle, highWaterMark?: number }=} [options]
 * @return {File}
 */
export async function createFile (filename, options = null) {
  if (!globalThis.File) {
    console.warn('socket:fs/web: Missing platform \'File\' implementation')
  }

  const decoder = new TextDecoder()
  const stats = options?.fd ? await options.fd.stat() : await fs.stat(filename)
  const types = await mime.lookup(path.extname(filename).slice(1))
  const type = types[0]?.mime ?? ''

  const highWaterMark = Number.isFinite(options?.highWaterMark)
    ? options.highWaterMark
    : Math.min(stats.size, DEFAULT_STREAM_HIGH_WATER_MARK)

  return create(File, class File {
    get [kFileFullName] () { return filename }

    get lastModifiedDate () { return new Date(stats.mtimeMs) }
    get lastModified () { return stats.mtimeMs }
    get name () { return path.basename(filename) }
    get size () { return stats.size }
    get type () { return type }

    slice () {
      console.warn('socket:fs/web: File.slice() is not supported in implementation')
      // This synchronous interface is not supported
      // An empty and ephemeral `Blob` is returned instead
      return new Blob([], { type })
    }

    async arrayBuffer () {
      const stream = this.stream()
      let buffer = null

      for await (const chunk of stream) {
        if (!buffer) {
          buffer = chunk
        } else {
          const next = new Uint8Array(buffer.byteLength + chunk.byteLength)
          next.set(buffer, 0)
          next.set(chunk, buffer.byteLength)
          buffer = next
        }
      }

      return buffer ? buffer.buffer : new ArrayBuffer(0)
    }

    async text () {
      const arrayBuffer = await this.arrayBuffer()
      return decoder.decode(new Uint8Array(arrayBuffer))
    }

    stream () {
      let buffer = null
      let offset = 0
      let fd = null

      const stream = new ReadableStream({
        async start (controller) {
          fd = options?.fd || await fs.open(filename)
          fd.once('close', () => controller.close())
          if (highWaterMark === 0) {
            await fd.close()
            await controller.close()
            fd = null
            return
          }

          buffer = new Uint8Array(highWaterMark)
          const result = await fd.read(buffer, 0, highWaterMark, offset)
          offset += result.bytesRead

          if (result.bytesRead === 0) {
            await controller.close()
          } else {
            controller.enqueue(buffer.slice(0, result.bytesRead))
          }
        },

        async cancel () {
          await fd.close()
          fd = null
        },

        async pull (controller) {
          // @ts-ignore
          if (controller.byobRequest) {
            // @ts-ignore
            const { byobRequest } = /** @type {ReadableStreamBYOBRequest} */(controller)
            const result = await fd.read(
              // @ts-ignore
              byobRequest.view.buffer,
              byobRequest.view.byteOffset,
              byobRequest.view.byteLength,
              offset
            )

            offset += result.bytesRead
            byobRequest.respond(result.bytesRead)

            if (result.bytesRead === 0) {
              await controller.close()
            }
          } else {
            const result = await fd.read(buffer, 0, highWaterMark, offset)
            offset += result.bytesRead

            if (result.bytesRead === 0) {
              await controller.close()
            } else {
              controller.enqueue(buffer.slice(0, result.bytesRead))
            }
          }
        }
      })

      // provide async iterator so `for await (...)` works on the `stream`
      return Object.defineProperty(stream, Symbol.asyncIterator, {
        get () {
          const reader = stream.getReader()
          return async function * () {
            while (true) {
              const { done, value = null } = await reader.read()
              if (done) break
              yield value
            }
          }
        }
      })
    }
  })
}

/**
 * Creates a `FileSystemWritableFileStream` instance backed
 * by `socket:fs:` module from a given `FileSystemFileHandle` instance.
 * @param {string|File} file
 * @return {Promise<FileSystemFileHandle>}
 */
export async function createFileSystemWritableFileStream (handle, options) {
  if (!globalThis.FileSystemWritableFileStream) {
    console.warn('socket:fs/web: Missing platform \'FileSystemWritableFileStream\' implementation')
  }

  const file = handle.getFile()
  let offset = 0
  let fd = null

  if (options?.keepExistingData === true) {
    try {
      fd = await fs.open(file.name, 'r+')
    } catch {}
  }

  if (!fd) {
    fd = await fs.open(file.name, 'w+')
  }

  // @ts-ignore
  return create(FileSystemWritableFileStream, class FileSystemWritableFileStream {
    async seek (position) {
      offset = position
    }

    async truncate (position) {
      await fd.truncate(position)
    }

    async write (options) {
      if (!fd) {
        throw new TypeError('Not opened')
      }

      const position = options?.position || null
      const data = options?.data || options || null
      const size = options?.size || null
      const type = options?.type || 'write'

      if (type === 'seek') {
        if (!Number.isFinite(position) || position < 0) {
          throw new TypeError('Expecting options.position to be an unsigned integer')
        }

        offset = position
      }

      if (type === 'truncate') {
        if (!Number.isFinite(size) || size < 0) {
          throw new TypeError('Expecting options.size to be an unsigned integer')
        }

        await this.truncate(size)
      }

      if (type === 'write') {
        if (
          !isBufferLike(data) &&
          !(data instanceof Blob) &&
          !(data instanceof DataView) &&
          !(data instanceof ArrayBuffer) &&
          typeof data !== 'string'
        ) {
          throw new TypeError(
            'Expecting data to be an ArrayBuffer, TypedArray, DataView, Blob, or string'
          )
        }

        if (Number.isFinite(position)) {
          offset = position
        }

        const buffer = toBuffer(data)
        await fd.write(buffer, 0, buffer.byteLength, offset)
      }
    }

    async close () {
      await fd.close()
      fd = null
    }
  })
}

/**
 * Creates a `FileSystemFileHandle` instance backed by `socket:fs:` module from
 * a given `File` instance or filename string.
 * @param {string|File} file
 * @param {object} [options]
 * @return {Promise<FileSystemFileHandle>}
 */
export async function createFileSystemFileHandle (file, options = null) {
  const { writable = true } = options || {}

  if (!globalThis.FileSystemFileHandle) {
    console.warn('socket:fs/web: Missing platform \'FileSystemFileHandle\' implementation')
  }

  if (typeof file === 'string') {
    file = await createFile(file)
  }

  return create(FileSystemFileHandle, class FileSystemFileHandle {
    get [kFileSystemHandleFullName] () {
      return file[kFileFullName]
    }

    get name () {
      return file.name
    }

    get kind () {
      return 'file'
    }

    getFile () {
      return file
    }

    async isSameEntry (entry) {
      if (this === entry) {
        return true
      }

      if (this[kFileSystemHandleFullName] === entry[kFileSystemHandleFullName]) {
        return true
      }

      try {
        if ('FileSystemFileHandle' in globalThis) {
          const { isSameEntry } = globalThis.FileSystemFileHandle.prototype
          if (await isSameEntry.call(this, entry)) {
            return true
          }
        }
      } catch {}

      return false
    }

    async move (nameOrDestinationHandle, name = null) {
      if (writable === false) {
        throw new NotAllowedError('FileSystemFileHandle is in \'readonly\' mode')
      }

      let destination = null
      if (typeof nameOrDestinationHandle === 'string') {
        name = nameOrDestinationHandle
      } else if (nameOrDestinationHandle instanceof FileSystemDirectoryHandle) {
        destination = nameOrDestinationHandle
      }

      const newName = path.resolve(
        destination?.[kFileSystemHandleFullName] || this[kFileSystemHandleFullName],
        '..',
        name
      )

      await fs.rename(
        this[kFileSystemHandleFullName],
        newName
      )

      file = await createFile(newName)
    }

    async createWritable (options = null) {
      if (writable === false) {
        throw new NotAllowedError('FileSystemFileHandle is in \'readonly\' mode')
      }

      return await createFileSystemWritableFileStream(this, options)
    }
  })
}

/**
 * Creates a `FileSystemDirectoryHandle` instance backed by `socket:fs:` module
 * from a given directory name string.
 * @param {string} dirname
 * @return {Promise<FileSystemFileHandle>}
 */
export async function createFileSystemDirectoryHandle (dirname, options = null) {
  const { writable = true } = options || {}

  if (!globalThis.FileSystemDirectoryHandle) {
    console.warn('socket:fs/web: Missing platform \'FileSystemDirectoryHandle\' implementation')
  }

  if (!dirname || typeof dirname !== 'string') {
    throw new TypeError('Expecting directory name to be a string')
  }

  dirname = path.resolve(dirname)

  try {
    const hasAccess = await fs.access(dirname)
    if (!hasAccess) {
      throw new NotAllowedError(`Unable to access directory: ${dirname}`)
    }
  } catch (err) {
    if (err instanceof NotAllowedError) {
      throw err
    }

    throw err
  }

  // `fd` is opened with `lazyOpen` at on demand
  let fd = null

  return create(FileSystemDirectoryHandle, class FileSystemDirectoryHandle {
    get [kFileSystemHandleFullName] () {
      return dirname
    }

    get name () {
      return path.basename(dirname)
    }

    get kind () {
      return 'directory'
    }

    async isSameEntry (entry) {
      if (this === entry || this[kFileSystemHandleFullName] === entry[kFileSystemHandleFullName]) {
        return true
      }

      try {
        if ('FileSystemDirectoryHandle' in globalThis) {
          const { isSameEntry } = globalThis.FileSystemDirectoryHandle.prototype
          if (await isSameEntry.call(this, entry)) {
            return true
          }
        }
      } catch {}

      return false
    }

    async move (nameOrDestinationHandle, name = null) {
      if (!writable) {
        throw new NotAllowedError('FileSystemDirectoryHandle is in \'readonly\' mode')
      }

      let destination = null
      if (typeof nameOrDestinationHandle === 'string') {
        name = nameOrDestinationHandle
      } else if (nameOrDestinationHandle instanceof FileSystemDirectoryHandle) {
        destination = nameOrDestinationHandle
      }

      const newName = path.resolve(destination?.[kFileSystemHandleFullName] || dirname, '..', name)

      await fs.rename(dirname, newName)
      dirname = newName
    }

    async * entries () {
      await lazyOpen()
      for await (const entry of fd) {
        if (entry.isDirectory()) {
          yield [
            entry.name,
            await createFileSystemDirectoryHandle(
              path.join(dirname, entry.name),
              { writable }
            )
          ]
        } else if (entry.isFile()) {
          yield [
            entry.name,
            await createFileSystemFileHandle(
              path.join(dirname, entry.name),
              { writable }
            )
          ]
        }
      }
    }

    async * keys () {
      for await (const entry of this.entries()) {
        yield entry[0]
      }
    }

    async * values () {
      for await (const entry of this.entries()) {
        yield entry[1]
      }
    }

    async resolve (possibleDescendant) {
      await lazyOpen()

      if (!(possibleDescendant instanceof FileSystemHandle)) {
        throw new TypeError(
          'Expecting possibleDescendant to be an instance of FileSystemHandle'
        )
      }

      const filename = possibleDescendant[kFileSystemHandleFullName]
      if (!filename) {
        return null
      }

      try {
        const hasAccess = await fs.access(filename)
        if (!hasAccess) {
          return null
        }
      } catch (err) {
        return null
      }

      const relative = path.relative(dirname, filename)

      if (relative.startsWith('.')) {
        return null
      }

      const components = relative.split(path.sep)
      const paths = []

      while (components.length) {
        const component = components.shift()
        const last = paths[paths.length - 1]
        const value = last && component ? path.join(last, component) : component
        if (value) {
          paths.push(value)
        }
      }

      return paths.length > 0 ? paths : null
    }

    async getDirectoryHandle (name, options = null) {
      const filename = path.join(dirname, name)
      if (options?.create) {
        try {
          const stats = await fs.stats(filename)
          if (!stats.isDirectory()) {
            throw new NotAllowedError(`'${dirname}' is not a directory`)
          }
        } catch (err) {
          if (!/exist/i.test(err.message)) {
            throw err
          }

          await fs.mkdir(filename)
        }
      }

      return await createFileSystemDirectoryHandle(filename, { writable })
    }

    async getFileHandle (name, options = null) {
      const filename = path.resolve(dirname, name)
      let fd = null
      if (options?.create === true) {
        try {
          const stats = await fs.stats(filename)
          if (!stats.isFile()) {
            throw new NotAllowedError(`'${dirname}' is not a file`)
          }
        } catch (err) {
          if (!/exist/i.test(err.message)) {
            throw err
          }

          fd = await fs.open(filename, 'w+')
        }
      }

      const file = await createFile(filename, { fd })
      const handle = await createFileSystemFileHandle(file, { writable })

      return handle
    }

    async removeEntry (name, options = null) {
      if (!writable) {
        throw new NotAllowedError('FileSystemDirectoryHandle is in \'readonly\' mode')
      }

      const filename = path.resolve(dirname, name)
      const stats = await fs.stats(filename)
      let handle = options?.handle || null

      if (!handle) {
        if (stats.isDirectory()) {
          handle = await createFileSystemDirectoryHandle(filename, { writable })
        } else if (stats.isFile()) {
          handle = await createFileSystemFileHandle(filename, { writable })
        }
      }

      if (options?.recursive === true) {
        for await (const entry of handle.values()) {
          if (entry.kind === 'file') {
            await fs.unlink(entry[kFileSystemHandleFullName])
          } else {
            await handle.removeEntry(entry.name, { recursive: true, handle: entry })
          }
        }
      }

      await fs.rmdir(dirname)
    }
  })

  async function lazyOpen () {
    if (!fd) {
      fd = await fs.opendir(dirname)
    }
  }
}

export default {
  createFileSystemWritableFileStream,
  createFileSystemDirectoryHandle,
  createFileSystemFileHandle,
  createFile
}
