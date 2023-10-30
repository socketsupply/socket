import { EventEmitter } from '../events.js'
import { rand64 } from '../crypto.js'
import hooks from '../hooks.js'
import ipc from '../ipc.js'
import gc from '../gc.js'

/**
 * Starts the `fs.Watcher`
 * @ignore
 * @param {Watcher} watcher
 * @return {Promise}
 */
async function start (watcher) {
  const result = await ipc.send('fs.watch', {
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
 * @param {Watcher}
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

    watcher.emit('change', events[0], path)
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
   * @param {string|number|bigint=} [options.id]
   */
  constructor (path, options = {}) {
    super()

    this.id = options?.id || String(rand64())
    this.path = path

    gc.ref(this)

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
    const result = await ipc.send('fs.stopWatch', { id: this.id })
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
        ipc.send('fs.stopWatch', { id }, options)
      }
    }
  }

  /**
   * Implements the `AsyncIterator` (`Symbol.asyncIterator`) iterface.
   * @ignore
   * @return {AsyncIterator<{ eventType: string, filename: string }>}
   */
  [Symbol.asyncIterator] () {
    return {
      async next () {
        if (this.closed) {
          return { done: true, value: null }
        }

        const event = await new Promise((resolve) => {
          this.once('change', (eventType, filename) => {
            resolve({ eventType, filename })
          })
        })

        return { done: false, value: event }
      }
    }
  }
}

export default Watcher
