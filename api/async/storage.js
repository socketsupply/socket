/**
 * @module AsyncStorage
 *
 * Primitives for creating user defined async storage contexts.
 */
import { AsyncResource } from './resource.js'
import { createHook } from './hooks.js'
import { Snapshot } from './context.js'
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
    const resource = executionAsyncResource()
    resource[this.#resourceStoreSymbol] = store
  }

  /**
   * TODO
   */
  run (store, callback, ...args) {
    if (Object.is(this.getStore(), store)) {
      return Reflect.apply(callback, null, args)
    }

    this.enable()
    const resource = executionAsyncResource()
    const previousStore = resource[this.#resourceStoreSymbol]

    try {
      // push
      resource[this.#resourceStoreSymbol] = store
      return Reflect.apply(callback, null, args)
    } finally {
      // pop
      resource[this.#resourceStoreSymbol] = previousStore
    }
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
      const resource = executionAsyncResource()
      return resource[this.#resourceStoreSymbol]
    }
  }
}

export default AsyncLocalStorage
