import { ErrorEvent } from '../events.js'

/**
 * Dispatched when a `Deferred` internal promise is resolved.
 */
export class DeferredResolveEvent extends Event {
  /**
   * The `Deferred` promise result value.
   * @type {any?}
   */
  result = null

  /**
   * `DeferredResolveEvent` class constructor
   * @ignore
   * @param {string=} [type]
   * @param {any=} [result]
   */
  constructor (type = 'resolve', result = null) {
    super(type)
    this.result = result
  }
}

/**
 * Dispatched when a `Deferred` internal promise is rejected.
 */
export class DeferredRejectEvent extends ErrorEvent {
  /**
   * `DeferredRejectEvent` class constructor
   * @ignore
   * @param {string=} [type]
   * @param {Error=} [error]
   */
  constructor (type = 'reject', error = null) {
    super(type, { error })
  }
}

/**
 * A utility class for creating deferred promises.
 */
export class Deferred extends EventTarget {
  /**
   * @type {Promise?}
   */
  #promise = null

  /**
   * Function to resolve the associated promise.
   * @type {function}
   */
  resolve = null

  /**
   * Function to reject the associated promise.
   * @type {function}
   */
  reject = null

  /**
   * `Deferred` class constructor.
   * @param {Deferred|Promise?} [promise]
   */
  constructor (promise) {
    super()

    this.#promise = new Promise((resolve, reject) => {
      this.resolve = (value) => {
        try {
          resolve(value)
          return this.promise
        } finally {
          this.dispatchEvent(new DeferredResolveEvent('resolve', value))
        }
      }

      this.reject = (error) => {
        try {
          reject(error)
          return this.promise
        } finally {
          this.dispatchEvent(new DeferredRejectEvent('reject', error))
        }
      }
    })

    if (typeof promise?.then === 'function') {
      const p = this.#promise
      this.#promise = promise.then(() => p)
    }

    this.then = this.then.bind(this)
    this.catch = this.catch.bind(this)
    this.finally = this.finally.bind(this)
  }

  /**
   * A string representation of this Deferred instance.
   * @type {string}
   * @ignore
   */
  get [Symbol.toStringTag] () {
    return 'Promise'
  }

  /**
   * The promise associated with this Deferred instance.
   * @type {Promise<any>}
   */
  get promise () {
    return this.#promise
  }

  /**
   * Attaches a fulfillment callback and a rejection callback to the promise,
   * and returns a new promise resolving to the return value of the called
   * callback.
   * @param {function(any)=} [resolve]
   * @param {function(Error)=} [reject]
   */
  then (resolve, reject) {
    if (resolve && reject) {
      return this.promise.then(resolve, reject)
    } else if (resolve) {
      return this.promise.then(resolve)
    } else {
      return this.promise.then()
    }
  }

  /**
   * Attaches a rejection callback to the promise, and returns a new promise
   * resolving to the return value of the callback if it is called, or to its
   * original fulfillment value if the promise is instead fulfilled.
   * @param {function(Error)=} [callback]
   */
  catch (callback) {
    return this.promise.catch(callback)
  }

  /**
   * Attaches a callback for when the promise is settled (fulfilled or rejected).
   * @param {function(any?)} [callback]
   */
  finally (callback) {
    return this.promise.finally(callback)
  }
}

export default Deferred
