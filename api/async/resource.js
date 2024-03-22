/**
 * @module AsyncContext
 *
 * Primitives for creating user defined async resources that implement
 * async hooks.
 */
import {
  CoreAsyncResource,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId
} from '../internal/async/hooks.js'

export {
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId
}

/**
 * @typedef {{
 *   triggerAsyncId?: number,
 *   requireManualDestroy?: boolean
 * }} AsyncResourceOptions
 */

/**
 * A container that should be extended that represents a resource with
 * an asynchronous execution context.
 */
export class AsyncResource extends CoreAsyncResource {
  /**
   * Binds function `fn` with an optional this `thisArg` binding to run
   * in the execution context of an anonymous `AsyncResource`.
   * @param {function} fn
   * @param {object|string=} [type]
   * @param {object=} [thisArg]
   * @return {function}
   */
  static bind (fn, type, thisArg) {
    if (typeof type === 'object') {
      thisArg = type
      type = fn.name
    }

    type = type || fn.name || 'bound-anonymous-function'
    const resource = new AsyncResource(type)
    return resource.bind(fn, thisArg)
  }

  /**
   * `AsyncResource` class constructor.
   * @param {string} type
   * @param {AsyncResourceOptions|number=} [options]
   */
  constructor (type, options = null) {
    super(type, options)
  }

  /**
   * The `AsyncResource` type.
   * @type {string}
   */
  get type () {
    return super.type
  }

  /**
   *`true` if the `AsyncResource` was destroyed, otherwise `false`. This
   * value is only set to `true` if `emitDestroy()` was called, likely from
   * d
   * @type {boolean}
   * @ignore
   */
  get destroyed () {
    return super.destroyed
  }

  /**
   * The unique async resource ID.
   * @return {number}
   */
  asyncId () {
    return super.asyncId()
  }

  /**
   * The trigger async resource ID.
   * @return {number}
   */
  triggerAsyncId () {
    return super.triggerAsyncId()
  }

  /**
   * Manually emits destroy hook for the resource.
   * @return {AsyncResource}
   */
  emitDestroy () {
    return super.emitDestroy()
  }

  /**
   * Binds function `fn` with an optional this `thisArg` binding to run
   * in the execution context of this `AsyncResource`.
   * @param {function} fn
   * @param {object=} [thisArg]
   * @return {function}
   */
  bind (fn, thisArg = undefined) {
    return super.bind(fn, thisArg)
  }

  /**
   * Runs function `fn` in the execution context of this `AsyncResource`.
   * @param {function} fn
   * @param {object=} [thisArg]
   * @param {...any} [args]
   * @return {any}
   */
  runInAsyncScope (fn, thisArg, ...args) {
    return super.runInAsyncScope(fn, thisArg, ...args)
  }
}

export default AsyncResource
