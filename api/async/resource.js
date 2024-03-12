/**
 * @module AsyncContext
 *
 * Primitives for creating user defined async resources that implement
 * async hooks.
 */
import {
  getDefaultExecutionAsyncId,
  getNextAsyncResourceId,
  asyncContextVariable
} from '../internal/async/hooks.js'

/**
 * @typedef {{
 *   triggerAsyncId?: number,
 *   requireManualDestroy?: boolean
 * }} AsyncResourceOptions
 */

/**
 * TODO
 */
export class AsyncResource {
  /**
   * TODO
   */
  static bind (fn, type, thisArg) {
    type = type || fn.name || 'bound-anonymous-function'
    const resource = new AsyncResource(type)
    return resource.bind(fn, thisArg)
  }

  #type = null
  #asyncId = getNextAsyncResourceId()
  #triggerAsyncId = getDefaultExecutionAsyncId()
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
    } else if (options?.triggerAsyncId !== undefined) {
      throw new TypeError(
        // eslint-disable-next-line
        `Expecting 'options.triggerAsyncId' to be a positive number.` +
        `Received: ${options.triggerAsyncId}`
      )
    }

    if (typeof options?.requireManualDestroy === 'boolean') {
      this.#requireManualDestroy = options.requireManualDestroy
    } else if (options?.requireManualDestroy !== undefined) {
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

  /**
   */
  bind (fn, thisArg) {
    const resource = this
    let binding = null
    if (thisArg === undefined) {
      binding = function (...args) {
        args.unshift(fn, this)
        return Reflect.apply(resource.runInAsyncScope, resource, args)
      }
    } else {
      binding = resource.runInAsyncScope.bind(this, fn, thisArg)
    }

    Object.defineProperty(binding, 'length', {
      __proto__: null,
      configurable: true,
      enumerable: false,
      writable: false,
      value: fn.length
    })

    return binding
  }

  runInAsyncScope (callback, thisArg, ...args) {
    return asyncContextVariable.run(this, callback.bind(thisArg), ...args)
  }
}

export default AsyncResource
