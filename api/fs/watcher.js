import { EventEmitter } from '../events.js'
import { FileHandle } from './handle.js'
import { AbortError } from '../errors.js'
import { rand64 } from '../crypto.js'
import { Buffer } from '../buffer.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

/**
 * Encodes filename based on encoding preference.
 * @ignore
 * @param {Watcher} watcher
 * @param {string} filename
 * @return {string|Buffer}
 */
function encodeFilename (watcher, filename) {
  if (!watcher.encoding || watcher.encoding === 'utf8') {
    return filename.toString()
  }

  if (watcher.encoding === 'buffer') {
    return Buffer.from(filename.toString())
  }

  return filename
}

/**
 * Starts the `fs.Watcher`
 * @ignore
 * @param {Watcher} watcher
 * @return {Promise}
 */
async function start (watcher) {
  // throws if not accessible
  await FileHandle.access(watcher.path)

  if (!watcher.id || typeof watcher.id !== 'string') {
    throw new TypeError('Expectig fs.Watcher to have a id.')
  }

  const result = await ipc.request('fs.watch', {
    path: watcher.path,
    id: watcher.id
  })

  if (result.err) {
    throw result.err
  }
}

/**
 * Internal watcher data event listeer.
 * @ignore
 * @param {Watcher} watcher
 * @return {function}
 */
function listen (watcher) {
  return hooks.onData((event) => {
    const { data, source } = event.detail.params
    if (source !== 'fs.watch') {
      return
    }

    if (BigInt(data.id) !== BigInt(watcher.id)) {
      return
    }

    const { path, events } = data

    watcher.emit('change', events[0], encodeFilename(watcher, path))
  })
}

/**
 * A container for a file system path watcher.
 */
export class Watcher extends EventEmitter {
  /**
   * The underlying `fs.Watcher` resource id.
   * @ignore
   * @type {string}
   */
  id = null

  /**
   * The path the `fs.Watcher` is watching
   * @type {string}
   */
  path = null

  /**
   * `true` if closed, otherwise `false.
   * @type {boolean}
   */
  closed = false

  /**
   * `true` if aborted, otherwise `false`.
   * @type {boolean}
   */
  aborted = false

  /**
   * The encoding of the `filename`
   * @type {'utf8'|'buffer'}
   */
  encoding = 'utf8'

  /**
   * A `AbortController` `AbortSignal` for async aborts.
   * @type {AbortSignal?}
   */
  signal = null

  /**
   * Internal event listener cancellation.
   * @ignore
   * @type {function?}
   */
  stopListening = null

  /**
   * `Watcher` class constructor.
   * @ignore
   * @param {string} path
   * @param {object=} [options]
   * @param {AbortSignal=} [options.signal}
   * @param {string|number|bigint=} [options.id]
   * @param {string=} [options.encoding = 'utf8']
   */
  constructor (path, options = null) {
    super()

    this.id = options?.id || String(rand64())
    this.path = path
    this.signal = options?.signal || null
    this.aborted = this.signal?.aborted === true
    this.encoding = options?.encoding || this.encoding

    gc.ref(this)

    if (this.signal?.aborted) {
      throw new AbortError(this.signal)
    }

    if (typeof this.signal?.addEventListener === 'function') {
      this.signal.addEventListener('abort', async () => {
        this.aborted = true
        try {
          await this.close()
        } catch (err) {
          console.warn('Failed to close fs.Watcher in AbortSignal:', err.message)
        }
      })
    }

    // internal
    if (options?.start !== false) {
      this.start()
    }
  }

  /**
   * Internal starter for watcher.
   * @ignore
   */
  async start () {
    try {
      await start(this)

      if (typeof this.stopListening === 'function') {
        this.stopListening()
      }

      this.stopListening = listen(this)
    } catch (err) {
      this.emit('err', err)
    }
  }

  /**
   * Closes watcher and stops listening for changes.
   * @return {Promise}
   */
  async close () {
    if (typeof this.stopListening === 'function') {
      this.stopListening()
      this.stopListening = null
    }

    this.closed = true
    const result = await ipc.request('fs.stopWatch', { id: this.id })
    if (result.err) {
      throw result.err
    }
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @ignore
   * @return {gc.Finalizer}
   */
  [gc.finalizer] (options) {
    return {
      args: [this.id, this.stopListening, options],
      handle (id, stopListening) {
        if (typeof stopListening === 'function') {
          stopListening()
        }

        console.warn('Closing fs.Watcher on garbage collection')
        ipc.request('fs.stopWatch', { id }, options)
      }
    }
  }

  /**
   * Implements the `AsyncIterator` (`Symbol.asyncIterator`) iterface.
   * @ignore
   * @return {AsyncIterator<{ eventType: string, filename: string }>}
   */
  [Symbol.asyncIterator] () {
    let watcher = this
    return {
      async next () {
        if (watcher?.aborted) {
          throw new AbortError(watcher.signal)
        }

        if (watcher.closed) {
          watcher = null
          return { done: true, value: null }
        }

        const event = await new Promise((resolve) => {
          watcher.once('change', (eventType, filename) => {
            resolve({ eventType, filename })
          })
        })

        return { done: false, value: event }
      }
    }
  }
}

export default Watcher
