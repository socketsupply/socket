/**
 * @module AsyncStorage
 *
 * Primitives for creating user defined async storage contexts.
 */
import { Snapshot, Variable } from './context.js'
import { AsyncResource } from './resource.js'
import { createHook } from './hooks.js'
import { executionAsyncResource } from '../internal/async/hooks.js'

const storages = []

// eslint-disable-next-line
const asyncLocalStorageHooks = createHook({
  init (asyncId, type, triggerAsyncId, resource) {
    const currentAsyncResource = executionAsyncResource()
    // eslint-disable-next-line
    for (var i = 0; i < storages.length; ++i) {
      storages[i].propagateTriggerResourceStore(
        resource,
        currentAsyncResource,
        type
      )
    }
  }
})

/**
 * A container for storing values that remain present during
 * asynchronous operations.
 */
export class AsyncLocalStorage {
  #store = null
  #enabled = false
  #variable = new Variable()

  /**
   * Binds function `fn` to run in the execution context of an
   * anonymous `AsyncResource`.
   * @param {function} fn
   * @return {function}
   */
  static bind (fn) {
    return AsyncResource.bind(fn)
  }

  /**
   * Captures the current async context and returns a function that runs
   * a function in that execution context.
   * @return {function}
   */
  static snapshot () {
    // eslint-disable-next-line
    return AsyncResource.bind(Snapshot.wrap((cb, ...args) => cb(...args)))
  }

  /**
   * @type {boolean}
   */
  get enabled () {
    return this.#enabled
  }

  /**
   * Disables the `AsyncLocalStorage` instance. When disabled,
   * `getStore()` will always return `undefined`.
   */
  disable () {
    if (this.#enabled) {
      this.#enabled = false
      const index = storages.indexOf(this)
      if (index > -1) {
        storages.splice(index, 1)
      }
    }
  }

  /**
   * Enables the `AsyncLocalStorage` instance.
   */
  enable () {
    if (!this.#enabled) {
      this.#enabled = true
      storages.push(this)
    }
  }

  /**
   * Enables and sets the `AsyncLocalStorage` instance default store value.
   * @param {any} store
   */
  enterWith (store) {
    this.enable()
    this.#variable = new Variable({ defaultValue: store })
  }

  /**
   * Runs function `fn` in the current asynchronous execution context with
   * a given `store` value and arguments given to `fn`.
   * @param {any} store
   * @param {function} fn
   * @param {...any} args
   * @return {any}
   */
  run (store, fn, ...args) {
    this.enable()
    return this.#variable.run(store, () => {
      return Reflect.apply(fn, null, args)
    })
  }

  exit (fn, ...args) {
    if (!this.#enabled) {
      return Reflect.apply(fn, null, args)
    }

    try {
      this.disable()
      return Reflect.apply(fn, null, args)
    } finally {
      // revert
      this.enable()
    }
  }

  /**
   * If the `AsyncLocalStorage` instance is enabled, it returns the current
   * store value for this asynchronous execution context.
   * @return {any|undefined}
   */
  getStore () {
    if (this.#enabled) {
      return this.#variable.get()
    }
  }
}

export default AsyncLocalStorage
