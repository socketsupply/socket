import AsyncContext from './async-context.js'
import {
  hooks,
  getNextAsyncResourceId,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId,
  getDefaultExecutionAsyncId,
  asyncContextVariable
} from './internal/async_hooks.js'

// import gc from './gc.js'

// const topLevelAsyncResourceStore = {}
const asyncLocalStorages = []

/**
 * @typedef {{
 *   triggerAsyncId?: number,
 *   requireManualDestroy?: boolean
 * }} AsyncResourceOptions
 */

/**
 * @typedef {{
 *   init?: function (number, string, number, AsyncResource),
 *   before?: function (number),
 *   after?: function (number),
 *   destroy?: function (number)
 * }} AsyncHookCallbackOptions
 */

export {
  executionAsyncId,
  executionAsyncResource,
  triggerAsyncId
}

/**
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
 * TODO
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
      const snapshot = AsyncContext.Snapshot.wrap((cb, ...args) => cb(...args))
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
      const index = asyncLocalStorages.indexOf(this)
      if (index > -1) {
        asyncLocalStorages.splice(index, 1)
      }
    }
  }

  /**
   * TODO
   */
  enable () {
    if (!this.#enabled) {
      this.#enabled = true
      asyncLocalStorages.push(this)
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

/**
 * @param {AsyncHookCallbackOptions} [options]
 * @return {AsyncHook}
 */
export function createHook (callbacks) {
  return new AsyncHook(callbacks)
}

// eslint-disable-next-line
const asyncLocalStorageHooks = createHook({
  init (asyncId, type, triggerAsyncId, resource) {
    const currentAsyncResource = executionAsyncResource()
    // eslint-disable-next-line
    for (var i = 0; i < asyncLocalStorages.length; ++i) {
      asyncLocalStorages[i].propagateTriggerResourceStore(
        resource,
        currentAsyncResource,
        type
      )
    }
  }
})

export default {
  AsyncLocalStorage,
  AsyncResource,
  AsyncHook,
  createHook,
  executionAsyncResource,
  executionAsyncId,
  triggerAsyncId
}
