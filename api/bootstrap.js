/**
 * @module Bootstrap
 *
 * This module is responsible for downloading the bootstrap file and writing it to disk.
 * It also provides a function to check the hash of the file on disk. This is used to
 * determine if the file needs to be downloaded again. The bootstrap file is used to
 * bootstrap the backend.
 */

import { createWriteStream } from './fs/index.js'
import { createDigest } from './crypto.js'
import { EventEmitter } from './events.js'
import { PassThrough } from './stream.js'
import { readFile } from './fs/promises.js'
import diagnostics from './diagnostics.js'

const dc = diagnostics.channels.group('bootstrap', [
  'handle',
  'run.start',
  'run.end',
  'write.start',
  'write.end',
  'download.start',
  'download.end'
])

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
    if (!options.url || !options.dest) {
      throw new Error('.url and .dest are required string properties on the object provided to the constructor at the first argument position')
    }

    super()
    this.options = options
    dc.channel('handle').publish({ bootstrap: this })
  }

  async run () {
    try {
      dc.channel('run.start').publish({ bootstrap: this })
      const fileBuffer = await this.download(this.options.url)
      await this.write({ fileBuffer, dest: this.options.dest })
      dc.channel('run.end').publish({ bootstrap: this })
    } catch (err) {
      if (this.listenerCount('error')) {
        this.emit('error', err)
      } else {
        throw err
      }
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
    dc.channel('write.start').publish({ bootstrap: this, dest })

    const passThroughStream = new PassThrough()
    const writeStream = createWriteStream(dest, { mode: 0o755 })
    let bytesWritten = 0

    passThroughStream.pipe(writeStream)
    passThroughStream.on('data', data => {
      bytesWritten += data.length
      const progress = bytesWritten / fileBuffer.byteLength
      this.emit('write-file-progress', progress)
    })

    passThroughStream.write(fileBuffer)
    passThroughStream.end()

    return new Promise((resolve, reject) => {
      writeStream.on('finish', () => {
        this.emit('write-file', { status: 'finished' })
        dc.channel('write.end').publish({ bootstrap: this, dest, bytesWritten })
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
    dc.channel('download.start').publish({ bootstrap: this, url })
    const response = await fetch(url, { mode: 'cors' })

    if (!response.ok) {
      throw new Error(`Bootstrap request failed: ${response.status} ${response.statusText}`)
    }

    const contentLength = parseInt(response.headers.get('Content-Length'))
    const fileData = new Uint8Array(contentLength)

    let currentProgress = 0
    let bytesRead = 0

    for await (const chunk of streamAsyncIterable(response.body)) {
      fileData.set(chunk, bytesRead)
      bytesRead += chunk.length
      const progress = (bytesRead / contentLength * 100) | 0

      if (progress !== currentProgress) {
        this.emit('download-progress', progress)
        currentProgress = progress
      }
    }

    dc.channel('download.end').publish({
      bootstrap: this,
      url,
      bytesRead,
      bytesTotal: contentLength
    })

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
