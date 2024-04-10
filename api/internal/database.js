/* global ErrorEvent, IDBTransaction */
import gc from '../gc.js'

/**
 * A typed container for optional options given to the `Database`
 * class constructor.
 *
 * @typedef {{
 *   version?: string | undefined
 * }} DatabaseOptions
 */

/**
 * A typed container for various optional options made to a `get()` function
 * on a `Database` instance.
 *
 * @typedef {{
 *   store?: string | undefined,
 *   stores?: string[] | undefined,
 *   count?: number | undefined
 * }} DatabaseGetOptions
 */

/**
 * A typed container for various optional options made to a `put()` function
 * on a `Database` instance.
 *
 * @typedef {{
 *   store?: string | undefined,
 *   stores?: string[] | undefined,
 *   durability?: 'strict' | 'relaxed' | undefined
 * }} DatabasePutOptions
 */

/**
 * A typed container for various optional options made to a `delete()` function
 * on a `Database` instance.
 *
 * @typedef {{
 *   store?: string | undefined,
 *   stores?: string[] | undefined
 * }} DatabaseDeleteOptions
 */

/**
 * A typed container for optional options given to the `Database`
 * class constructor.
 *
 * @typedef {{
 *   offset?: number | undefined,
 *   backlog?: number | undefined
 * }} DatabaseRequestQueueWaitOptions
 */

/**
 * A typed container for various optional options made to a `entries()` function
 * on a `Database` instance.
 *
 * @typedef {{
 *   store?: string | undefined,
 *   stores?: string[] | undefined
 * }} DatabaseEntriesOptions
 */

/**
 * A `DatabaseRequestQueueRequestConflict` callback function type.
 * @typedef {function(Event, DatabaseRequestQueueRequestConflict): any} DatabaseRequestQueueConflictResolutionCallback
 */

/**
 * Waits for an event of `eventType` to be dispatched on a given `EventTarget`.
 * @param {EventTarget} target
 * @param {string} eventType
 * @return {Promise<Event>}
 */
export async function waitFor (target, eventType) {
  return await new Promise((resolve) => {
    target.addEventListener(eventType, resolve, { once: true })
  })
}

/**
 * A mapping of named `Database` instances that are currently opened
 * @type {Map<string, WeakRef<Database>>}
 */
export const opened = new Map()

/**
 * A container for conflict resolution for a `DatabaseRequestQueue` instance
 * `IDBRequest` instance.
 */
export class DatabaseRequestQueueRequestConflict {
  /**
   * Internal conflict resolver callback.
   * @type {function(any): void)}
   */
  #resolve = null

  /**
   * Internal conflict rejection callback.
   * @type {function(Error): void)}
   */
  #reject = null

  /**
   * Internal conflict cleanup callback.
   * @type {function(): void)}
   */
  #cleanup = null

  /**
   * `DatabaseRequestQueueRequestConflict` class constructor
   * @param {function(any): void)} resolve
   * @param {function(Error): void)} reject
   * @param {function(): void)} cleanup
   */
  constructor (resolve, reject, cleanup) {
    this.#resolve = resolve
    this.#reject = reject
    this.#cleanup = cleanup
  }

  /**
   * Called when a conflict is resolved.
   * @param {any} argument
   */
  resolve (argument = undefined) {
    if (typeof this.#resolve === 'function') {
      this.#resolve(argument)
    }

    if (typeof this.#cleanup === 'function') {
      this.#cleanup()
    }

    this.#reject = null
    this.#resolve = null
    this.#cleanup = null
  }

  /**
   * Called when a conflict is rejected
   * @param {Error} error
   */
  reject (error) {
    if (typeof this.#reject === 'function') {
      this.#reject(error)
    }

    if (typeof this.#cleanup === 'function') {
      this.#cleanup()
    }

    this.#reject = null
    this.#resolve = null
    this.#cleanup = null
  }
}

/**
 * An event dispatched on a `DatabaseRequestQueue`
 */
export class DatabaseRequestQueueEvent extends Event {
  #request = null

  /**
   * `DatabaseRequestQueueEvent` class constructor.
   * @param {string} type
   * @param {IDBRequest|IDBTransaction} request
   */
  constructor (type, request) {
    super(type)
    this.#request = request
  }

  /**
   * A reference to the underlying request for this event.
   * @type {IDBRequest|IDBTransaction}
   */
  get request () {
    return this.#request
  }
}

/**
 * An event dispatched on a `Database`
 */
export class DatabaseEvent extends Event {
  /**
   * An internal reference to the underlying database for this event.
   * @type {Database}
   */
  #database = null

  /**
   * `DatabaseEvent` class constructor.
   * @param {string} type
   * @param {Database} database
   */
  constructor (type, database) {
    super(type)
    this.#database = database
  }

  /**
   * A reference to the underlying database for this event.
   * @type {Database}
   */
  get database () {
    return this.#database
  }
}

/**
 * An error event dispatched on a `DatabaseRequestQueue`
 */
export class DatabaseRequestQueueErrorEvent extends ErrorEvent {
  #request = null

  /**
   * `DatabaseRequestQueueErrorEvent` class constructor.
   * @param {string} type
   * @param {IDBRequest|IDBTransaction} request
   * @param {{ error: Error, cause?: Error }} options
   */
  constructor (type, request, options) {
    super(type, options)
    this.#request = request
  }

  /**
   * A reference to the underlying request for this error event.
   * @type {IDBRequest|IDBTransaction}
   */
  get request () {
    return this.#request
  }
}

/**
 * A container for various `IDBRequest` and `IDBTransaction` instances
 * occurring during the life cycles of a `Database` instance.
 */
export class DatabaseRequestQueue extends EventTarget {
  #queue = []
  #promises = []

  /**
   * Computed queue length
   * @type {number}
   */
  get length () {
    return this.#queue.length
  }

  /**
   * Pushes an `IDBRequest` or `IDBTransaction onto the queue and returns a
   * `Promise` that resolves upon a 'success' or 'complete' event and rejects
   * upon an error' event.
   * @param {IDBRequest|IDBTransaction}
   * @param {?DatabaseRequestQueueConflictResolutionCallback} [conflictResolutionCallback]
   * @return {Promise}
   */
  async push (request, conflictResolutionCallback = null) {
    const promises = this.#promises
    const queue = this.#queue

    this.#queue.push(request)

    const promise = new Promise((resolve, reject) => {
      if (typeof conflictResolutionCallback === 'function') {
        request.addEventListener('upgradeneeded', onupgradeneeded, { once: true })
        request.addEventListener('blocked', onblocked, { once: true })
      }

      request.addEventListener('complete', oncomplete, { once: true })
      request.addEventListener('success', onsuccess, { once: true })
      request.addEventListener('error', onerror, { once: true })
      request.addEventListener('abort', onabort, { once: true })

      function cleanup () {
        const queueIndex = queue.indexOf(request)
        const promiseIndex = promises.indexOf(promise)

        if (queueIndex >= 0) {
          queue.splice(queueIndex, 1)
        }

        if (promiseIndex >= 0) {
          promises.splice(promiseIndex, 1)
        }

        request.removeEventListener('upgradeneeded', onupgradeneeded, { once: true })
        request.removeEventListener('blocked', onblocked, { once: true })

        request.removeEventListener('complete', oncomplete, { once: true })
        request.removeEventListener('success', onsuccess, { once: true })
        request.removeEventListener('error', onerror, { once: true })
        request.removeEventListener('abort', onabort, { once: true })
      }

      function oncomplete () {
        cleanup()
        resolve(request.result ?? request)
      }

      function onsuccess () {
        cleanup()
        resolve(request.result ?? null)
      }

      function onerror (event) {
        const message = 'Database: The request failed. A cause is attached'
        const error = new Error(message, { cause: request.error ?? event.target })

        error.request = request

        if (request.source) {
          error.source = request.source
        }

        if (request instanceof IDBTransaction) {
          error.transaction = request
        } else {
          error.transaction = request.transaction
        }

        cleanup()
        reject(error)
      }

      function onabort () {
        const message = 'Database: The request was aborted'
        const error = new Error(message)

        error.request = request

        if (request.source) {
          error.source = request.source
        }

        if (request instanceof IDBTransaction) {
          error.transaction = request
        }

        cleanup()
        reject(error)
      }

      function onupgradeneeded (event) {
        const conflict = new DatabaseRequestQueueRequestConflict(resolve, reject, cleanup)
        conflictResolutionCallback(event, conflict)
      }

      function onblocked (event) {
        const conflict = new DatabaseRequestQueueRequestConflict(resolve, reject, cleanup)
        conflictResolutionCallback(event, conflict)
      }
    })

    this.#promises.push(promise)
    promise.then(() => {
      const event = new DatabaseRequestQueueEvent('resolve', request)
      this.dispatchEvent(event)
    })

    promise.catch((error) => {
      const event = new DatabaseRequestQueueErrorEvent('error', request, {
        error
      })

      this.dispatchEvent(event)
    })

    // await in tail for call stack inclusion
    return await promise
  }

  /**
   * Waits for all pending requests to complete. This function will throw when
   * an `IDBRequest` or `IDBTransaction` instance emits an 'error' event.
   * Callers of this function can optionally specify a maximum backlog to wait
   * for instead of waiting for all requests to finish.
   * @param {?DatabaseRequestQueueWaitOptions | undefined} [options]
   */
  async wait (options = null) {
    if (this.#queue.length === 0) {
      return
    }

    const backlog = Number.isFinite(options?.backlog)
      ? Math.min(options.backlog, this.#queue.length)
      : this.#queue.length

    const offset = Number.isFinite(options?.offset)
      ? Math.min(options.offset, this.#queue.length - 1)
      : 0

    const pending = []

    for (let i = offset; i < backlog; ++i) {
      const promise = this.promises[i]
      if (promise instanceof Promise) {
        pending.push(promise)
      }
    }

    return await Promise.all(pending)
  }
}

/**
 * An interface for reading from named databases backed by IndexedDB.
 */
export class Database extends EventTarget {
  /**
   * A reference to an opened `IDBDatabase` from an `IDBOpenDBRequest`
   * @type {?IDBDatabase}
   */
  #storage = null

  /**
   * An optional version of the database.
   * @type {?string}
   */
  #version = null

  /**
   * The name of the database
   * @param {?string}
   */
  #name = null

  /**
   * An internal queue of `Database` pending `IDBRequest` operations.
   * @type {DatabaseRequestQueue}
   */
  #queue = new DatabaseRequestQueue()

  /**
   * This value is `true` if a pending `IDBOpenDBRequest` is present in the
   * request queue.
   * @type {boolean}
   */
  #opening = false

  /**
   * This value is `true` if the `Database` instance `close()` function was called,
   * but the 'close' event has not been dispatched.
   * @type {boolean}
   */
  #closing = false

  /**
   * `Database` class constructor.
   * @param {string} name
   * @param {?DatabaseOptions | undefined} [options]
   */
  constructor (name, options = null) {
    if (!name || typeof name !== 'string') {
      throw new TypeError(
        `Database: Expecting 'name' to be a string. Received: ${name}`
      )
    }

    super()

    this.#name = name

    if (options?.version && typeof options?.version === 'string') {
      this.#version = options.version
    }
  }

  /**
   * `true` if the `Database` is currently opening, otherwise `false`.
   * A `Database` instance should not attempt to be opened if this property value
   * is `true`.
   * @type {boolean}
   */
  get opening () {
    return this.#opening
  }

  /**
   * `true` if the `Database` instance was successfully opened such that the
   * internal `IDBDatabase` storage instance was created and can be referenced
   * on the `Database` instance, otherwise `false`.
   * @type {boolean}
   */
  get opened () {
    return this.#storage !== null
  }

  /**
   * `true` if the `Database` instance was closed or has not been opened such
   * that the internal `IDBDatabase` storage instance was not created or cannot
   * be referenced on the `Database` instance, otherwise `false`.
   * @type {boolean}
   */
  get closed () {
    return !this.closing && this.#storage === null
  }

  /**
   * `true` if the `Database` is currently closing, otherwise `false`.
   * A `Database` instance should not attempt to be closed if this property value
   * is `true`.
   * @type {boolean}
   */
  get closing () {
    return this.#closing
  }

  /**
   * The name of the `IDBDatabase` database. This value cannot be `null`.
   * @type {string}
   */
  get name () {
    return this.#name
  }

  /**
   * The version of the `IDBDatabase` database. This value may be `null`.
   * @type {?string}
   */
  get version () {
    return this.#version
  }

  /**
   * A reference to the `IDBDatabase`, if the `Database` instance was opened.
   * This value may ba `null`.
   * @type {?IDBDatabase}
   */
  get storage () {
    return this.#storage
  }

  /**
   * Opens the `IDBDatabase` database optionally at a specific "version" if
   * one was given upon construction of the `Database` instance. This function
   * is not idempotent and will throw if the underlying `IDBDatabase` instance
   * was created successfully or is in the process of opening.
   * @return {Promise}
   */
  async open () {
    const queue = this.#queue

    if (this.opened) {
      throw new Error('Database: storage is already opened')
    }

    if (this.opening) {
      throw new Error('Database: storage is already opening')
    }

    this.#opening = true
    const request = this.#version !== null
      ? globalThis.indexedDB.open(this.name, this.#version)
      : globalThis.indexedDB.open(this.name)

    this.#storage = await queue.push(request, async (conflictEvent, conflict) => {
      if (conflictEvent.type === 'blocked') {
        // TODO(@jwerle): handle a 'blocked' event
        conflict.reject(new Error('Database: storage cannot be opened (blocked)'))
      } else if (conflictEvent.type === 'upgradeneeded') {
        const storage = conflictEvent.target.result
        const store = storage.createObjectStore(this.name, { keyPath: 'key' })
        await queue.push(store.transaction)
        conflict.resolve(storage)
      } else {
        conflict.reject(new Error('Database: storage cannot be opened (unknown reason)'))
      }
    })

    this.#opening = false
    gc.ref(this)

    queueMicrotask(() => {
      this.dispatchEvent(new DatabaseEvent('open', this))
    })
  }

  /**
   * Closes the `IDBDatabase` database storage, if opened. This function is not
   * idempotent and will throw if the underlying `IDBDatabase` instance is
   * already closed (not opened) or currently closing.
   * @return {Promise}
   */
  async close () {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    const storage = this.#storage
    this.#closing = true
    this.#storage = null
    await storage.close()
    gc.unref(this)
    this.#closing = false
    this.dispatchEvent(new DatabaseEvent('close', this))
  }

  /**
   * Deletes entire `Database` instance and closes after successfully
   * delete storage.
   */
  async drop () {
    if (this.opening) {
      throw new Error('Database: storage is still opening')
    }

    if (this.opened) {
      await this.close()
    }

    await this.#queue.push(globalThis.indexedDB.deleteDatabase(this.name))
  }

  /**
   * Gets a "readonly" value by `key` in the `Database` object storage.
   * @param {string} key
   * @param {?DatabaseGetOptions|undefined} [options]
   * @return {Promise<object|object[]|null>}
   */
  async get (key, options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readonly'
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    const count = Number.isFinite(options?.count) && options.count > 0
      ? options.count
      : 1

    for (const store of stores) {
      if (count > 1) {
        pending.push(this.#queue.push(store.getAll(key, count)))
      } else {
        pending.push(this.#queue.push(store.get(key)))
      }
    }

    await this.#queue.push(transaction)
    const results = await Promise.all(pending)

    if (count === 1 || options?.store) {
      const [result] = results
      return result?.value ?? null
    }

    return results
      .map(function map (result) {
        return Array.isArray(result) ? result.flatMap(map) : result
      })
      .reduce((a, b) => a.concat(b), [])
      .filter((value) => value)
      .map((entry) => {
        return [entry.key, entry.value]
      })
  }

  /**
   * Put a `value` at `key`, updating if it already exists, otherwise
   * "inserting" it into the `Database` instance.
   * @param {string} key
   * @param {any} value
   * @param {?DatabasePutOptions|undefined} [options]
   * @return {Promise}
   */
  async put (key, value, options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readwrite',
      { durability: options?.durability ?? 'strict' }
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    for (const store of stores) {
      pending.push(this.#queue.push(store.put({ key, value })))
    }

    transaction.commit()
    await this.#queue.push(transaction)
    await Promise.all(pending)
  }

  /**
   * Inserts a new `value` at `key`. This function throws if a value at `key`
   * already exists.
   * @param {string} key
   * @param {any} value
   * @param {?DatabasePutOptions|undefined} [options]
   * @return {Promise}
   */
  async insert (key, value, options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readwrite',
      { durability: options?.durability ?? 'strict' }
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    for (const store of stores) {
      pending.push(this.#queue.push(store.add(value, key)))
    }

    transaction.commit()
    await this.#queue.push(transaction)
    await Promise.all(pending)
  }

  /**
   * Update a `value` at `key`, updating if it already exists, otherwise
   * "inserting" it into the `Database` instance.
   * @param {string} key
   * @param {any} value
   * @param {?DatabasePutOptions|undefined} [options]
   * @return {Promise}
   */
  async update (key, value, options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readwrite',
      { durability: 'strict' }
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    for (const store of stores) {
      let existing = null
      try {
        existing = await this.#queue.push(store.get(key))
      } catch (err) {
        globalThis.reportError(err)
      }

      if (existing) {
        pending.push(this.#queue.push(store.put({
          key,
          value: { ...(existing.value ?? null), ...value }
        })))
      } else {
        pending.push(this.#queue.push(store.put({ key, value })))
      }
    }

    transaction.commit()
    await this.#queue.push(transaction)
    await Promise.all(pending)
  }

  /**
   * Delete a value at `key`.
   * @param {string} key
   * @param {?DatabaseDeleteOptions|undefined} [options]
   * @return {Promise}
   */
  async delete (key, options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readwrite',
      { durability: 'strict' }
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    for (const store of stores) {
      pending.push(this.#queue.push(store.delete(key)))
    }

    transaction.commit()
    await this.#queue.push(transaction)
    await Promise.all(pending)
  }

  /**
   * Gets a "readonly" value by `key` in the `Database` object storage.
   * @param {string} key
   * @param {?DatabaseEntriesOptions|undefined} [options]
   * @return {Promise<object|object[]|null>}
   */
  async entries (options = null) {
    if (!this.opened) {
      throw new Error('Database: storage is not opened')
    }

    if (this.closing) {
      throw new Error('Database: storage is already closing')
    }

    if (this.opening) {
      await waitFor(this, 'open')
    }

    const stores = []
    const pending = []
    const transaction = this.#storage.transaction(
      options?.store ?? options?.stores ?? this.name,
      'readonly'
    )

    if (options?.store) {
      stores.push(transaction.objectStore(options.store))
    } else if (options?.stores) {
      for (const store of options.stores) {
        stores.push(transaction.objectStore(store))
      }
    } else {
      stores.push(transaction.objectStore(this.name))
    }

    for (const store of stores) {
      const request = store.openCursor()
      let cursor = await this.#queue.push(request)

      while (cursor) {
        if (!cursor.value) {
          break
        }
        pending.push(Promise.resolve(cursor.value))
        cursor.continue()
        cursor = await this.#queue.push(request)
      }
    }

    await this.#queue.push(transaction)
    const results = await Promise.all(pending)

    return results
      .filter((value) => value)
      .map((entry) => [entry.key, entry.value])
  }

  /**
   * Implements `gc.finalizer` for gc'd resource cleanup.
   * @return {gc.Finalizer}
   */
  [gc.finalizer] () {
    return {
      args: [this.#name, this.#storage],
      handle (name, storage) {
        opened.delete(name)
        storage.close()
      }
    }
  }
}

/**
 * Creates an opens a named `Database` instance.
 * @param {string} name
 * @param {?DatabaseOptions | undefined} [options]
 * @return {Promise<Database>}
 */
export async function open (name, options) {
  const ref = opened.get(name)

  // return already opened instance if still referenced somehow
  if (ref && ref.deref()) {
    const database = ref.deref()
    if (database.opened) {
      return database
    }

    return new Promise((resolve) => {
      database.addEventListener('open', () => {
        resolve(database)
      }, { once: true })
    })
  }

  const database = new Database(name, options)

  opened.set(name, new WeakRef(database))

  database.addEventListener('close', () => {
    opened.delete(name)
  }, { once: true })

  try {
    await database.open()
  } catch (err) {
    opened.delete(name)
    throw err
  }

  return database
}

/**
 * Complete deletes a named `Database` instance.
 * @param {string} name
 * @param {?DatabaseOptions|undefined} [options]
 */
export async function drop (name, options) {
  const ref = opened.get(name)
  const database = ref?.deref?.() ?? new Database(name, options)
  await database.drop()
  opened.delete(name)
}

export default {
  Database,
  open,
  drop
}
