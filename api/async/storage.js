/**
 * @module AsyncStorage
 *
 * Primitives for creating user defined async storage contexts.
 */
import { Snapshot, Variable } from './context.js'
import { AsyncResource } from './resource.js'
import { createHook } from './hooks.js'
import {
  executionAsyncResource,
  asyncContextVariable
} from '../internal/async/hooks.js'

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
 * TODO
 */
export class AsyncLocalStorage {
  #store = null
  #enabled = false
  #variable = new Variable()
  #resourceStoreSymbol = Symbol('resourceStore')

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
    const resource = executionAsyncResource()
    console.log({ resource0: resource })
    return asyncContextVariable.run(resource, () => {
      console.log({ resource1: resource })
      // eslint-disable-next-line
      const snapshot = Snapshot.wrap((cb, ...args) => cb(...args))
      return AsyncResource.bind(snapshot)
    })
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
      if (index > -1) {
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

  /**
   * @ignore
   * @param {AsyncResource} resource
   * @param {AsyncResource} triggerResource
   */
  propagateTriggerResourceStore (resource, triggerResource) {
    if (this.enabled) {
      resource[this.#resourceStoreSymbol] = triggerResource[this.#resourceStoreSymbol]
    }
  }

  /**
   * @param {any}
   */
  enterWith (store) {
    this.enable()
    this.#store = store
  }

  /**
   * TODO
   */
  run (store, callback, ...args) {
    this.enable()
    this.variable.run(store, () => {
      return Reflect.apply(callback, null, args)
    })
  }

  exit (callback, ...args) {
    if (!this.#enabled) {
      return Reflect.apply(callback, null, args)
    }

    try {
      this.disable()
      return Reflect.apply(callback, null, args)
    } finally {
      // revert
      this.enable()
    }
  }

  getStore () {
    if (this.#enabled) {
      return this.variable.get() ?? this.#store
    }
  }
}

export default AsyncLocalStorage
