/* global DOMException */
import { IllegalConstructorError } from '../errors.js'
import { Environment } from './env.js'
import { Deferred } from '../async/deferred.js'
import state from './state.js'
import ipc from '../ipc.js'

// used to ensure `Storage` is not illegally constructed
const createStorageSymbol = Symbol('Storage.create')

/**
 * @typedef {{ done: boolean, value: string | undefined }} IndexIteratorResult
 */

/**
 * An iterator interface for an `Index` instance.
 */
export class IndexIterator {
  #index = null
  #current = 0

  /**
   * `IndexIterator` class constructor.
   * @ignore
   * @param {Index} index
   */
  constructor (index) {
    this.#index = index
  }

  /**
   * `true` if the iterator is "done", otherwise `false`.
   * @type {boolean}
   */
  get done () {
    return this.#current === -1 || this.#current >= this.#index.length
  }

  /**
   * Returns the next `IndexIteratorResult`.
   * @return {IndexIteratorResult}
   */
  next () {
    if (this.done) {
      return { done: true, value: undefined }
    }

    const value = this.#index.entry(this.#current++)
    return { done: false, value }
  }

  /**
   * Mark `IndexIterator` as "done"
   * @return {IndexIteratorResult}
   */
  return () {
    this.#current = -1
    return { done: true, value: undefined }
  }
}

/**
 * A container used by the `Provider` to index keys and values
 */
export class Index {
  #keys = []
  #values = []

  /**
   * A reference to the keys in this index.
   * @type {string[]}
   */
  get keys () {
    return this.#keys
  }

  /**
   * A reference to the values in this index.
   * @type {string[]}
   */
  get values () {
    return this.#values
  }

  /**
   * The number of entries in this index.
   * @type {number}
   */
  get length () {
    return this.#keys.length
  }

  /**
   * Returns the key at a given `index`, if it exists otherwise `null`.
   * @param {number} index}
   * @return {string?}
   */
  key (index) {
    return this.#keys[index] ?? null
  }

  /**
   * Returns the value at a given `index`, if it exists otherwise `null`.
   * @param {number} index}
   * @return {string?}
   */
  value (index) {
    return this.#values[index] ?? null
  }

  /**
   * Inserts a value in the index.
   * @param {string} key
   * @param {string} value
   */
  insert (key, value) {
    const index = this.#keys.indexOf(key)
    if (index >= 0) {
      this.#values[index] = value
    } else {
      this.#keys.push(key)
      this.#values.push(value)
    }
  }

  /**
   * Computes the index of a key in this index.
   * @param {string} key
   * @return {number}
   */
  indexOf (key) {
    return this.#keys.indexOf(key)
  }

  /**
   * Clears all keys and values in the index.
   */
  clear () {
    this.#keys.splice(0, this.#keys.length)
    this.#values.splice(0, this.#values.length)
  }

  /**
   * Returns an entry at `index` if it exists, otherwise `null`.
   * @param {number} index
   * @return {string[]|null}
   */
  entry (index) {
    const key = this.key(index)
    const value = this.value(index)

    if (key) {
      return [key, value]
    }

    return null
  }

  /**
   * Removes entries at a given `index`.
   * @param {number} index
   * @return {boolean}
   */
  remove (index) {
    if (index >= 0 && index < this.#keys.length) {
      this.#keys.splice(index, 1)
      this.#values.splice(index, 1)
      return true
    }

    return false
  }

  /**
   * Returns an array of computed entries in this index.
   * @return {IndexIterator}
   */
  entries () {
    return this[Symbol.iterator]()
  }

  /**
   * @ignore
   * @return {IndexIterator}
   */
  [Symbol.iterator] () {
    return new IndexIterator(this)
  }
}

/**
 * A base class for a storage provider.
 */
export class Provider {
  #id = state.id
  #index = new Index()
  #ready = new Deferred()

  /**
   * @type {{ error: Error | null }}
   */
  #state = { error: null }

  /**
   * `Provider` class constructor.
   */
  constructor () {
    try {
      const request = this.load()

      if (typeof request.then === 'function') {
        request.then(() => this.#ready.resolve())
      } else {
        queueMicrotask(() => this.#ready.resolve())
      }

      if (typeof request?.catch === 'function') {
        request.catch((err) => {
          this.#state.error = err
          state.reportError(err)
        })
      }
    } catch (err) {
      this.#state.error = err
      state.reportError(err)
    }
  }

  /**
   * An error currently associated with the provider, likely from an
   * async operation.
   * @type {Error?}
   */
  get error () {
    return this.#state.error
  }

  /**
   * A promise that resolves when the provider is ready.
   * @type {Promise}
   */
  get ready () {
    return this.#ready.promise
  }

  /**
   * A reference the service worker storage ID, which is the service worker
   * registration ID.
   * @type {string}
   * @throws DOMException
   */
  get id () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#id
  }

  /**
   * A reference to the provider `Index`
   * @type {Index}
   * @throws DOMException
   */
  get index () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index
  }

  /**
   * The number of entries in the provider.
   * @type {number}
   * @throws DOMException
   */
  get length () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index.length
  }

  /**
   * Returns `true` if the provider has a value for a given `key`.
   * @param {string} key}
   * @return {boolean}
   * @throws DOMException
   */
  has (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index.indexOf(key) >= 0
  }

  /**
   * Get a value by `key`.
   * @param {string} key
   * @return {string?}
   * @throws DOMException
   */
  get (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    const index = this.#index.indexOf(key)
    return this.#index.value(index)
  }

  /**
   * Sets a `value` by `key`
   * @param {string} key
   * @param {string} value
   * @throws DOMException
   */
  set (key, value) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    this.#index.insert(key, value)
  }

  /**
   * Removes a value by `key`.
   * @param {string} key
   * @return {boolean}
   * @throws DOMException
   */
  remove (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    const index = this.#index.indexOf(key)
    return this.#index.remove(index)
  }

  /**
   * Clear all keys and values.
   * @throws DOMException
   */
  clear () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    this.#index.clear()
  }

  /**
   * The keys in the provider index.
   * @return {string[]}
   * @throws DOMException
   */
  keys () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index.keys
  }

  /**
   * The values in the provider index.
   * @return {string[]}
   * @throws DOMException
   */
  values () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index.values
  }

  /**
   * Returns the key at a given `index`
   * @param {number} index
   * @return {string|null}
   * @throws DOMException
   */
  key (index) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    return this.#index.keys[index] ?? null
  }

  /**
   * Loads the internal index with keys and values.
   * @return {Promise}
   */
  async load () {
    // no-op
  }
}

/**
 * An in-memory storage provider. It just used the built-in provider `Index`
 * for storing key-value entries.
 */
export class MemoryStorageProvider extends Provider {}

/**
 * A session storage provider that persists for the runtime of the
 * application and through service worker restarts.
 */
export class SessionStorageProvider extends Provider {
  /**
   * Get a value by `key`.
   * @param {string} key
   * @return {string?}
   * @throws DOMException
   * @throws NotFoundError
   */
  get (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    if (super.has(key)) {
      return super.get(key)
    }

    const { id } = this
    const result = ipc.sendSync('serviceWorker.storage.get', { id, key })

    // @ts-ignore
    if (result.err && result.err.code !== 'NOT_FOUND_ERR') {
      throw result.err
    }

    const value = result.data?.value ?? null

    if (value !== null) {
      super.set(key, value)
    }

    return value
  }

  /**
   * Sets a `value` by `key`
   * @param {string} key
   * @param {string} value
   * @throws DOMException
   * @throws NotFoundError
   */
  set (key, value) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    value = String(value)

    const { id } = this
    // send async
    ipc.send('serviceWorker.storage.set', { id, key, value })
    return super.set(key, value)
  }

  /**
   * Remove a value by `key`.
   * @param {string} key
   * @return {string?}
   * @throws DOMException
   * @throws NotFoundError
   */
  remove (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    const { id } = this
    const result = ipc.sendSync('serviceWorker.storage.remove', { id, key })

    if (result.err) {
      throw result.err
    }

    return super.remove(key)
  }

  /**
   * Clear all keys and values.
   * @throws DOMException
   * @throws NotFoundError
   */
  clear () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    const { id } = this
    const result = ipc.sendSync('serviceWorker.storage.clear', { id })

    if (result.err) {
      throw result.err
    }

    return super.clear()
  }

  /**
   * Loads the internal index with keys and values.
   * @return {Promise}
   * @throws NotFoundError
   */
  async load () {
    const { id } = this
    const result = await ipc.request('serviceWorker.storage', { id })

    if (result.err) {
      throw result.err
    }

    if (result.data && typeof result.data === 'object') {
      for (const key in result.data) {
        const value = result.data[key]
        this.index.insert(key, value)
      }
    }

    return await super.load()
  }
}

/**
 * A local storage provider that persists until the data is cleared.
 */
export class LocalStorageProvider extends Provider {
  /**
   * Get a value by `key`.
   * @param {string} key
   * @return {string?}
   * @throws DOMException
   */
  get (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    // @ts-ignore
    const { localStorage } = Environment.instance.context

    if (localStorage && typeof localStorage === 'object') {
      if (key in localStorage) {
        return localStorage[key]
      }
    }

    const value = super.get(key)

    if (value !== null) {
      super.set(key, value)
    }

    return value
  }

  /**
   * Sets a `value` by `key`
   * @param {string} key
   * @param {string} value
   * @throws DOMException
   */
  set (key, value) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    // @ts-ignore
    const { localStorage } = Environment.instance.context

    if (localStorage) {
      // @ts-ignore
      Environment.instance.context.localStorage = {
        ...localStorage,
        [key]: String(value)
      }
    }

    return super.set(key, value)
  }

  /**
   * Remove a value by `key`.
   * @param {string} key
   * @return {boolean}
   * @throws DOMException
   */
  remove (key) {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    // @ts-ignore
    const { localStorage } = Environment.instance.context

    if (localStorage && typeof localStorage === 'object') {
      if (key in localStorage) {
        delete localStorage[key]
        // @ts-ignore
        Environment.instance.context.localStorage = localStorage
      }
    }

    return super.remove(key)
  }

  /**
   * Clear all keys and values.
   * @throws DOMException
   */
  clear () {
    if (this.error) {
      throw Object.assign(new DOMException(
        'Storage provider in error state', 'InvalidStateError'
      ), { cause: this.error })
    }

    // @ts-ignore
    Environment.instance.context.localStorage = {}
    return super.clear()
  }

  /**
   * Loads the internal index with keys and values.
   * @return {Promise}
   * @throws DOMException
   */
  async load () {
    if (!Environment.instance) {
      throw Object.assign(new DOMException(
        'Storage provider is missing Environment', 'InvalidStateError'
      ), { cause: this.error })
    }

    // ensure `Environment` is opened
    await Environment.instance.open()

    // @ts-ignore
    const localStorage = Environment.instance.context.localStorage || {}

    for (const key in localStorage) {
      const value = localStorage[key]
      this.index.insert(key, value)
    }
  }
}

/**
 * A generic interface for storage implementations
 */
export class Storage {
  /**
   * A factory for creating a `Storage` instance that is backed
   * by a storage provider. Extending classes should define a `Provider`
   * class that is statically available on the extended `Storage` class.
   * @param {symbol} token
   * @return {Promise<Proxy<Storage>>}
   */
  static async create (token) {
    // @ts-ignore
    const { Provider } = this

    if (typeof Provider !== 'function') {
      throw new TypeError('Storage implementation is missing Provider implementation')
    }

    const provider = new Provider()
    const instance = new this(token, provider)

    await provider.ready

    return new Proxy(instance, {
      get (_, property) {
        if (instance.hasItem(property)) {
          return instance.getItem(property)
        }

        if (property in instance) {
          const value = instance[property]
          if (typeof value === 'function') {
            return value.bind(instance)
          }

          return value
        }

        return undefined
      },

      set (_, property, value) {
        if (property in Storage.prototype) {
          return false
        }

        instance.setItem(property, value)
        return true
      },

      has (_, property) {
        return instance.hasItem(property)
      },

      deleteProperty (_, property) {
        if (instance.hasItem(property)) {
          instance.removeItem(property)
          return true
        }

        return false
      },

      getOwnPropertyDescriptor (_, property) {
        if (instance.hasItem(property)) {
          return {
            configurable: true,
            enumerable: true,
            writable: true,
            value: instance.getItem(property)
          }
        }
      },

      ownKeys (_) {
        return Array.from(instance.provider.keys())
      }
    })
  }

  #provider = null

  /**
   * `Storage` class constructor.
   * @ignore
   * @param {symbol} token
   * @param {Provider} provider
   */
  constructor (token, provider) {
    if (token !== createStorageSymbol) {
      throw new IllegalConstructorError('Illegal constructor')
    }

    this.#provider = provider
  }

  /**
   * A readonly reference to the storage provider.
   * @type {Provider}
   */
  get provider () {
    return this.#provider
  }

  /**
   * The number of entries in the storage.
   * @type {number}
   */
  get length () {
    return this.provider.length
  }

  /**
   * Returns `true` if the storage has a value for a given `key`.
   * @param {string} key
   * @return {boolean}
   * @throws TypeError
   */
  hasItem (key) {
    if (arguments.length === 0) {
      throw new TypeError('Not enough arguments')
    }

    return this.provider.has(key)
  }

  /**
   * Clears the storage of all entries
   */
  clear () {
    this.provider.clear()
  }

  /**
   * Returns the key at a given `index`
   * @param {number} index
   * @return {string|null}
   */
  key (index) {
    if (arguments.length === 0) {
      throw new TypeError('Not enough arguments')
    }

    return this.provider.key(index)
  }

  /**
   * Get a storage value item for a given `key`.
   * @param {string} key
   * @return {string|null}
   */
  getItem (key) {
    if (arguments.length === 0) {
      throw new TypeError('Not enough arguments')
    }

    return this.provider.get(key)
  }

  /**
   * Removes a storage value entry for a given `key`.
   * @param {string}
   * @return {boolean}
   */
  removeItem (key) {
    if (arguments.length === 0) {
      throw new TypeError('Not enough arguments')
    }

    this.provider.remove(key)
  }

  /**
   * Sets a storage item `value` for a given `key`.
   * @param {string} key
   * @param {string} value
   */
  setItem (key, value) {
    if (arguments.length === 0) {
      throw new TypeError('Not enough arguments')
    }

    this.provider.set(key, value)
  }

  /**
   * @ignore
   */
  get [Symbol.toStringTag] () {
    return 'Storage'
  }
}

/**
 * An in-memory `Storage` interface.
 */
export class MemoryStorage extends Storage {
  static Provider = MemoryStorageProvider
}

/**
 * A locally persisted `Storage` interface.
 */
export class LocalStorage extends Storage {
  static Provider = LocalStorageProvider
}

/**
 * A session `Storage` interface.
 */
export class SessionStorage extends Storage {
  static Provider = SessionStorageProvider
}

/**
 * A factory for creating storage interfaces.
 * @param {'memoryStorage'|'localStorage'|'sessionStorage'} type
 * @return {Promise<Storage>}
 */
export async function createStorageInterface (type) {
  if (type === 'memoryStorage') {
    return await MemoryStorage.create(createStorageSymbol)
  } else if (type === 'localStorage') {
    return await LocalStorage.create(createStorageSymbol)
  } else if (type === 'sessionStorage') {
    return await SessionStorage.create(createStorageSymbol)
  }

  throw new TypeError(
    `Invalid 'Storage' interface type given: Received: ${type}`
  )
}

export default {
  Storage,
  LocalStorage,
  MemoryStorage,
  SessionStorage,
  createStorageInterface
}
