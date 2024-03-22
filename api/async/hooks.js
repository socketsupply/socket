/**
 * @module AsyncHooks
 *
 * Primitives for hooks into async lifecycles such as `queueMicrotask`,
 * `setTimeout`, `setInterval`, and `Promise` built-ins as well as user defined
 * async resources.
 */
import {
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  hooks
} from '../internal/async/hooks.js'

export {
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId
}

/**
 * A container for `AsyncHooks` callbacks.
 * @ignore
 */
export class AsyncHookCallbacks {
  /**
   * `AsyncHookCallbacks` class constructor.
   * @ignore
   * @param {AsyncHookCallbackOptions} [options]
   */
  constructor (options = null) {
    if (typeof options?.init === 'function') {
      this.init = options.init
    }

    if (typeof options?.before === 'function') {
      this.before = options.before
    }

    if (typeof options?.after === 'function') {
      this.after = options.after
    }

    if (typeof options?.destroy === 'function') {
      this.destroy = options.destroy
    }
  }

  init (asyncId, type, triggerAsyncId, resource) {
    // noop
  }

  before (asyncId) {
    // noop
  }

  after (asyncId) {
    // noop
  }

  destroy (asyncId) {
    // noop
  }

  promiseResolve (asyncId) {
    // noop
  }
}

/**
 * A container for registering various callbacks for async resource hooks.
 */
export class AsyncHook {
  /**
   * @type {AsyncHookCallbacks}
   */
  #callbacks

  /**
   * @type {boolean}
   */
  #enabled = false

  /**
   * @param {AsyncHookCallbackOptions|AsyncHookCallbacks=} [options]
   */
  constructor (callbacks = null) {
    this.#callbacks = new AsyncHookCallbacks(callbacks)
  }

  /**
   * @type {boolean}
   */
  get enabled () {
    return this.#enabled
  }

  /**
   * Enable the async hook.
   * @return {AsyncHook}
   */
  enable () {
    if (this.#enabled) {
      return this
    }

    const { init, before, after, destroy, promiseResolve } = this.#callbacks

    if (!hooks.init.includes(init)) {
      hooks.init.push(init)
    }

    if (!hooks.before.includes(before)) {
      hooks.before.push(before)
    }

    if (!hooks.after.includes(after)) {
      hooks.after.push(after)
    }

    if (!hooks.destroy.includes(destroy)) {
      hooks.destroy.push(destroy)
    }

    if (!hooks.promiseResolve.includes(promiseResolve)) {
      hooks.promiseResolve.push(promiseResolve)
    }

    this.#enabled = true
    return this
  }

  /**
   * Disables the async hook
   * @return {AsyncHook}
   */
  disable () {
    if (!this.#enabled) {
      return this
    }

    const { init, before, after, destroy, promiseResolve } = this.#callbacks

    if (hooks.init.includes(init)) {
      hooks.init.splice(hooks.init.indexOf(init), 1)
    }

    if (hooks.before.includes(before)) {
      hooks.before.splice(hooks.before.indexOf(before), 1)
    }

    if (hooks.after.includes(after)) {
      hooks.after.splice(hooks.after.indexOf(after), 1)
    }

    if (hooks.destroy.includes(destroy)) {
      hooks.destroy.splice(hooks.destroy.indexOf(destroy), 1)
    }

    if (hooks.promiseResolve.includes(promiseResolve)) {
      hooks.promiseResolve.splice(hooks.promiseResolve.indexOf(promiseResolve), 1)
    }

    this.#enabled = false
    return this
  }
}

/**
 * Factory for creating a `AsyncHook` instance.
 * @param {AsyncHookCallbackOptions|AsyncHookCallbacks=} [callbacks]
 * @return {AsyncHook}
 */
export function createHook (callbacks) {
  return new AsyncHook(callbacks)
}

export default createHook
