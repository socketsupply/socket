import { readFile } from './fs/promises.js'
import { createWriteStream } from './fs/index.js'
import { PassThrough } from './stream.js'
import { createDigest } from './crypto.js'
import { EventEmitter } from './events.js'

async function * streamAsyncIterable (stream) {
  const reader = stream.getReader()
  try {
    while (true) {
      const { done, value } = await reader.read()
      if (done) return
      yield value
    }
  } finally {
    reader.releaseLock()
  }
}

/**
 * @param {Buffer|String} buf
 * @param {string} hashAlgorithm
 * @returns {Promise<string>}
 */
async function getHash (buf, hashAlgorithm) {
  const digest = await createDigest(hashAlgorithm, buf)
  return digest.toString('hex')
}

/**
 * @param {string} dest - file path
 * @param {string} hash - hash string
 * @param {string} hashAlgorithm - hash algorithm
 * @returns {Promise<boolean>}
 */
export async function checkHash (dest, hash, hashAlgorithm) {
  let buf
  try {
    buf = await readFile(dest)
  } catch (err) {
    // download if file is corrupted or does not exist
    return false
  }
  return hash === await getHash(buf, hashAlgorithm)
}

class Bootstrap extends EventEmitter {
  constructor (options) {
    super()
    if (!options.url || !options.dest) {
      throw new Error('.url and .dest are required string properties on the object provided to the constructor at the first argument position')
    }
    this.options = options
  }

  async run () {
    try {
      const fileBuffer = await this.download(this.options.url)
      await this.write({ fileBuffer, dest: this.options.dest })
    } catch (err) {
      this.emit('error', err)
      throw err
    } finally {
      this.cleanup()
    }
  }

  /**
   * @param {object} options
   * @param {Uint8Array} options.fileBuffer
   * @param {string} options.dest
   * @returns {Promise<void>}
   */
  async write ({ fileBuffer, dest }) {
    this.emit('write-file', { status: 'started' })
    const passThroughStream = new PassThrough()
    const writeStream = createWriteStream(dest, { mode: 0o755 })
    passThroughStream.pipe(writeStream)
    let written = 0
    passThroughStream.on('data', data => {
      written += data.length
      const progress = written / fileBuffer.byteLength
      this.emit('write-file-progress', progress)
    })
    passThroughStream.write(fileBuffer)
    passThroughStream.end()
    return new Promise((resolve, reject) => {
      writeStream.on('finish', () => {
        this.emit('write-file', { status: 'finished' })
        resolve()
      })
      writeStream.on('error', err => {
        this.emit('error', err)
        reject(err)
      })
    })
  }

  /**
   * @param {string} url - url to download
   * @returns {Promise<Uint8Array>}
   * @throws {Error} - if status code is not 200
   */
  async download (url) {
    const response = await fetch(url, { mode: 'cors' })
    if (!response.ok) {
      throw new Error(`Bootstrap request failed: ${response.status} ${response.statusText}`)
    }
    const contentLength = +response.headers.get('Content-Length')
    let receivedLength = 0
    let prevProgress = 0
    const fileData = new Uint8Array(contentLength)

    for await (const chunk of streamAsyncIterable(response.body)) {
      fileData.set(chunk, receivedLength)
      receivedLength += chunk.length
      const progress = (receivedLength / contentLength * 100) | 0
      if (progress !== prevProgress) {
        this.emit('download-progress', progress)
        prevProgress = progress
      }
    }
    return fileData
  }

  cleanup () {
    this.removeAllListeners()
  }
}

export function bootstrap (options) {
  return new Bootstrap(options)
}

export default {
  bootstrap,
  checkHash
}
