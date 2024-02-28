/**
 * A utility class for creating deferred promises.
 */
export class Deferred {
  /**
   * Creates a new Deferred instance.
   */
  constructor () {
    /**
     * The promise associated with this Deferred instance.
     * @type {Promise<any>}
     * @private
     */
    this._promise = new Promise((resolve, reject) => {
      /**
       * Function to resolve the associated promise.
       * @type {function}
       */
      this.resolve = resolve
      /**
       * Function to reject the associated promise.
       * @type {function}
       */
      this.reject = reject
    })
    /**
     * Attaches a fulfillment callback and a rejection callback to the promise,
     * and returns a new promise resolving to the return value of the called
     * callback.
     * @type {function}
     */
    this.then = this._promise.then.bind(this._promise)
    /**
     * Attaches a rejection callback to the promise, and returns a new promise
     * resolving to the return value of the callback if it is called, or to its
     * original fulfillment value if the promise is instead fulfilled.
     * @type {function}
     */
    this.catch = this._promise.catch.bind(this._promise)
    /**
     * Attaches a callback for when the promise is settled (fulfilled or rejected).
     * @type {function}
     */
    this.finally = this._promise.finally.bind(this._promise)
    /**
     * A string representation of this Deferred instance.
     * @type {string}
     * @ignore
     */
    this[Symbol.toStringTag] = 'Promise'
  }
}

/**
 * Exports the Deferred class.
 */
export default {
  Deferred
}
