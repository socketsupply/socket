import {
  getCurrentExecutionAsyncResource,
  getNextAsyncResourceId
} from './internal/async.js'

// import gc from './gc.js'

const storages = []

/**
 * @typedef {{
 *   triggerAsyncId?: number,
 *   requireManualDestroy?: boolean
 * }} AsyncResourceOptions
 */

/**
 * The current execution async resource ID
 * @return {number}
 */
export function executionAsyncId () {
  return getCurrentExecutionAsyncResource().id
}

/**
 * TODO
 */
export class AsyncResource {
  #type = null
  #asyncId = getNextAsyncResourceId()
  #triggerAsyncId = executionAsyncId()
  #requireManualDestroy = false

  /**
   * `AsyncResource` class constructor.
   * @param {string} type
   * @param {AsyncResourceOptions|number=} [options]
   */
  constructor (type, options = null) {
    if (!type || typeof type !== 'string') {
      throw new TypeError(
        `Expecting 'type' to be a string. Received: ${type}`
      )
    }

    if (typeof options === 'number') {
      options = { triggerAsyncId: options }
    }

    this.#type = type

    if (Number.isFinite(options?.triggerAsyncId) && options.triggerAsyncId > 0) {
      this.#triggerAsyncId = options.triggerAsyncId
    } else {
      throw new TypeError(
        // eslint-disable-next-line
        `Expecting 'options.triggerAsyncId' to be a positive number.` +
        `Received: ${options.triggerAsyncId}`
      )
    }

    if (typeof options?.requireManualDestroy === 'boolean') {
      this.#requireManualDestroy = options.requireManualDestroy
    } else {
      throw new TypeError(
        // eslint-disable-next-line
        `Expecting 'options.requireManualDestroy' to be a boolean.` +
        `Received: ${options.requireManualDestroy}`
      )
    }
  }

  /**
   * @type {string}
   */
  get type () {
    return this.#type
  }

  /**
   * @return {number}
   */
  asyncId () {
    return this.#asyncId
  }

  /**
   * @return {number}
   */
  triggerAsyncId () {
    return this.#triggerAsyncId
  }

  runInAsyncScope (callback, thisArg, ...args) {
    try {
      return Reflect.apply(callback, thisArg, args)
    } finally {
      //
    }
  }
}

/**
 * TODO
 */
export class AsyncLocalStorage {
  #store = null
  #enabled = false

  /**
   * TODO
   */
  static bind (fn) {
    return AsyncResource.bind(fn)
  }

  /**
   * TODO
   */
  static snapshot () {
    // eslint-disable-next-line
    return AsyncLocalStorage.bind((cb, ...args) => cb(...args))
  }

  /**
   * @type {boolean}
   */
  get enabled () {
    return this.#enabled
  }

  /**
   * TODO
   */
  disable () {
    if (this.#enabled) {
      this.#enabled = false
      const index = storages.indexOf(this)
      if (index !== -1) {
        storages.splice(index, 1)
      }
    }
  }

  /**
   * TODO
   */
  enable () {
    if (!this.#enabled) {
      this.#enabled = true
      storages.push(this)
    }
  }

  enterWith (store) {
    this.enable()
  }

  run (store, callback, ...args) {
  }

  exit (callback, ...args) {
  }

  getStore () {
  }
}

export default {
  AsyncLocalStorage
}
