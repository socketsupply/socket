/* global ErrorEvent */
import { Deferred } from '../async/deferred.js'
import serialize from '../internal/serialize.js'
import database from '../internal/database.js'
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
 * @typedef {{
 *   name: string
 * }} StorageOptions
 */

/**
 * An storage context object with persistence and durability
 * for service worker storages.
 */
export class Storage extends EventTarget {
  /**
   * Maximum entries that will be restored from storage into the context object.
   * @type {number}
   */
  static MAX_CONTEXT_ENTRIES = 16 * 1024

  /**
   * A mapping of known `Storage` instances.
   * @type {Map<string, Storage>}
   */
  static instances = new Map()

  /**
   * Opens an storage for a particular name.
   * @param {StorageOptions} options
   * @return {Promise<Storage>}
   */
  static async open (options) {
    if (Storage.instances.has(options?.name)) {
      const storage = Storage.instances.get(options.name)
      await storage.ready
      return storage
    }

    const storage = new this(options)
    Storage.instances.set(storage.name, storage)
    await storage.open()
    return storage
  }

  #database = null
  #opening = false
  #context = {}
  #proxy = null
  #ready = null
  #name = null

  /**
   * `Storage` class constructor
   * @ignore
   * @param {StorageOptions} options
   */
  constructor (options) {
    super()

    this.#name = options.name
    this.#proxy = new Proxy(this.#context, {
      get: (_, property) => {
        return this.#context[property]
      },

      set: (_, property, value) => {
        this.#context[property] = value
        if (this.database && this.database.opened) {
          this.forwardRequest(this.database.put(property, value, { durability: 'relaxed' }))
        }
        return true
      },

      deleteProperty: (_, property) => {
        if (this.database && this.database.opened) {
          this.forwardRequest(this.database.delete(property))
        }

        return Reflect.deleteProperty(this.#context, property)
      },

      getOwnPropertyDescriptor: (_, property) => {
        if (property in this.#context) {
          return {
            configurable: true,
            enumerable: true,
            writable: true,
            value: this.#context[property]
          }
        }
      },

      has: (_, property) => {
        return Reflect.has(this.#context, property)
      },

      ownKeys: (_) => {
        return Reflect.ownKeys(this.#context)
      }
    })
  }

  /**
   * A reference to the currently opened storage database.
   * @type {import('../internal/database.js').Database}
   */
  get database () {
    return this.#database
  }

  /**
   * `true` if the storage is opened, otherwise `false`.
   * @type {boolean}
   */
  get opened () {
    return this.#database?.opened === true
  }

  /**
   * `true` if the storage is opening, otherwise `false`.
   * @type {boolean}
   */
  get opening () {
    return this.#opening
  }

  /**
   * A proxied object for reading and writing storage state.
   * Values written to this object must be cloneable with respect to the
   * structured clone algorithm.
   * @see {https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Structured_clone_algorithm}
   * @type {Proxy<object>}
   */
  get context () {
    return this.#proxy
  }

  /**
   * The current storage name. This value is also used as the
   * internal database name.
   * @type {string}
   */
  get name () {
    return `socket.runtime.commonjs.cache.storage(${this.#name})`
  }

  /**
   * A promise that resolves when the storage is opened.
   * @type {Promise?}
   */
  get ready () {
    return this.#ready?.promise
  }

  /**
   * @ignore
   * @param {Promise} promise
   */
  async forwardRequest (promise) {
    try {
      return await promise
    } catch (error) {
      this.dispatchEvent(new ErrorEvent('error', { error }))
    }
  }

  /**
   * Resets the current storage to an empty state.
   */
  async reset () {
    await this.close()
    await database.drop(this.name)
    this.#database = null
    await this.open()
  }

  /**
   * Synchronizes database entries into the storage context.
   */
  async sync (options = null) {
    const entries = await this.#database.get(options?.query ?? undefined, {
      count: Storage.MAX_CONTEXT_ENTRIES
    })

    const promises = []
    const snapshot = Object.keys(this.#context)
    const delta = []

    for (const [key, value] of entries) {
      if (!snapshot.includes(key)) {
        delta.push(key)
      }

      this.#context[key] = value
    }

    for (const key of delta) {
      const value = this.#context[key]
      promises.push(this.forwardRequest(this.database.put(key, value, { durability: 'relaxed' })))
    }

    await Promise.all(promises)
  }

  /**
   * Opens the storage.
   * @ignore
   */
  async open (options = null) {
    if (!this.opening && !this.#database) {
      this.#opening = true
      this.#ready = new Deferred()
      this.#database = await database.open(this.name)
      await this.sync(options?.sync ?? null)
      this.#opening = false
      this.#ready.resolve()
    }

    return this.#ready.promise
  }

  /**
   * Closes the storage database, purging existing state.
   * @ignore
   */
  async close () {
    await this.#database.close()
    for (const key in this.#context) {
      Reflect.deleteProperty(this.#context, key)
    }
  }
}

/**
 * A container for `Snapshot` data storage.
 */
export class SnapshotData {
  /**
   * `SnapshotData` class constructor.
   * @param {object=} [data]
   */
  constructor (data = null) {
    // make the `prototype` a `null` value
    Object.setPrototypeOf(this, null)

    if (data && typeof data === 'object') {
      Object.assign(this, data)
    }

    this[Symbol.toStringTag] = 'Snapshot.Data'

    this.toJSON = () => {
      return { ...this }
    }
  }
}

/**
 * A container for storing a snapshot of the cache data.
 */
export class Snapshot {
  /**
   * @type {typeof SnapshotData}
   */
  static Data = SnapshotData

  // @ts-ignore
  #data = new Snapshot.Data()

  /**
   * `Snapshot` class constructor.
   */
  constructor () {
    for (const key in Cache.shared) {
      // @ts-ignore
      this.#data[key] = new Snapshot.Data(Object.fromEntries(Cache.shared[key].entries()))
    }
  }

  /**
   * A reference to the snapshot data.
   * @type {Snapshot.Data}
   */
  get data () {
    return this.#data
  }

  /**
   * @ignore
   */
  [Symbol.for('socket.runtime.serialize')] () {
    return { ...serialize(this.#data) }
  }

  /**
   * @ignore
   * @return {object}
   */
  toJSON () {
    return { ...serialize(this.#data) }
  }
}

/**
 * An interface for managing and performing operations on a collection
 * of `Cache` objects.
 */
export class CacheCollection {
  /**
   * `CacheCollection` class constructor.
   * @ignore
   * @param {Cache[]|Record<string, Cache>} collection
   */
  constructor (collection) {
    if (collection && typeof collection === 'object') {
      if (Array.isArray(collection)) {
        for (const value of collection) {
          if (value instanceof Cache) {
            this.add(value)
          }
        }
      } else {
        for (const key in collection) {
          const value = collection[key]
          this.add(key, value)
        }
      }
    }
  }

  /**
   * Adds a `Cache` instance to the collection.
   * @param {string|Cache} name
   * @param {Cache=} [cache]
   * @param {boolean}
   */
  add (name, cache = null) {
    if (name instanceof Cache) {
      return this.add(name.name, name)
    }

    if (typeof name === 'string' && cache instanceof Cache) {
      if (name in Object.getPrototypeOf(this)) {
        return false
      }

      Object.defineProperty(this, name, {
        configurable: false,
        enumerable: true,
        writable: false,
        value: cache
      })

      return true
    }

    return false
  }

  /**
   * Calls a method on each `Cache` object in the collection.
   * @param {string} method
   * @param {...any} args
   * @return {Promise<Record<string,any>>}
   */
  async call (method, ...args) {
    const results = {}

    for (const key in this) {
      const value = this[key]
      if (value instanceof Cache) {
        if (typeof value[method] === 'function') {
          results[key] = await value[method](...args)
        }
      }
    }

    return results
  }

  async restore () {
    return await this.call('restore')
  }

  async reset () {
    return await this.call('reset')
  }

  async snapshot () {
    return await this.call('snapshot')
  }

  async get (key) {
    return await this.call('get', key)
  }

  async delete (key) {
    return await this.call('delete', key)
  }

  async keys (key) {
    return await this.call('keys', key)
  }

  async values (key) {
    return await this.call('values', key)
  }

  async clear (key) {
    return await this.call('clear', key)
  }
}

/**
 * A container for a shared cache that lives for the life time of
 * application execution. Updates to this storage are replicated to other
 * instances in the application context, including windows and workers.
 */
export class Cache {
  /**
   * A globally shared type mapping for the cache to use when
   * derserializing a value.
   * @type {Map<string, function>}
   */
  static types = new Map()

  /**
   * A globally shared cache store keyed by cache name. This is useful so
   * when multiple instances of a `Cache` are created, they can share the
   * same data store, reducing duplications.
   * @type {Record<string, Map<string, object>}
   */
  static shared = Object.create(null)

  /**
   * A mapping of opened `Storage` instances.
   * @type {Map<string, Storage>}
   */
  static storages = Storage.instances

  /**
   * The `Cache.Snapshot` class.
   * @type {typeof Snapshot}
   */
  static Snapshot = Snapshot

  /**
   * The `Cache.Storage` class
   * @type {typeof Storage}
   */
  static Storage = Storage

  /**
   * Creates a snapshot of the current cache which can be serialized and
   * stored in persistent storage.
   * @return {Snapshot}
   */
  static snapshot () {
    return new Snapshot()
  }

  /**
   * Restore caches from persistent storage.
   * @param {string[]} names
   * @return {Promise}
   */
  static async restore (names) {
    const promises = []

    for (const name of names) {
      promises.push(Storage.open({ name }).then((storage) => {
        this.storages.set(name, storage)

        Cache.shared[name] ||= new Map()
        for (const key in storage.context) {
          const value = storage.context[key]
          Cache.shared[name].set(key, value)
        }
      }))
    }

    await Promise.all(promises)
  }

  #onmessage = null
  #storage = null
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
    this.#storage = Cache.storages.get(name) ?? new Storage({ name })
    this.#channel = new BroadcastChannel(`socket.runtime.commonjs.cache.${name}`)

    if (!Cache.storages.has(name)) {
      Cache.storages.set(name, this.#storage)
    }

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
    this.#storage.open()

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
   * A reference to the persisted storage.
   * @type {Storage}
   */
  get storage () {
    return this.#storage
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
   * @type {Map}
   */
  get types () {
    return this.#types
  }

  /**
   * Resets the cache map and persisted storage.
   */
  async reset () {
    this.#data.clear()
    await this.#storage.reset()
  }

  /**
   * Restores cache data from storage.
   */
  async restore () {
    if (!this.#storage.opened) {
      await this.#storage.open()
    }

    for (const key in this.#storage.context) {
      const value = this.#storage.context[key]

      if (value && !this.#data.has(key)) {
        if (value?.__type__) {
          this.#data.set(key, this.#types.get(value.__type__).from(value, {
            loader: this.loader
          }))
        } else {
          this.#data.set(key, value)
        }
      }
    }
  }

  /**
   * Creates a snapshot of the current cache which can be serialized and
   * stored in persistent storage.
   * @return {Snapshot.Data}
   */
  snapshot () {
    const snapshot = new Snapshot()
    return snapshot.data[this.name]
  }

  /**
   * Get a value at `key`.
   * @param {string} key
   * @return {object|undefined}
   */
  get (key) {
    const types = Array.from(this.types.values())
    let value = null

    if (this.#data.has(key)) {
      value = this.#data.get(key)
    } else if (key in this.#storage.context) {
      value = this.#storage.context[key]
    }

    if (!value) {
      return
    }

    // late init from type
    if (value?.__type__ && this.#types.has(value.__type__)) {
      value = this.#types.get(value.__type__).from(value, {
        loader: this.#loader
      })
    } else if (types.length === 1) {
      // if there is only 1 type in this cache types mapping, it most likely is the
      // general type used for this cache, so try to use it
      const [Type] = types
      if (typeof Type === 'function' && !(value instanceof Type)) {
        if (typeof Type.from === 'function') {
          value = Type.from(value, {
            loader: this.#loader
          })
        }
      }
    }

    // reset the value
    this.#data.set(key, value)

    return value
  }

  /**
   * Set `value` at `key`.
   * @param {string} key
   * @param {object} value
   * @return {Cache}
   */
  set (key, value) {
    this.#data.set(key, value)
    this.#storage.context[key] = serialize(value)
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
   * @return {boolean}
   */
  delete (key) {
    delete this.#storage.context[key]
    if (this.#data.delete(key)) {
      return true
    }

    return false
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
      async handle (data, channel, onmessage) {
        data.clear()
        channel.removeEventListener('message', onmessage)
      }
    }
  }
}

export default Cache
