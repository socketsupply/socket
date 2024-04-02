import gc from '../gc.js'

/**
 * @typedef {{
 *   types?: object,
 *   loader?: import('./loader.js').Loader
 * }} CacheOptions
 */

export const CACHE_CHANNEL_MESSAGE_ID = 'id'
export const CACHE_CHANNEL_MESSAGE_REPLICATE = 'replicate'

/**
 * A container for a shared cache that lives for the life time of
 * application execution. Updates to this storage are replicated to other
 * instances in the application context, including windows and workers.
 */
export class Cache {
  static types = new Map()
  static shared = Object.create(null)

  #onmessage = null
  #channel = null
  #loader = null
  #name = ''
  #types = null
  #data = null
  #id = null

  /**
   * `Cache` class constructor.
   * @param {string} name
   * @param {CacheOptions=} [options]
   */
  constructor (name, options) {
    if (!name || typeof name !== 'string') {
      throw new TypeError(`Expecting 'name' to be a string. Received: ${name}`)
    }

    if (!Cache.shared[name]) {
      Cache.shared[name] = new Map()
    }

    this.#id = Math.random().toString(16).slice(2)
    this.#name = name
    this.#data = Cache.shared[name]
    this.#types = new Map(Cache.types.entries())
    this.#loader = options?.loader ?? null
    this.#channel = new BroadcastChannel(`socket.runtime.commonjs.cache.${name}`)

    if (options?.types && typeof options.types === 'object') {
      for (const key in options.types) {
        const value = options.types[key]
        if (typeof value === 'function') {
          this.#types.set(key, value)
        }
      }
    }

    this.#onmessage = (event) => {
      const { data } = event
      if (!data || typeof data !== 'object') {
        return
      }

      if (data[CACHE_CHANNEL_MESSAGE_ID] === this.#id) {
        return
      }

      // recv 'replicate'
      if (Array.isArray(data[CACHE_CHANNEL_MESSAGE_REPLICATE])) {
        for (const entry of data[CACHE_CHANNEL_MESSAGE_REPLICATE]) {
          if (!this.has(...entry)) {
            const [key, value] = entry
            if (value?.__type__) {
              this.#data.set(key, this.#types.get(value.__type__).from(value, {
                loader: this.#loader
              }))
            } else {
              this.#data.set(key, value)
            }
          }
        }
      }
    }

    this.#channel.addEventListener('message', this.#onmessage)

    gc.ref(this)
  }

  /**
   * The unique ID for this cache.
   * @type {string}
   */
  get id () {
    return this.#id
  }

  /**
   * The loader associated with this cache.
   * @type {import('./loader.js').Loader}
   */
  get loader () {
    return this.#loader
  }

  /**
   * The cache name
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * The underlying cache data map.
   * @type {Map}
   */
  get data () {
    return this.#data
  }

  /**
   * The broadcast channel associated with this cach.
   * @type {BroadcastChannel}
   */
  get channel () {
    return this.#channel
  }

  /**
   * The size of the cache.
   * @type {number}
   */
  get size () {
    return this.#data.size
  }

  /**
   * Get a value at `key`.
   * @param {string} key
   * @return {object|undefined}
   */
  get (key) {
    return this.#data.get(key)
  }

  /**
   * Set `value` at `key`.
   * @param {string} key
   * @param {object} value
   * @return {Cache}
   */
  set (key, value) {
    this.#data.set(key, value)
    return this
  }

  /**
   * Returns `true` if `key` is in cache, otherwise `false`.
   * @param {string}
   * @return {boolean}
   */
  has (key) {
    return this.#data.has(key)
  }

  /**
   * Delete a value at `key`.
   * This does not replicate to shared caches.
   * @param {string} key
   * @return {object|undefined}
   */
  delete (key) {
    return this.#data.delete(key)
  }

  /**
   * Returns an iterator for all cache keys.
   * @return {object}
   */
  keys () {
    return this.#data.keys()
  }

  /**
   * Returns an iterator for all cache values.
   * @return {object}
   */
  values () {
    return this.#data.values()
  }

  /**
   * Returns an iterator for all cache entries.
   * @return {object}
   */
  entries () {
    return this.#data.entries()
  }

  /**
   * Clears all entries in the cache.
   * This does not replicate to shared caches.
   * @return {undefined}
   */
  clear () {
    this.#data.clear()
  }

  /**
   * Enumerates entries in map calling `callback(value, key
   * @param {function(object, string, Cache): any} callback
   */
  forEach (callback) {
    if (!callback || typeof callback !== 'function') {
      throw new TypeError(`${callback} is not a function`)
    }

    this.#data.forEach((value, key) => {
      callback(value, key, this)
    })
  }

  /**
   * Broadcasts a replication to other shared caches.
   */
  replicate () {
    const entries = Array.from(this.#data.entries())

    if (entries.length === 0) {
      return this
    }

    const message = {
      [CACHE_CHANNEL_MESSAGE_ID]: this.#id,
      [CACHE_CHANNEL_MESSAGE_REPLICATE]: entries
    }

    this.#channel.postMessage(message)

    return this
  }

  /**
   * Destroys the cache. This function stops the broadcast channel and removes
   * and listeners
   */
  destroy () {
    this.#channel.removeEventListener('message', this.#onmessage)
    this.#data.clear()
    this.#data = null
    this.#channel = null
    gc.unref(this)
  }

  /**
   * @ignore
   */
  [Symbol.iterator] () {
    return this.#data.entries()
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   * @ignore
   */
  [gc.finalizer] () {
    return {
      args: [this.#data, this.#channel, this.#onmessage],
      handle (data, channel, onmessage) {
        data.clear()
        channel.removeEventListener('message', onmessage)
      }
    }
  }
}

export default Cache
